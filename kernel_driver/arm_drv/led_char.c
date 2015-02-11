/*****************************************
* led driver
* support four leds on or off use "on " or "off" strings
* author: lrl
* date: 2013-03-22
*******************************************/

#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/ioport.h>
#include <linux/io.h>
#include <linux/proc_fs.h>


#define		LED_NUM		4
#define		LED_MAJOR	61
#define		GPIO_BASE	0x7F008000
#define		GPIO_SIZE	0x1000
#define		GPMCON		0x820
#define		GPMDAT		0x824


/* device private structure */

struct led_priv{
		int led_state;/* on:1,off:0*/
		int	led_num; /* led0~3*/
		dev_t dev_id;/*device num */
		struct cdev led_cdev;
};

struct led_priv * leds[LED_NUM];
static void __iomem * vir_base;


/* proc file */
static int led_proc_read(char * page,char **start,off_t off,int count,int * eof,void * data)
{
		/* print  device num and state  of every led*/
		int ret=0;
		int i;
		for(i=0;i<LED_NUM;i++){
				ret+=sprintf(page+ret,"device id major %d,min %d, state:%d\n",MAJOR(leds[i]->dev_id),MINOR(leds[i]->dev_id),leds[i]->led_state);
		}
		return ret;
}

/* file_operations implement*/
static int led_open(struct inode * inode,struct file * filp)
{
		struct led_priv * tmp=list_entry(inode->i_cdev,struct led_priv,led_cdev);
		filp->private_data=tmp;
		return 0;
}
static int led_release(struct inode * inode,struct file * filp)
{
		return 0;
}
static ssize_t led_write(struct file*filp,const char __user * buf,size_t count, loff_t *fops)
{
		char * tmpbuf;
		unsigned val;
		struct led_priv * l_priv=filp->private_data;
		tmpbuf=kzalloc(count+1,GFP_KERNEL);
		if(copy_from_user(tmpbuf,buf,count))
				return -EFAULT;
		if(!strncmp(tmpbuf,"on",2)){
				val=readl(vir_base+GPMDAT);
				val&=~(1<<l_priv->led_num);
				writel(val,vir_base+GPMDAT);
				l_priv->led_state=1;
		}
		else if(!strncmp(tmpbuf,"off",3)){
				val=readl(vir_base+GPMDAT);
				val|=1<<l_priv->led_num;
				writel(val,vir_base+GPMDAT);
				l_priv->led_state=0;
		}
		kfree(tmpbuf);
		return count;
}


static struct file_operations f_ops={
		.owner=THIS_MODULE,
		.open=led_open,
		.release=led_release,
		.write=led_write,
};



static int __init my_init(void)
{
		int ret;
		int i,j;
		dev_t dev_id;
		unsigned val;
		struct proc_dir_entry * filp;
		dev_id=MKDEV(LED_MAJOR,0);
		ret=register_chrdev_region(dev_id, LED_NUM, "led_drv");
		if(ret){
				ret=-1;
				printk("can't register\n");
				goto register_err;
		}
		vir_base=ioremap(GPIO_BASE,GPIO_SIZE);
		if(!vir_base){
				ret=-EIO;
				goto map_err;
		}
		/* create proc file led_test */
		filp=create_proc_entry("led_test",0444,NULL);
		if(!filp){
				ret=-EFAULT;
				goto create_proc_err;
		}
		filp->read_proc=led_proc_read;
		/* allocate space for every led_priv*/
		for(i=0;i<LED_NUM;i++){
				leds[i]=kzalloc(sizeof(struct led_priv),GFP_KERNEL);
				if(!leds[i]){
						ret=-ENOMEM;
						goto led_priv_alloc_err;
				}
		}
		for(i=0;i<LED_NUM;i++){
				leds[i]->led_state=0;
				leds[i]->led_num=i;
				leds[i]->dev_id=MKDEV(MAJOR(dev_id),i);
				cdev_init(&leds[i]->led_cdev,&f_ops);
				leds[i]->led_cdev.owner=THIS_MODULE;
				cdev_add(&leds[i]->led_cdev,leds[i]->dev_id,1);
		}
		/* config register GPMCON*/
		val=readl(vir_base+GPMCON);
		val&=~0xffff;
		val|=0x1111;
		writel(val,vir_base+GPMCON);
		val=readl(vir_base+GPMDAT);
		val|=0xf;
		writel(val,vir_base+GPMDAT);
		return 0;


		i=LED_NUM;
led_priv_alloc_err:
		for(j=0;j<i;j++){
				kfree(leds[j]);
		}
		remove_proc_entry("led_test",NULL);
create_proc_err:
		iounmap(vir_base);
map_err:
		unregister_chrdev_region(dev_id, LED_NUM);
register_err:
		return ret;
}

static void __exit my_exit(void)
{
		/* free led_priv*/
		int i;
		unregister_chrdev_region(MKDEV(LED_MAJOR,0),LED_NUM);
		iounmap(vir_base);
		remove_proc_entry("led_test",NULL);
		for(i=0;i<LED_NUM;i++){
				kfree(leds[i]);
		}
}


module_init(my_init);
module_exit(my_exit);

MODULE_AUTHOR("LRL");
MODULE_LICENSE("GPL");
