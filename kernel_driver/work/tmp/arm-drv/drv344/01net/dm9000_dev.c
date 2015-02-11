/***********************
 * 为DM9000准备并注册platform_device
 * author: zht
 * date: 2013-04-02
 ***********************/
#include <linux/module.h>
#include <linux/ioport.h>
#include <linux/platform_device.h>
#include <linux/dm9000.h>
#include <mach/irqs.h>

/* DM9000的物理地址和中断号通过读电路图获得 */
#define PORT_INDEX	0x18000300
#define PORT_DATA	0x19000304
#define PORT_SIZE	4
#define DM9000_IRQ	S3C_EINT(7)

//release函数，在注销pdev时调用
static void dm9k_release(struct device *dev)
{
	printk("Release dm9000 pdev\n");
}

//准备资源数组
static struct resource dm9k_res[] = {
	[0] = {
		.start = PORT_INDEX, 
		.end = PORT_INDEX + PORT_SIZE - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = PORT_DATA,
		.end = PORT_DATA + PORT_SIZE - 1,
		.flags = IORESOURCE_MEM,
	},
	[2] = {
		.start = DM9000_IRQ,
		.end = DM9000_IRQ,
		.flags = IORESOURCE_IRQ,
	},
};

//准备包含设备特定信息的结构体
static struct dm9000_plat_data pdata = {
	.flags = DM9000_PLATF_16BITONLY | 
		 DM9000_PLATF_NO_EEPROM,
	.dev_addr = {0x0, 0xab, 0x23, 0xff, 0x55, 0x14},
};

//准备platform_device
static struct platform_device dm9k_dev = {
	.name = "shrek-dm9k",
	.id = -1,
	.num_resources = ARRAY_SIZE(dm9k_res),
	.resource = dm9k_res,
	.dev = {
		.platform_data = &pdata,
		.release = dm9k_release,
	},
};

static int __init my_init(void)
{
	//注册platform_device
	return platform_device_register(&dm9k_dev);
}

static void __exit my_exit(void)
{
	platform_device_unregister(&dm9k_dev);
}

module_init(my_init);
module_exit(my_exit);

MODULE_AUTHOR("ZHT");
MODULE_LICENSE("GPL");

