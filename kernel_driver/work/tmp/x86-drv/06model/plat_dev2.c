/********************
 * platform_device
 * 针对第二个网卡
 ********************/
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/platform_device.h>

#define BASE_8101E	0xE3010000
#define SIZE_8101E	256
#define	IRQ_TEST	10

static void 
plat_release(struct device *dev)
{
	struct platform_device *tmp = container_of(dev, struct platform_device, dev);

	printk("Release device %s, id is %d\n", tmp->name, tmp->id);
}


static struct resource plat_res[] = {
	[0] = {
		.start 	= BASE_8101E,
		.end 	= BASE_8101E + SIZE_8101E - 1,
		.flags	= IORESOURCE_MEM,
	},	
	[1] = {
		.start 	= IRQ_TEST,
		.end	= IRQ_TEST,
		.flags	= IORESOURCE_IRQ,
	},
};


static struct platform_device pdev = {
	.name 	= "shrek-net",
	.id 	= 2,
	.num_resources 	= ARRAY_SIZE(plat_res),
	.resource = plat_res,
	.dev = {
		.release = plat_release,
	}
};
	

static int __init my_init(void)
{
	/* register platform_device */
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

