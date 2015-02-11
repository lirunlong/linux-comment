/***********************
 * Driver for shrek6410 board LED 0~3
 * control led through "echo on|off > /dev/ledx"
 * author: zht
 * date: 2012-07-18
 ***********************/

#include <linux/module.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/ioport.h>
#include <linux/io.h>

#define GPM_BASE 	0x7F008820
#define GPM_SIZE	0x8
#define GPMCON		0x0
#define	GPMDAT		0x4

#define LED_MAJOR	60
#define LED_NUM		4


/* private struct for led control */
struct led_priv {
	int 	led_num; /* led 0~3 */
	int 	led_state;/* on: 1; off: 0 */
	dev_t	led_id;
	struct 	cdev led_cdev;
};

static struct led_priv *leds[LED_NUM];
static struct resource *led_res;
static void __iomem *vir_base;

static int 
proc_led_read(char *page, char **start, off_t off, int count, int *eof, void *data)
{
	int i, ret = 0;

	ret += sprintf(page+ret, "----LED 0~%d states----\n", (LED_NUM-1));
	for (i=0; i<LED_NUM; i++) {
		if (leds[i]) {
			ret += sprintf(page+ret, "Led %d: %s\n", i, ((leds[i]->led_state) ? "on" : "off"));
		}
	}

	return ret;
}


static int 
led_open(struct inode *inode, struct file *filp)
{
	struct led_priv *tmp = container_of(inode->i_cdev, struct led_priv, led_cdev);

	filp->private_data = tmp;

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
	char *tmp;
	int value;
	struct led_priv *dev = filp->private_data;

	tmp = (char *)kzalloc(count, GFP_KERNEL);
	if (NULL == tmp)
		return -ENOMEM;

	if (copy_from_user(tmp, buf, count)) {
		kfree(tmp);
		return -EFAULT;
	}

	/* led on/off */
	if (strncmp(tmp, "on", 2) == 0) {
		value = readl(vir_base + GPMDAT);
		value &= ~(0x1 << dev->led_num);
		writel(value, (vir_base + GPMDAT));
		dev->led_state = 1;
	} else if (strncmp(tmp, "off", 3) == 0) {
		value = readl(vir_base + GPMDAT);
		value |= (0x1 << dev->led_num);
		writel(value, (vir_base + GPMDAT));
		dev->led_state = 0;
	} else {
		printk("Use on|off to control led\n");
	}

	kfree(tmp);

	return count;
}


static struct file_operations led_fops = {
	.owner = THIS_MODULE,
	.open	= led_open,
	.release = led_release,
	.write	= led_write,
};


static int 
ledx_init(void)
{
	int i;
	int value;

	for (i=0; i<LED_NUM; i++) {
		leds[i] = (struct led_priv *)kzalloc(sizeof(*leds[i]), GFP_KERNEL);
		if (NULL == leds[i])
			return -ENOMEM;

		leds[i]->led_num = i;	/* led 0~3 */
		leds[i]->led_state = 0; /* default off */

		/* 将led配置为输出 */
		value = readl(vir_base + GPMCON);
		value &= ~(0xF << (i*4));
		value |= (0x1 << (i*4));
		writel(value, (vir_base + GPMCON));
	
		/* 默认将LED关闭 */
		value = readl(vir_base + GPMDAT);
		value |= (0x1 << i);
		writel(value, (vir_base + GPMDAT));
	
		/* cdev init & add */
		leds[i]->led_id = MKDEV(LED_MAJOR, i);
		cdev_init(&leds[i]->led_cdev, &led_fops);
		leds[i]->led_cdev.owner = THIS_MODULE;
		cdev_add(&leds[i]->led_cdev, leds[i]->led_id, 1);
	}

	return 0;
}


static void 
ledx_exit(void)
{
	int i;
	for (i=0; i<LED_NUM; i++) {
		if (leds[i]) {
			cdev_del(&leds[i]->led_cdev);
			kfree(leds[i]);
		}
	}
}


static int __init my_init(void)
{
	int ret = 0;

	/* request_mem_region */
	led_res = request_mem_region(GPM_BASE, GPM_SIZE, "gpio port M");
	if (NULL == led_res)
		printk("Cannot request iomem region for GPIO port M\n");

	/* ioremap for GPIO port M */
	vir_base = ioremap(GPM_BASE, GPM_SIZE);
	if (NULL == vir_base) {
		printk("Cannot ioremap for: 0x%x\n", GPM_BASE);
		ret = -EIO;
		goto err1;
	}
	printk("GPIO M port ioremap to 0x%p\n", vir_base);

	/* alloc & init led_priv */
	ret = ledx_init();
	if (ret) {
		printk("alloc & init led_priv error\n");
		goto err2;
	}
		
	/* create /proc/led_state */
	create_proc_read_entry("led_state", /* name */
			0444,	/* mode */
			NULL, /* parent */
			proc_led_read, /* func */
			NULL);

	return 0;
err2:
	ledx_exit();
	iounmap(vir_base);
err1:
	if (led_res)
		release_mem_region(GPM_BASE, GPM_SIZE);
	return ret;
}


static void __exit 
my_exit(void)
{
	remove_proc_entry("led_state", NULL);
	ledx_exit();
	iounmap(vir_base);
	if (led_res)
		release_mem_region(GPM_BASE, GPM_SIZE);
}

module_init(my_init);
module_exit(my_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("ZHANG");

