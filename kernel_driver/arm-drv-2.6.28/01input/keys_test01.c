/***********************
 * 支持6个按键的input驱动
 * 支持linux-2.6.28和linux-3.4.4
 * author: zht
 * date: 2013-01-22
 ***********************/
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/timer.h>
#include <linux/slab.h>
#include <linux/input.h>

#define KEY_NUM	 6

/* key的私有结构体(每个按键一个) */
struct key_priv {
	int 	key_irq; /* 按键对应的中断号 */
	char 	irq_name[10];
	int	key_code; /* 按键值 */
	struct timer_list timer;
	int 	ready;
	unsigned long last_press;
};

/* 每个按键分配一个key_priv
 * 六个按键合在一起分配一个input_dev */
static struct key_priv *devs[KEY_NUM];
static struct input_dev *idev;
static int codes[KEY_NUM] = {KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT, KEY_ENTER, KEY_ESC};


/*********************
 * 定时器的处理函数
 *********************/
static void 
key_timer_func(unsigned long data)
{
	struct key_priv *dev = (void *)data;

	if (dev->last_press == 0) {
		dev->last_press = jiffies;
		goto pressed;
	}

	/* 如果和前一次按键中断间的间隔过短，则不记录 */
	if ((dev->last_press+60) > jiffies) {
		dev->ready = 0;
		return;
	}

	/* 报告按键按下后迅速报告按键抬起，其实不太合理 */
pressed:
	input_report_key(idev, dev->key_code, 1);
	input_report_key(idev, dev->key_code, 0);
	input_sync(idev);
	dev->ready = 0;
	dev->last_press = jiffies;
}


/************************
 * 中断处理函数
 ************************/
static irqreturn_t 
key_handler(int irq, void *dev_id)
{
	struct key_priv *dev = dev_id;
	if (!dev->ready++)
		mod_timer(&dev->timer, (jiffies+5));
	return IRQ_HANDLED;
}


/* 初始化6个按键 */
static int keys_init(void)
{
	int i,ret;
	unsigned long flags = IRQF_SHARED | IRQF_TRIGGER_FALLING;

	for (i=0; i<KEY_NUM; i++) {
		devs[i] = (struct key_priv *)kzalloc(sizeof(*devs[i]), GFP_KERNEL);
		if (NULL == devs[i])
			return -ENOMEM;

		/* 设定按键值 */
		devs[i]->key_code = codes[i];
		set_bit(devs[i]->key_code, idev->keybit);

		/* 初始化定时器 */
		setup_timer(&devs[i]->timer, key_timer_func, (unsigned long)devs[i]);
	
		/* 注册中断处理函数 */
		devs[i]->key_irq = S3C_EINT(i);
		sprintf(devs[i]->irq_name, "keypad%d", i);
	 	ret = request_irq(devs[i]->key_irq, 
			key_handler,
			flags,
			devs[i]->irq_name,
			devs[i]);
		if (ret) {
			printk("Request keypad irq %d error\n", devs[i]->key_irq);
			kfree(devs[i]);	
			devs[i] = NULL;
			return ret;
		}
	}

	return 0;
}


/* 释放6个按键对应的资源 */
static void keys_exit(void)
{
	int i;
	for (i=0; i<KEY_NUM; i++) {
		if (devs[i]) {
			free_irq(devs[i]->key_irq, devs[i]);
			del_timer(&devs[i]->timer);
			kfree(devs[i]);
		}
	}
}


static int __init my_init(void)
{
	int ret = 0;

	printk("=====init S3C keypad=====\n");

	/* 分配并初始化input_dev */
	idev = input_allocate_device();
	if (!idev) {
		return -ENOMEM;
	}
	
	set_bit(EV_SYN, idev->evbit);
	set_bit(EV_KEY, idev->evbit);
	idev->name = "keypad";
	idev->phys = "keypad-phy";
	idev->id.bustype = BUS_HOST;
	idev->id.vendor = 0xdead;
	idev->id.product = 0xbeaf;
	idev->id.version = 0x0101;

	/* 初始化6个按键 */
	ret = keys_init();
	if (ret) {
		printk("Init key_priv failed\n");
		goto err;
	}

	/* 将input_dev注册到input子系统 */
	ret = input_register_device(idev);
	if(ret) {
		printk("Cannot register input_dev(keys)!\n");
		ret = -EIO;
		goto err;
	}
	printk("=====keypad OK=====\n");

	return 0;
err:
	keys_exit();
	input_free_device(idev);
	return ret;
}


static void __exit my_exit(void)
{
	keys_exit();
	input_unregister_device(idev);
	input_free_device(idev);
}

module_init(my_init);
module_exit(my_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("ZHANG");

