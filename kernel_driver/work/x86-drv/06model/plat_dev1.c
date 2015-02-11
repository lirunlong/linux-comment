/*************************
 * 针对PCI网卡准备的platform_device
 * 网卡本身有对应的pci_dev，本例子为了测试，又创建一个platform_device
 * author: zht
 * date: 2012-08-28
 *************************/

#include <linux/module.h>
#include <linux/sched.h>
#include <linux/platform_device.h>

#define BASE_8139	0xE2000000
#define SIZE_8139	0x100
#define	IRQ_8139	20


/* 在platform_device注销时调用 */
static void 
plat_release(struct device *dev)
{
	struct platform_device *tmp = container_of(dev, struct platform_device, dev);
	printk("Release device %s, id is %d\n", tmp->name, tmp->id);
}


static struct resource plat_res[] = {
	[0] = {
		.start 	= BASE_8139,
		.end 	= BASE_8139 + SIZE_8139 - 1,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start 	= IRQ_8139,
		.end	= IRQ_8139,
		.flags	= IORESOURCE_IRQ,
	},
};


static struct platform_device pdev = {
	.name 	= "shrek-net",
	.id 	= 1,
	.num_resources = ARRAY_SIZE(plat_res),
	.resource = plat_res,
	.dev = {
		.release = plat_release,
	}
};


static int __init my_init(void)
{
	/* register platform_device */
	//如果是注册一组pdev，用函数：
	//platform_add_devices(struct platform_device **pdev, int num)
	return platform_device_register(&pdev);
}

static void __exit my_exit(void)
{
	/* unregister platform_device */
	platform_device_unregister(&pdev);
}

module_init(my_init);
module_exit(my_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("ZHT");

