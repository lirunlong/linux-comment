/********************************
 * Simple disk driver, just for test
 * for linux 2.6.28.6, support media change
 * Author: zhanght
 * Date: 2011-10-15
 ********************************/

#include <linux/module.h>
#include <linux/sched.h>
#include <linux/fs.h>		/* everything... */
#include <linux/timer.h>
#include <linux/hdreg.h>	/* HDIO_GETGEO */
#include <linux/vmalloc.h>
#include <linux/genhd.h>
#include <linux/blkdev.h>
#include <linux/buffer_head.h>
#include <linux/bio.h>

/* Minor number and partition management */
#define DEV_MINORS		16
#define SECTOR_SIZE		512

/* After 30s, the driver will simulate a media change. */
#define INVALIDATE_DELAY	30*HZ


static int ram_major = 0;	/* dynamic major */
module_param(ram_major, int, 0);
static int hardsect_shift = 9;	/* sector size is 512 bytes */
module_param(hardsect_shift, int, 0);
static int nsectors = 1024;	/* How big the drive is */
module_param(nsectors, int, 0);


/* private struct */
struct ram_dev {
	int size;		/* Device size in sectors */
	u8 *data;		/* The data array */
	short users;	/* How many users */
	short media_change; /* Flag for media change */
	spinlock_t lock;	/* For mutual exclusion */
	struct request_queue *queue;    /* The device request queue */
	struct gendisk *gd;             /* The gendisk structure */
	struct timer_list timer;        /* For simulated media changes */
};

static struct ram_dev *mydev;


/* Handle an I/O request */
static int 
ram_transfer(struct ram_dev *dev, unsigned long sector,	unsigned long nsect, char *buffer, int write)
{
	unsigned long offset = sector * SECTOR_SIZE;
	unsigned long nbytes = nsect * SECTOR_SIZE;

	if ((offset + nbytes) > dev->size) {
		printk (KERN_NOTICE "Beyond-end write (%ld %ld)\n", offset, nbytes);
		return -EIO;
	}

	if (write)
		memcpy(dev->data + offset, buffer, nbytes);
	else
		memcpy(buffer, dev->data + offset, nbytes);

	return 0;
}


/***********************
 * The simple form of the request function.
 ***********************/
static void ram_request(struct request_queue *q)
{
	struct request *req;
//	int res;
//	int block, nsect;
//	char *buf;
	struct ram_dev *dev;

	while ((req = elv_next_request(q))!= NULL) {
		dev = (struct ram_dev *)req->rq_disk->private_data;
		if (!blk_fs_request(req)) {
			printk ("Skip non-fs request\n");
			end_request(req, 0);
			continue;
		}
       	printk ("Req: %s sector %ld, nr %d\n", ((long)rq_data_dir(req)?"write":"read"), (long)req->sector, (int)req->current_nr_sectors);
	
		//block = blk_rq_pos(req) << 9 >> hardsect_shift;
		//nsect = blk_rq_cur_bytes(req) >> hardsect_shift;
		//buf = req->buffer;
		ram_transfer(dev, req->sector, req->current_nr_sectors, req->buffer, rq_data_dir(req));
		end_request(req, 1);
	}
}


/***********************
 * block_device_operations->open
 ***********************/
static int ram_open(struct block_device *bdev, fmode_t mode)
{
	struct ram_dev *dev = bdev->bd_disk->private_data;

	del_timer_sync(&dev->timer);
	spin_lock(&dev->lock);
	if (!dev->users) 
		check_disk_change(bdev);
	dev->users++;
	spin_unlock(&dev->lock);

	return 0;
}


/***********************
 * block_device_operations->open
 ***********************/
static int ram_release(struct gendisk *gd, fmode_t mode)
{
	struct ram_dev *dev = gd->private_data;

	spin_lock(&dev->lock);
	dev->users--;

	if (!dev->users) {
		dev->timer.expires = jiffies + INVALIDATE_DELAY;
		add_timer(&dev->timer);
	}
	spin_unlock(&dev->lock);

	return 0;
}


/***********************
 * block_device_operations->open
 ***********************/
int ram_media_changed(struct gendisk *gd)
{
	struct ram_dev *dev = gd->private_data;
	
	return dev->media_change;
}


/***********************
 * block_device_operations->open
 ***********************/
int ram_revalidate(struct gendisk *gd)
{
	struct ram_dev *dev = gd->private_data;
	
	if (dev->media_change) {
		dev->media_change = 0;
		memset (dev->data, 0, dev->size);
	}
	return 0;
}


