/****************************
 * PIPE的例子
 * 通过echo向内存缓冲区写数据，通过cat读走
 * 无论是缓冲区空还是满，都需要睡眠
 * Author: zht
 * Date: 2013-03-27
 ****************************/
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/proc_fs.h>
#include <linux/wait.h> /* wait_queue_head_t */
#include <linux/mutex.h> /* mutex */

#define DEF_LEN 	100
#define DEF_MAJOR 	70

struct mem_priv {
	char *start, *end;
	int buf_len;
	char *wp, *rp;
	dev_t dev_id;
	struct cdev mem_cdev;
	wait_queue_head_t writeq, readq;
	struct mutex mem_lock;
};

static struct mem_priv *dev;


int mem_proc_read(char *page, char **start, off_t off, int count, int *eof, void *data)
{
	int ret = 0;

	//lock
	ret += sprintf(page+ret, "-----DEV %d:%d-----\n", MAJOR(dev->dev_id), MINOR(dev->dev_id));
	ret += sprintf(page+ret, "buf_len = %dbytes; from 0x%p to 0x%p\n", dev->buf_len, dev->start, dev->end);
	ret += sprintf(page+ret, "wp = 0x%p; rp = 0x%p\n", dev->wp, dev->rp);
	//unlock

	return ret;
}

int mem_open(struct inode *inode, struct file *filp)
{
	return 0;
}

int mem_release(struct inode *inode, struct file *filp)
{
	return 0;
}

ssize_t mem_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
	//mutex_lock;
	//判断缓冲区是否为空
	//如果缓冲区空，先解锁，再wait_event
	//再次获得锁(判断一下缓冲区是否非空)
	//到此为止，缓冲区非空且持有锁
	//copy_to_user
	//更新rp指针，如果rp==end，回卷
	//wake_up(&writeq);
	//mutex_unlock;

	return count;
}


ssize_t mem_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
	//mutex_lock;
	//判断缓冲区是否为满
	//如果缓冲区满，先解锁，再wait_event
	//再次获得锁(判断一下缓冲区是否非满)
	//到此为止，缓冲区非满且持有锁
	//copy_from_user
	//更新wp指针，如果wp==end，回卷
	//wake_up(&readq);
	//mutex_unlock;

	return count;
}


static struct file_operations mem_fops = {
	.owner = THIS_MODULE, 
	.open = mem_open,
	.release = mem_release,
	.read = mem_read,
	.write = mem_write,
};


static int __init my_init(void)
{
	int ret;

	/* create /proc/mem_test */
	create_proc_read_entry("mem_test", /* file name */
			0, 	 /* mode */
			NULL,  	/* parent */
			mem_proc_read, /* function */
			NULL); /* parameter */

	/* allocate & init mem_priv */
	dev = (struct mem_priv *)kzalloc(sizeof(*dev), GFP_KERNEL);
	if (NULL == dev) {
		ret = -ENOMEM;
		goto err_alloc;
	}

	dev->start = (char *)kzalloc(DEF_LEN, GFP_KERNEL);
	if (NULL == dev->start) {
		ret = -ENOMEM;
		goto err_buf;
	}
	dev->buf_len = DEF_LEN;
	dev->end = dev->start + dev->buf_len;
	dev->wp = dev->start;
	dev->rp = dev->start;

	/* init wait_queue_head */
	init_waitqueue_head(&dev->writeq);
	init_waitqueue_head(&dev->readq);
	
	/* 初始化锁 */
	mutex_init(&dev->mem_lock);

	/* init & add cdev */
	dev->dev_id = MKDEV(DEF_MAJOR, 0);
	cdev_init(&dev->mem_cdev, &mem_fops);
	dev->mem_cdev.owner = THIS_MODULE;
	cdev_add(&dev->mem_cdev, dev->dev_id, 1);

	return 0;

err_buf:
	kfree(dev);
err_alloc:
	remove_proc_entry("mem_test", NULL);

	return ret;
}

static void __exit my_exit(void)
{
	remove_proc_entry("mem_test", NULL);
	cdev_del(&dev->mem_cdev);
	kfree(dev->start);
	kfree(dev);
}

module_init(my_init);
module_exit(my_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("ZHANG");

