/*****************************
 * Test for timer_list
 * 通过读/proc/proc_timer文件来启动一个定时器，设定定时器循环执行10次)
 * Author: zht
 * Date: 2012-12-28
 *****************************/

#include <linux/module.h>
#include <linux/sched.h>
#include <linux/proc_fs.h>
#include <linux/timer.h> /* timer_list */

#define DEF_INTERVAL 	1 /* 循环间隔的ticks */
#define DEF_TIMES	10 /* 循环10次 */

struct timer_priv {
	struct timer_list mytimer;
	int interval; //循环的间隔
	int times; //剩余的执行次数
};

struct timer_priv *dev;

/* 定时器到期时的执行函数，由硬件定时器的中断处理函数调用 */
void timer_func(unsigned long data)
{
	struct timer_priv *tmp = (void *)data;

	printk("Timer %d: in %ld, task is %s, pid is %d\n", tmp->times, jiffies, current->comm, current->pid);

	if (tmp->times-- > 1) {
		tmp->mytimer.expires = jiffies + tmp->interval;
		add_timer(&tmp->mytimer);
	}
}


/************************
 * 当用户态用cat读proc文件时，启动timer_list
 * cat启动timer后，马上退出，不会等待到timer执行完
 ************************/
int proc_timer(char *page, char **start, off_t off, int count, int *eof, void *data)
{
	int ret = 0;

	/* start up timer */
	if (!dev->times) {
		dev->times = DEF_TIMES;
		mod_timer(&dev->mytimer, jiffies + dev->interval);
		ret += sprintf(page+ret, "Start timer in %ld, execute %d times\n", jiffies, dev->times);
	} else {
		ret += sprintf(page+ret, "Timer still running, please try again\n");
	}

	return ret;
}



static int __init my_init(void)
{
	dev = (struct timer_priv *)kzalloc(sizeof(*dev), GFP_KERNEL);
	if (NULL == dev)
		return -ENOMEM;

	/* 初始化定时器 */
	dev->interval = DEF_INTERVAL;
	dev->times = 0;
	setup_timer(&dev->mytimer, timer_func, (unsigned long)dev);

	create_proc_read_entry("proc_timer", 0, NULL,
			proc_timer, NULL);

	return 0;
}


static void __exit my_exit(void)
{
	remove_proc_entry("proc_timer", NULL);
	del_timer(&dev->mytimer);
	kfree(dev);
}

module_init(my_init);
module_exit(my_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("ZHANG");