/*************************
 * timer_list->func
 * it sets a flag to simulate the removal of the media.
 *************************/
void ram_invalidate(unsigned long ldev)
{
	struct ram_dev *dev = (struct ram_dev *) ldev;

	spin_lock(&dev->lock);
	if (dev->users || !dev->data) 
		printk ("ram: timer sanity check failed\n");
	else
		dev->media_change = 1;
	spin_unlock(&dev->lock);
}


/*
 * The ioctl() implementation
 */

int ram_ioctl(struct block_device *bdev, fmode_t mode,
                 unsigned int cmd, unsigned long arg)
{
	long size;
	struct hd_geometry geo;
	struct ram_dev *dev = bdev->bd_disk->private_data;

	switch(cmd) {
	    case HDIO_GETGEO:
        	/*
		 * Get geometry: since we are a virtual device, we have to make
		 * up something plausible.  So we claim 16 sectors, four heads,
		 * and calculate the corresponding number of cylinders.  We set the
		 * start of data at sector four.
		 */
		size = (dev->size << hardsect_shift) / SECTOR_SIZE;
		geo.cylinders = (size & ~0x3f) >> 6;
		geo.heads = 4;
		geo.sectors = 16;
		geo.start = 4;
		if (copy_to_user((void __user *) arg, &geo, sizeof(geo)))
			return -EFAULT;
		return 0;
	}

	return -ENOTTY; /* unknown command */
}



/*
 * The device operations structure.
 */
static struct block_device_operations ram_ops = {
	.owner           = THIS_MODULE,
	.open 	         = ram_open,
	.release 	 	 = ram_release,
	.media_changed   = ram_media_changed,
	.revalidate_disk = ram_revalidate,
	.ioctl	         = ram_ioctl
};


/*
 * Set up our internal device.
 */
static int setup_device(struct ram_dev *dev)
{
	/*
	 * Get some memory.
	 */
	memset (dev, 0, sizeof(struct ram_dev));
	dev->size = nsectors << hardsect_shift;
	dev->data = vmalloc(dev->size);
	if (dev->data == NULL) {
		printk ("vmalloc failure.\n");
		return -ENOMEM;
	}
	spin_lock_init(&dev->lock);
	
	/*
	 * The timer which "invalidates" the device.
	 */
	init_timer(&dev->timer);
	dev->timer.data = (unsigned long) dev;
	dev->timer.function = ram_invalidate;

	/* alloc & init block_queue */
	dev->queue = blk_init_queue(ram_request, &dev->lock);
	if (dev->queue == NULL)
		goto out_vfree;
	//blk_queue_hardsect_size(dev->queue, hardsect_size);
	dev->queue->queuedata = dev;

	/*
	 * And the gendisk structure.
	 */
	dev->gd = alloc_disk(DEV_MINORS);
	if (! dev->gd) {
		printk (KERN_NOTICE "alloc_disk failure\n");
		goto out_vfree;
	}
	dev->gd->major = ram_major;
	dev->gd->first_minor = 0;
	dev->gd->fops = &ram_ops;
	dev->gd->queue = dev->queue;
	dev->gd->private_data = dev;
	snprintf (dev->gd->disk_name, 32, "ramtest%c", 'a');
	set_capacity(dev->gd, (nsectors << hardsect_shift)/SECTOR_SIZE);
	add_disk(dev->gd);

	return 0;

out_vfree:
	if (dev->data)
		vfree(dev->data);
	return -1;
}



static int __init ram_init(void)
{
	/* register or allocate block device major */
	ram_major = register_blkdev(ram_major, "ram_test");
	if (ram_major <= 0) {
		printk("ram: unable to get major number\n");
		return -EBUSY;
	}

	/* Alloc & init device */
	mydev = kzalloc(sizeof(struct ram_dev), GFP_KERNEL);
	if (mydev == NULL)
		goto out_unregister;
	setup_device(mydev);
    
	return 0;

out_unregister:
	unregister_blkdev(ram_major, "ram_test");
	return -ENOMEM;
}

static void ram_exit(void)
{
	del_timer_sync(&mydev->timer);
	blk_cleanup_queue(mydev->queue);

	if (mydev->gd) {
		del_gendisk(mydev->gd);
		put_disk(mydev->gd);
	}

	if (mydev->data)
		vfree(mydev->data);

	unregister_blkdev(ram_major, "ram_test");
	kfree(mydev);
}
	
module_init(ram_init);
module_exit(ram_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("ZHANG");

