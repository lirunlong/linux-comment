/*************************
 * 利用开发板上的4个按键进行atomic_t计数的测试
 * 用普通变量和atomic_t变量进行计数，理论上讲，普通变量可能会丢计数
 * author: zht
 * date: 2012-08-23
 *************************/

#include <linux/module.h>
#include <linux/sched.h>
#include <linux/interrupt.h> /* request_irq */
#include <linux/proc_fs.h>
#include <asm/atomic.h>
#include <plat/irqs.h>

#define KEY_NUM	 	4

struct key_priv {
	int key_irq;	/* 按键对应的中断号 */
	char irq_name[10]; /* 中断处理的名字 */
};

static struct key_priv *keys[KEY_NUM];
static int manual_i = 0;
static atomic_t atom_i = ATOMIC_INIT(0);

static int atomic_proc_read(char *page, char **start, off_t off, int count, int *eof, void *data)
{
	int ret = 0;
	ret += sprintf(page, "manual_i = %d; atom_i = %d\n", manual_i, atomic_read(&atom_i));

	return ret;
}


/* 中断处理函数: 可以为4个按键提供服务 */
static irqreturn_t 
key_service(int irq, void *dev_id)
{
	manual_i++;
	atomic_inc(&atom_i);

	return IRQ_HANDLED; /* IRQ_NONE */
}


static int __init my_init(void)
{
	int i, ret;
	unsigned long flags;

	for (i=0; i<KEY_NUM; i++) {
		/* 分配并初始化key_priv */
		keys[i] = kzalloc(sizeof(*keys[i]), GFP_KERNEL);
		if (!keys[i]) {
			ret = -ENOMEM;
			goto err;
		}

		keys[i]->key_irq = S3C_EINT(i);
		sprintf(keys[i]->irq_name, "atom%d", i);

		/* 注册中断处理函数 */
		flags = IRQF_SHARED | 
			IRQF_SAMPLE_RANDOM | 
			//IRQF_TRIGGER_FALLING;
			IRQF_TRIGGER_LOW;

		ret = request_irq(keys[i]->key_irq,
			key_service, 
			flags, 
			keys[i]->irq_name,
			keys[i]);
		if (ret) {
			kfree(keys[i]);
			keys[i] = NULL;
			printk("Cannot request irq %d\n", keys[i]->key_irq);
			goto err;
		}
	}

	create_proc_read_entry("proc_atomic", 0,
		NULL, atomic_proc_read, NULL);

	return 0;
err:
	for (i=0; i<KEY_NUM; i++) {
		if (keys[i]) {
			free_irq(keys[i]->key_irq, keys[i]);
			kfree(keys[i]);
		}
	}

	return ret;
}

static void __exit my_exit(void)
{
	int i;

	for (i=0; i<KEY_NUM; i++) {
		if (keys[i]) {
			free_irq(keys[i]->key_irq, keys[i]);
			kfree(keys[i]);
		}
	}
	remove_proc_entry("proc_atomic", NULL);
}

module_init(my_init);
module_exit(my_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("ZHT");

