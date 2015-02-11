/******************
 * 模块的测试例子
 * author: zht
 * date: 2013-03-18
 ******************/
#include <linux/module.h>

//函数的原型声明
extern void my_print(int);


/* 模块的入口函数，在insmod时调用 */
static int __init my_init(void)
{
	printk("In test02.c, call my_print()\n");
	my_print(13);
	return 0;
}

/* 模块的出口函数，在rmmod时调用 */
static void __exit my_exit(void)
{
}

module_init(my_init);
module_exit(my_exit);

MODULE_AUTHOR("ZHT");
MODULE_LICENSE("GPL");
