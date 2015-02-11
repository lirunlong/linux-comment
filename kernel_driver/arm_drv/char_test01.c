/*********************************************************
* support single buffer driver example
*
* author:lrl
*
* date: 2013-03-20
*********************************************************/


#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/proc_fs.h>


#define 	BUF_SIZE		100
#define		DEF_MAJOR		50


/* allocate a private struct for every device */

struct mem_priv{
		char * start,*end;
		int buf_size;
		int wp;
		dev_t dev_id;
		struct cdev	mem_cdev;
};


struct mem_priv * dev;
extern struct module __this_module;


static int mem_open(struct inode * inode,struct file * filp)
{
		return 0;
}
static int mem_release(struct inode *inode, struct file *filp)
{
		return 0;
}
static ssize_t mem_read(struct file * filp,char __user * buf,size_t count,loff_t * f_pos)
{
		count=min(count,(size_t)(dev->wp-*f_pos));
		if(!count)
				return 0;
		if(copy_to_user(buf,(dev->start+*f_pos),count))
				return -EFAULT;
		*f_pos+=count;
		return count;		
}
static ssize_t mem_write(struct file * filp,const char __user * buf,size_t count,loff_t * f_pos)
{
		count=min(count,(size_t)(dev->buf_size-dev->wp));
		if(!count)
				return 0;
		if(copy_from_user(dev->start+dev->wp,buf,count))
				return -EFAULT;
		dev->wp+=count;
		return count;
}
static struct file_operations mem_fops={
		.owner=THIS_MODULE,
		.open=mem_open,
		.release=mem_release,
		.read=mem_read,
		.write=mem_write,
};

static int __init my_init(void)
{
		/*allocate and initialize mem_priv*/
		dev=kzalloc(sizeof(*dev),GFP_KERNEL);
		if(!dev)
				return -ENOMEM;
		/*allocate and initialize mem_priv's members */
		dev->buf_size=BUF_SIZE;
		dev->start=kzalloc(dev->buf_size,GFP_KERNEL);
		if(!dev->start){
				kfree(dev);
				return -ENOMEM;
		}
		dev->end=dev->start+dev->buf_size;
		dev->wp=0;
		/* provide num for the device */
		dev->dev_id=MKDEV(DEF_MAJOR,10);
		/* register for file_operatons*/
		cdev_init(&dev->mem_cdev,&mem_fops);
		dev->mem_cdev.owner=THIS_MODULE;
		cdev_add(&dev->mem_cdev,dev->dev_id,1);
		return 0;
}
static void __exit my_exit(void)
{
		cdev_del(&dev->mem_cdev);
		kfree(dev->start);
		kfree(dev);
}
module_init(my_init);
module_exit(my_exit);


MODULE_AUTHOR("LRL");
MODULE_LICENSE("GPL");
