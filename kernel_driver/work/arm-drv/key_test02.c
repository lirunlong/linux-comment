/*************************
 * 利用开发板上的4个按键进行中断处理的测试
 * 在按键的中断处理函数中，切换对应的灯的状态
 * author: zht
 * date: 2012-08-23
 *************************/

#include <linux/module.h>
#include <linux/sched.h>
#include <linux/interrupt.h> /* request_irq */
#include <linux/ioport.h>
#include <linux/io.h>
#include <plat/irqs.h>

#define GPM_BASE 	0x7F008820
#define GPM_SIZE	0xC
#define GPMCON		0x0
#define	GPMDAT		0x4
#define GPMPUP		0x8

#define KEY_NUM	 	4

struct key_priv {
	int key_irq;	/* 按键对应的中断号 */
	int led_num;	/* 控制的灯 */
	char irq_name[10]; /* 中断处理的名字 */
	int key_count;	/* 中断计数 */
};

static struct key_priv *keys[KEY_NUM];
static void __iomem *vir_base;


/* 中断处理函数: 可以为4个按键提供服务 */
static irqreturn_t 
key_service(int irq, void *dev_id)
{
	struct key_priv *tmp = dev_id;
	int value;

	/* 反转灯的状态 */
	value = readl(vir_base + GPMDAT);
	value ^= (0x1 << tmp->led_num);
	writel(value, (vir_base + GPMDAT));

	tmp->key_count++;
	printk("irq %d: %d times\n", tmp->key_irq, tmp->key_count);

	return IRQ_HANDLED; /* IRQ_NONE */
}


static int __init my_init(void)
{
	int i, ret;
	unsigned long flags, value;

	/* request_mem_region */
	vir_base = ioremap(GPM_BASE, GPM_SIZE);
	if (!vir_base)
		return -EIO;

	for (i=0; i<KEY_NUM; i++) {
		/* 分配并初始化key_priv */
		keys[i] = kzalloc(sizeof(*keys[i]), GFP_KERNEL);
		if (!keys[i]) {
			ret = -ENOMEM;
			goto err;
		}

		keys[i]->key_irq = S3C_EINT(i);
		keys[i]->key_count = 0;
		keys[i]->led_num = i;
		sprintf(keys[i]->irq_name, "key%d", i);

		/* 设置灯的IO为输出 */
		value = readl(vir_base + GPMCON);
		value &= ~(0xF << (i*4));
		value |= (0x1 << (i*4));
		writel(value, (vir_base + GPMCON));
	
		/* turn off ledx */
		value = readl(vir_base + GPMDAT);
		value |= (0x1 << i);
		writel(value, (vir_base + GPMDAT));

		/* 注册中断处理函数 */
		flags = IRQF_SHARED | 
			IRQF_SAMPLE_RANDOM | 
			IRQF_TRIGGER_FALLING;
			//IRQF_TRIGGER_LOW;

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

	iounmap(vir_base);
	for (i=0; i<KEY_NUM; i++) {
		if (keys[i]) {
			free_irq(keys[i]->key_irq, keys[i]);
			kfree(keys[i]);
		}
	}
}

module_init(my_init);
module_exit(my_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("ZHT");

