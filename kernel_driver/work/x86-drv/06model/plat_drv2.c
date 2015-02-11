/********************
 * 实现设备文件的自动创建
 * 驱动要实现一个自己的class，然后每匹配一个platform_device，就在class下新生成一个device
 * author: zht
 * date: 2012-08-29
 ********************/

#include <linux/module.h>
#include <linux/sched.h>
#include <linux/fs.h>		/* file_operations */
#include <linux/cdev.h>		/* cdev */
#include <linux/ioport.h>	/* request_mem_region */
#include <linux/slab.h>
#include <linux/io.h>		/* readb/writeb */
#include <linux/uaccess.h>
#include <linux/platform_device.h>

#define DEF_MAJOR 	85

struct pdev_priv {
	unsigned long phy_base;
	void __iomem *vir_base;
	unsigned long phy_size;
	int irq;
	struct cdev mycdev;
	dev_t dev_id;
	struct device *class_dev;
};

static struct class *pclass;


/*
 * file_operations->open
 */
static int 
pdev_open(struct inode *inode, struct file *filp)
{
	struct pdev_priv *tmp = container_of(inode->i_cdev, struct pdev_priv, mycdev);

	filp->private_data = tmp;
	return 0;
}


/* 
 * file_operations->release
 */
static int 
pdev_release(struct inode *inode, struct file *filp)
{
	return 0;
}


/*
 * file_operations->read
 */
static ssize_t
pdev_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
	struct pdev_priv *dev = filp->private_data;
	char *tmp = NULL;

	count = min(count, (size_t)(dev->phy_size - *f_pos));
	if (0 == count)
		return 0;
	
	/* read register */
	tmp = (char *)kzalloc(count, GFP_KERNEL);
	if (NULL == tmp)
		return -ENOMEM;
	memcpy_fromio(tmp, (dev->vir_base + *f_pos), count);

	/* copy to user */
	if (copy_to_user(buf, tmp, count)) {
		kfree(tmp);
		return -EFAULT;
	}

	*f_pos += count;

	return count;
}


static struct file_operations my_fops = {
	.owner 	= THIS_MODULE,
	.open	= pdev_open,
	.release = pdev_release,
	.read	= pdev_read,
};


/* 
 * platform_driver->probe 
 */
static int 
plat_probe(struct platform_device *pdev)
{
	struct pdev_priv *dev;
	int ret;
	struct resource *res1, *res2;
	
	/* get resource from platform_device */
	res1 = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	res2 = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	if (!res1 || !res2)
		printk("Not enough resources.\n");

	/* alloc & init private struct */
	dev = (struct pdev_priv *)kzalloc(sizeof(*dev), GFP_KERNEL);
	if (NULL == dev)
		return -ENOMEM;

	dev->phy_base = res1->start;
	dev->phy_size = res1->end - res1->start + 1;
	if (res2)
		dev->irq = res2->start;

	/* ioremap */
	dev->vir_base = ioremap(dev->phy_base, dev->phy_size);
	if (NULL == dev->vir_base) {
		printk("cannot ioremap: 0x%lx\n", dev->phy_base);
		ret = -EIO;
		goto err;
	}
	printk("ioremap 0x%lx to 0x%p\n", dev->phy_base, dev->vir_base);

	/* register cdev */
	if (pdev->id == -1)
		dev->dev_id = MKDEV(DEF_MAJOR, 0);
	else 
		dev->dev_id = MKDEV(DEF_MAJOR, pdev->id);

	cdev_init(&dev->mycdev, &my_fops);
	dev->mycdev.owner = THIS_MODULE;
	cdev_add(&dev->mycdev, dev->dev_id, 1);

	/* create device file */
	dev->class_dev = device_create(pclass, /* class */
			&pdev->dev,	/* parent */
			dev->dev_id,	/* dev_t */
			NULL,		/* drvdata */
			"shrek-%c", 'a'+MINOR(dev->dev_id)); /* name */
	if (IS_ERR(dev->class_dev)) {
		ret = PTR_ERR(dev->class_dev);
		goto err2;
	}

	/* save pdev_priv to platform_device */
	platform_set_drvdata(pdev, dev);

	return 0;
err2:
	cdev_del(&dev->mycdev);
	iounmap(dev->vir_base);
err:
	kfree(dev);
	return ret;
}


/*
 * platform_driver->remove
 */
static int 
plat_remove(struct platform_device *pdev)
{
	struct pdev_priv *dev = platform_get_drvdata(pdev);

	device_destroy(pclass, dev->dev_id);
	cdev_del(&dev->mycdev);
	iounmap(dev->vir_base);
	kfree(dev);

	return 0;
}


static struct platform_driver pdrv = {
	.probe 	= plat_probe,
	.remove = plat_remove,
	.driver = {
		.name = "shrek-net",
	}
};

static int __init my_init(void)
{
	int ret;
	pclass = class_create(THIS_MODULE, "shrek_io");
	if (IS_ERR(pclass))
		return PTR_ERR(pclass);
	ret = platform_driver_register(&pdrv);
	if (ret) {
		class_destroy(pclass);
		return ret;
	}
	return 0;
}


static void __exit my_exit(void)
{
	/* unregister platform_driver */
	platform_driver_unregister(&pdrv);
	class_destroy(pclass);
}

module_init(my_init);
module_exit(my_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("ZHT");

