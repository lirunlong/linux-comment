/**********************
 * 按键中断的测试例子
 * 在4个按键的中断处理函数中，切换对应LED的状态
 * author: zht
 * date: 2013-03-25
 **********************/
#include <linux/module.h>
#include <linux/ioport.h>
#include <linux/io.h>
#include <linux/interrupt.h>
#include <plat/irqs.h>

#define KEY_NUM		4
#define GPM_BASE 	0x7F008820
#define GPM_SIZE	0x8
#define GPMCON		0x0
#define	GPMDAT		0x4


/* 私有结构体，对应每个按键 */
struct key_priv {
	int led_num; /* 本按键要控制的灯 */
	int irq_num; /* 按键对应的中断号 */
	int irq_cnt; /* 按键中断发生的次数 */
	char irq_name[10]; /* 按键中断处理函数的名字 */
};

static struct key_priv *keys[KEY_NUM];
static void __iomem *vir_base; /* LED灯相关寄存器的基地址 */


/* 中断处理函数，为4个中断服务 */
static irqreturn_t
key_service(int irq, void *dev_id)
{
	//随着按键的不同，函数中传入的irq和dev_id也不同，建议使用dev_id，应该对应按键的key_priv
	struct key_priv *tmp = dev_id;
	tmp->irq_cnt++;
	printk("key %d: %d times\n", irq, tmp->irq_cnt);
	//切换灯的状态，可以使用 ^= 

	return IRQ_HANDLED;
}



static int __init my_init(void)
{
	//ioremap
	//分配并初始化4个key_priv，要利用宏S3C_EINT确定每个按键对应的中断号
	//初始化led灯
	//要为每个中断号调用request_irq，最后的dev_id参数应该使用对应的key_priv，每个中断最好传入不同的名字
	
	return 0;
}

static void __exit my_exit(void)
{
	//循环4次，free_irq
	//释放4个key_priv
	//iounmap
}

module_init(my_init);
module_exit(my_exit);

MODULE_AUTHOR("ZHT");
MODULE_LICENSE("GPL");

