/******************
 * 模块的测试例子
 * author: zht
 * date: 2013-03-18
 ******************/
#include <linux/module.h>

static void my_print(int value)
{
	printk("In my_print(): Value = %d\n", value);
}
EXPORT_SYMBOL(my_print);


/* 模块的入口函数，在insmod时调用 */
int __init my_init(void)
{
	printk("Hello kernel\n");
	return 0;
}

/* 模块的出口函数，在rmmod时调用 */
static void __exit my_exit(void)
{
	printk("Bye, I've to go\n");
}

module_init(my_init);
module_exit(my_exit);

MODULE_AUTHOR("ZHT");
MODULE_LICENSE("GPL");
