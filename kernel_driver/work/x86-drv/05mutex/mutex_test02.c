/****************************
 * PIPE的例子
 * 实现循环缓冲区，模拟PIPE的使用方式
 * 必须提供锁保护，要支持阻塞机制
 * Author: zht
 * Date: 2013-01-17
 ****************************/
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/proc_fs.h>
#include <linux/wait.h> /* wait_queue_head_t */
#include <linux/mutex.h> /* mutex */
#include <asm/atomic.h> /* atomic_t */

#define DEF_LEN 	100
#define DEF_MAJOR 	70

struct mem_priv {
	char *start, *end;
	int buf_len;
	char *wp, *rp;
	atomic_t open_count;
	dev_t dev_id;
	struct cdev mem_cdev;
	wait_queue_head_t writeq, readq;
	struct mutex mem_lock;
};

static struct mem_priv *dev;

static int 
mem_proc_read(char *page, char **start, off_t off, int count, int *eof, void *data)
{
	int ret = 0;

	/* lock */
	mutex_lock(&dev->mem_lock);

	ret += sprintf(page+ret, "-----DEV %d:%d-----\n", MAJOR(dev->dev_id), MINOR(dev->dev_id));
	ret += sprintf(page+ret, "buf_len = %dbytes; from 0x%p to 0x%p\n", dev->buf_len, dev->start, dev->end);
	ret += sprintf(page+ret, "wp = 0x%p; rp = 0x%p;\nopen_count = %d\n", dev->wp, dev->rp, atomic_read(&dev->open_count));

	/* unlock */
	mutex_unlock(&dev->mem_lock);

	return ret;
}


static int 
mem_open(struct inode *inode, struct file *filp)
{
	atomic_inc(&dev->open_count);
	return 0;
}


static int 
mem_release(struct inode *inode, struct file *filp)
{
	atomic_dec(&dev->open_count);
	return 0;
}

static ssize_t 
mem_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
	if (0 == count)
		return 0;

	/* 获得锁 */
	//mutex_lock(&...);
	if (mutex_lock_interruptible(&dev->mem_lock))
		return -ERESTARTSYS;
	
	/* 如果缓冲区空，则先解锁，再wait_event */
	while (dev->wp == dev->rp) {
		mutex_unlock(&dev->mem_lock);

		if (wait_event_interruptible(dev->readq, (dev->wp != dev->rp))) {
			return -ERESTARTSYS;
		}
		if (mutex_lock_interruptible(&dev->mem_lock))
			return -ERESTARTSYS;
	}

	/* 至此，则缓存区非空，并且持有锁 */
	if (dev->wp > dev->rp)
		count = min(count, (size_t)(dev->wp - dev->rp));
	else
		count = min(count, (size_t)(dev->end - dev->rp));

	if (copy_to_user(buf, dev->rp, count)) {
		mutex_unlock(&dev->mem_lock);
		return -EFAULT;
	}

	/* 更新rp指针，如果到了缓冲区的结尾，则回卷 */
	dev->rp += count;
	if (dev->rp == dev->end)
		dev->rp = dev->start;

	/* unlock */
	mutex_unlock(&dev->mem_lock);

	/* wakeup writeq */
	//wake_up_interruptible(&dev->writeq);
	wake_up(&dev->writeq);

	return count;
}


/* return 0: buffer full
 * return 1~99: buffer not full */
static int 
space_free(struct mem_priv *tmp)
{
	if (tmp->wp < tmp->rp)
		return (tmp->rp - tmp->wp - 1);
	else /* tmp->wp >= tmp->rp) */
		return (tmp->buf_len + tmp->rp - tmp->wp - 1);
}


static ssize_t 
mem_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
	if (count == 0)
		return 0;

	/* 先获得锁 */
	if (mutex_lock_interruptible(&dev->mem_lock))
		return -ERESTARTSYS;
	
	/* 如果缓冲区满，则先释放锁，再睡眠 */
	while (!space_free(dev)) {
		mutex_unlock(&dev->mem_lock);
		wait_event(dev->writeq, space_free(dev));
		mutex_lock(&dev->mem_lock);
	}

	count = min(count, (size_t)space_free(dev));
	if (dev->wp >= dev->rp)
		count = min(count, (size_t)(dev->end - dev->wp));

	/* copy_from_user */
	if (copy_from_user(dev->wp, buf, count)) {
		mutex_unlock(&dev->mem_lock);
		return -EFAULT;
	}

	dev->wp += count;
	if (dev->wp == dev->end)
		dev->wp = dev->start;

	/* unlock */
	mutex_unlock(&dev->mem_lock);

	wake_up_interruptible(&dev->readq);

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

	/* create /proc/mem_pipe */
	create_proc_read_entry("mem_pipe",
		0, NULL,
		mem_proc_read, NULL);

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
	atomic_set(&dev->open_count, 0);

	/* init wait_queue_head */
	init_waitqueue_head(&dev->writeq);
	init_waitqueue_head(&dev->readq);

	/* init mutex */
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
	remove_proc_entry("mem_pipe", NULL);

	return ret;
}

static void __exit my_exit(void)
{
	remove_proc_entry("mem_pipe", NULL);
	cdev_del(&dev->mem_cdev);
	kfree(dev->start);
	kfree(dev);
}

module_init(my_init);
module_exit(my_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("ZHANG");

