/**********************
 * 模块参数的例子
 * author: zht
 * date: 2013-03-19
 **********************/
#include <linux/module.h>

static int value = 10;
static char *name = "shrek";
module_param(value, int, 0444);
module_param(name, charp, 0755);

static int __init my_init(void)
{
	printk("value = %d; name = %s\n", value, name);
	return 0;
}

static void __exit my_exit(void)
{}

module_init(my_init);
module_exit(my_exit);

MODULE_AUTHOR("ZHT");
MODULE_LICENSE("GPL");

