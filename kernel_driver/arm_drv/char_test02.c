/*********************************************************
* support multiply buffer driver example
*
* author:lrl
*
* date: 2013-03-20
*********************************************************/


#include <linux/module.h>
#include <linux/fs.h>
#include <linux/list.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/proc_fs.h>


#define		DEV_NUM			3
#define 	BUF_SIZE		100
#define		DEF_MAJOR		55


/* allocate a private struct for every device */

struct mem_priv{
		char * start,*end;
		int buf_size;
		int wp;
		dev_t dev_id;
		struct cdev	mem_cdev;
};


struct mem_priv * dev[DEV_NUM];

static int sizes[DEV_NUM]={BUF_SIZE,BUF_SIZE,BUF_SIZE};

static int num;
module_param_array(sizes,int,&num,0444);

static int mem_open(struct inode * inode,struct file * filp)
{
		/* judge which buffer to use */

		struct mem_priv * tmp=list_entry(inode->i_cdev,struct mem_priv,mem_cdev);
		filp->private_data=tmp;
		return 0;
}
static int mem_release(struct inode *inode, struct file *filp)
{
		return 0;
}
static ssize_t mem_read(struct file * filp,char __user * buf,size_t count,loff_t * f_pos)
{
		struct mem_priv * dev=filp->private_data;
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
		struct mem_priv * dev=filp->private_data;
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
static int my_proc_read(char *page,char **start,off_t off,int count,int *eof,void *data)
{
		return sprintf(page,"first bufsize %d\n,second bufsize %d\n,third bufsize %d\n",dev[0]->buf_size,dev[1]->buf_size,dev[2]->buf_size);
}
static int __init my_init(void)
{
		/*allocate and initialize mem_priv*/
		int i,j,ret;
		struct proc_dir_entry * filp;
		filp=create_proc_entry("proc_char",0444,NULL);
		if(!filp){
				ret=-1;
				goto creat_proc_entry_err;
		}
		filp->read_proc=my_proc_read;
		for(i=0;i<DEV_NUM;i++){
			dev[i]=kzalloc(sizeof(*(dev[i])),GFP_KERNEL);
			if(!dev[i]){
					ret=-ENOMEM;
					goto dev_err;
			}
			/*allocate and initialize mem_priv's members */
		}
		for(i=0;i<DEV_NUM;i++){
			dev[i]->buf_size=sizes[i];
			dev[i]->start=kzalloc(dev[i]->buf_size,GFP_KERNEL);
			if(!dev[i]->start){
					ret=-ENOMEM;
					goto dev_start_err;
			}
			dev[i]->end=dev[i]->start+dev[i]->buf_size;
			dev[i]->wp=0;
			/* provide num for the device */
			dev[i]->dev_id=MKDEV(DEF_MAJOR,i);
		}
		for(i=0;i<DEV_NUM;i++){
			/* register for file_operatons*/
			cdev_init(&dev[i]->mem_cdev,&mem_fops);
			dev[i]->mem_cdev.owner=THIS_MODULE;
			cdev_add(&dev[i]->mem_cdev,dev[i]->dev_id,1);
		}
		return 0;

		i=DEV_NUM;
dev_start_err:
		for(j=0;j<i;j++){
			kfree(dev[j]->start);
		}
		i=DEV_NUM;	
dev_err:
		for(j=0;j<i;j++){
			kfree(dev[j]);
		}
creat_proc_entry_err:
		return ret;
}
static void __exit my_exit(void)
{
		int i;
		remove_proc_entry("proc_char",NULL);
		for(i=0;i<DEV_NUM;i++){
			cdev_del(&dev[i]->mem_cdev);
			kfree(dev[i]->start);
			kfree(dev[i]);
		}
}
module_init(my_init);
module_exit(my_exit);


MODULE_AUTHOR("LRL");
MODULE_LICENSE("GPL");
