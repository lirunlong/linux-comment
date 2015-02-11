/**********************
 * 支持多个缓冲区的char驱动例子
 * author: zht
 * date: 2013-03-20
 **********************/
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/proc_fs.h>

#define DEV_NUM		3
#define BUF_SIZE	100
#define DEF_MAJOR	55

/* 为每个实际存在的设备分配一个私有结构体 */
struct mem_priv {
	char *start, *end;
	int buf_size;
	int wp;
	dev_t dev_id;
	struct cdev mem_cdev;
};

struct mem_priv *devs[DEV_NUM];

//在insmod时确定缓冲区的大小
static int sizes[DEV_NUM] = {BUF_SIZE, BUF_SIZE, BUF_SIZE};
static int num;
module_param_array(sizes, int, &num, 0444);


static int 
mem_open(struct inode *inode, struct file *filp)
{
	//确定用户态准备写入哪个缓冲区
	//可以使用inode->i_rdev和i_cdev
	//推荐用i_cdev，用container_of
	struct mem_priv *tmp = ...;
	filp->private_data = tmp;

	return 0;
}

static int
mem_release(struct inode *inode, struct file *filp)
{
	return 0;
}

static ssize_t
mem_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
	struct mem_priv *dev;
	dev = (struct mem_priv *)filp->private_data;

	count = min(count, (size_t)(dev->wp-*f_pos));
	if (0==count)
		return 0;

	/* copy_to_user(to,from,count) */
	if (copy_to_user(buf, (dev->start+*f_pos), count))
		return -EFAULT;

	*f_pos += count;
	return count;
}

static ssize_t 
mem_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
	count = min(count, (size_t)(dev->buf_size - dev->wp));
	if (0 == count)
		return 0;

	/* copy_from_user(to,from,count) */
	if (copy_from_user((dev->start+dev->wp), buf, count))
		return -EFAULT;

	dev->wp += count;
	return count;
}

static struct file_operations mem_fops = {
	.owner = THIS_MODULE, //&__this_module,
	.open = mem_open,
	.release = mem_release,
	.read = mem_read,
	.write = mem_write,
};

/* /proc/mem_test文件的读函数 */
static int mem_proc_read(char *page, char **start, off_t off, int count, int *eof, void *data)
{
	//显示3个缓冲区的当前信息
}



static int __init my_init(void)
{
	create_proc_read_entry("mem_test", ...);

	//下面的内容，都需要执行3次
	/* 分配并初始化mem_priv */
	dev = (struct mem_priv *)kzalloc(sizeof(*dev), GFP_KERNEL);
	if (!dev)
		return -ENOMEM;

	/* 分配并初始化缓冲区 */
	dev->buf_size = sizes[...];
	dev->start = (char *)kzalloc(dev->buf_size, GFP_KERNEL);
	if (!dev->start) {
		kfree(dev);
		return -ENOMEM;
	}
	dev->end = dev->start + dev->buf_size;
	dev->wp = 0;

	/* 为设备分配设备号 */
	dev->dev_id = MKDEV(DEF_MAJOR, 10);

	/* 将设备号以及对应的file_operations注册到VFS */
	cdev_init(&dev->mem_cdev, &mem_fops);
	dev->mem_cdev.owner = THIS_MODULE;
	cdev_add(&dev->mem_cdev, dev->dev_id, 1);

	return 0;
}

static void __exit my_exit(void)
{
	remove_proc_entry("mem_test", NULL);

	//下面内容也要执行3次
	cdev_del(&dev->mem_cdev);
	kfree(dev->start);
	kfree(dev);	
}

module_init(my_init);
module_exit(my_exit);

MODULE_AUTHOR("ZHT");
MODULE_LICENSE("GPL");

