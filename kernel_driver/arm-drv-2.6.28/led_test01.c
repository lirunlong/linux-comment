/**********************
 * LED灯的char驱动
 * 驱动支持4个LED等，用on|off控制
 * author: zht
 * date: 2013-03-22
 **********************/
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/ioport.h>
#include <linux/io.h>
#include <linux/proc_fs.h>

#define LED_NUM 	4
#define LED_MAJOR	60

#define GPM_BASE	0x7F008000
#define GPM_SIZE	0x1000
#define GPMCON		0x820
#define GPMDAT		0x824

/* 和LED灯对应的私有结构体 */
struct led_priv {
	int led_state; 	/* on:1; off:0 */
	int led_num;  	/* led0~3 */
	dev_t dev_id;	/* 设备号 */
	struct cdev led_cdev; 
};

struct led_priv *leds[LED_NUM];
static void __iomem *vir_base;

//和灯对应的/proc/led_state文件的读函数
static int 
led_proc_read(char *page, char **start, off_t off, int count, int *eof, void *data)
{
	//在for循环中，依次打印每个灯的设备号和当前状态
}

static int
led_open(struct inode *inode, struct file *filp)
{
	//找到用户态要控制的灯对应的led_priv
	//将led_priv存入filp->private_data;
	return 0;
}

static int
led_release(struct inode *inode, struct file *filp)
{
	return 0;
}

static ssize_t
led_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
	struct led_priv *tmp = filp->private_data;

	//为buf执行的字符串分配空间
	//copy_from_user
	//可以使用strcmp/strncmp判断输入的是on还是off
	//如果字符串内容为on，则设置GPMDAT寄存器的对应位点亮灯，为off则关闭对应灯
	//kfree(xxx);
	
	return count;
}

static struct file_operations my_fops = {
	.owner = THIS_MODULE,
	.open = led_open,
	.release = led_release,
	.write = led_write,
};


static int __init my_init(void)
{
	vir_base = ioremap(GPM_BASE, GPM_SIZE);
	if (!vir_base) 
		return -EIO;

	//创建/proc/led_test文件

	//在for循环中，分配并初始化4个led_priv
	//循环中还要将每个LED对应的IO配置为输出
	//为每个灯分配设备号，cdev_add

	return 0;
err:
	...
}

static void __exit my_exit(void)
{
	//循环释放4个led_priv
	remove_proc_entry("led_state", NULL);
	iounmap(vir_base);
}

module_init(my_init);
module_exit(my_exit);

MODULE_AUTHOR("ZHT");
MODULE_LICENSE("GPL");

