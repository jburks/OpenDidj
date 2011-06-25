/*
 * Sample disk driver, from the beginning.
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>

#include <linux/sched.h>
#include <linux/kernel.h>	/* printk() */
#include <linux/slab.h>		/* kmalloc() */
#include <linux/fs.h>		/* everything... */
#include <linux/errno.h>	/* error codes */
#include <linux/types.h>	/* size_t */
#include <linux/fcntl.h>	/* O_ACCMODE */
#include <linux/hdreg.h>	/* HDIO_GETGEO */
#include <linux/kdev_t.h>
#include <linux/vmalloc.h>
#include <linux/genhd.h>
#include <linux/blkdev.h>
#include <linux/buffer_head.h>	/* invalidate_bdev */
#include <linux/bio.h>
#include <linux/mtd/ubi.h>
#include <linux/err.h>

MODULE_LICENSE("Dual BSD/GPL");

static int ubiblk_major = 61;
module_param(ubiblk_major, int, 0);
static int nsectors = 1024;	/* How big the drive is */
module_param(nsectors, int, 0);
static int ndevices = 1;
module_param(ndevices, int, 0);

/*
 * Minor number and partition management.
 */
#define UBIBLK_MINORS	16
#define MINOR_SHIFT	4
#define DEVNUM(kdevnum)	(MINOR(kdev_t_to_nr(kdevnum)) >> MINOR_SHIFT

/*
 * We can tweak our hardware sector size, but the kernel talks to us
 * in terms of small sectors, always.
 */
#define KERNEL_SECTOR_SIZE	512

/*
 * The internal representation of our device.
 */
struct ubiblk_dev {
        int size; /* Device size in sectors */
        spinlock_t lock;
        struct request_queue *queue; /* The device request queue */
        struct gendisk *gd;
	struct ubi_device_info ubi_dev;
	int hardsect_size;
	struct ubi_volume_desc *ubi_vol;
};

static struct ubiblk_dev *Devices = NULL;

/*
 * Handle an I/O request.
 */
static void ubiblk_transfer(struct ubiblk_dev *dev, unsigned long sector,
		unsigned long nsect, char *buffer, int write)
{
	unsigned long offset = sector*KERNEL_SECTOR_SIZE;
	unsigned long nbytes = nsect*KERNEL_SECTOR_SIZE;

	if((offset + nbytes) > dev->size) {
		printk(KERN_NOTICE "Beyond-end write\n");
		return;
	}

	printk("%s some data.\n", write?"wrote":"read");
}

/*
 * The simple form of the request function.
 */
static void ubiblk_request(request_queue_t *q)
{
	struct request *req;

	while ((req = elv_next_request(q)) != NULL) {
		struct ubiblk_dev *dev = req->rq_disk->private_data;
		if(! blk_fs_request(req)) {
			printk (KERN_NOTICE "Skip non-fs request\n");
			end_request(req, 0);
			continue;
		}
		ubiblk_transfer(dev, req->sector, req->current_nr_sectors,
				req->buffer, rq_data_dir(req));
		end_request(req, 1);
	}
}

static int ubiblk_open(struct inode *inode, struct file *filp)
{
	struct ubiblk_dev *dev = inode->i_bdev->bd_disk->private_data;

	filp->private_data = dev;
	return 0;
}

static int ubiblk_release(struct inode *inode, struct file *filp)
{
	struct ubiblk_dev *dev = inode->i_bdev->bd_disk->private_data;

	return 0;
}

int ubiblk_ioctl (struct inode *inode, struct file *filp,
                 unsigned int cmd, unsigned long arg)
{
	long size;
	struct hd_geometry geo;
	struct ubiblk_dev *dev = filp->private_data;

	switch(cmd) {
	case HDIO_GETGEO:
        	/*
		 * Get geometry: since we are a virtual device, we have to make
		 * up something plausible.  So we claim 16 sectors, four heads,
		 * and calculate the corresponding number of cylinders.  We set the
		 * start of data at sector four.
		 */
		size = dev->size*(dev->hardsect_size/KERNEL_SECTOR_SIZE);
		geo.cylinders = (size & ~0x3f) >> 6;
		geo.heads = 4;
		geo.sectors = 16;
		geo.start = 4;
		printk("Responding with geometry c/h/s %d/%d/%d\n",
		       geo.cylinders, geo.heads, geo.sectors);
		if (copy_to_user((void __user *) arg, &geo, sizeof(geo)))
			return -EFAULT;
		return 0;
	}

	return -ENOTTY; /* unknown command */
}

static int ubiblk_getgeo(struct block_device *bdev, struct hd_geometry *geo)
{
	struct ubiblk_dev *dev = bdev->bd_disk->private_data;
	long size;

	size = dev->size*(dev->hardsect_size/KERNEL_SECTOR_SIZE);
	geo->cylinders = (size & ~0x3f) >> 6;
	geo->heads = 4;
	geo->sectors = 16;
	geo->start = 4;
	printk("Responding with geometry c/h/s %d/%d/%d\n",
	       geo->cylinders, geo->heads, geo->sectors);
	
	return 0;
}


/*
 * The device operations structure.
 */
static struct block_device_operations ubiblk_ops = {
	.owner           = THIS_MODULE,
	.open 	         = ubiblk_open,
	.release 	 = ubiblk_release,
	.ioctl	         = ubiblk_ioctl,
	.getgeo	         = ubiblk_getgeo,
};


/*
 * Set up our internal device.
 */
static void setup_device(struct ubiblk_dev *dev, int which)
{
	memset (dev, 0, sizeof (struct ubiblk_dev));

	spin_lock_init(&dev->lock);

	/* get each ubi device that we're working with.  Each mtd device claimed
	 * by the ubi layer is a device.  Note that this can be a chip or a
	 * partition.
	 */
	if(ubi_get_device_info(which, &dev->ubi_dev)) {
		printk("ubiblk: Failed to get device info for ubi%d\n", which);
		goto out_vfree;
	}

	/* each ubi device is either empty, corrupted, or contains some number
	 * of volumes.  For now, I'm assuming that there's exactly one volume,
	 * and it's volume id is 0.  This is a bit inapproriate, because ubi
	 * volumes are supposed to be like partitions.  The proper solution
	 * would be to allow the ubi userspace tools to create partitions much
	 * like fdisk would.
	 */

	dev->ubi_vol = ubi_open_volume(which, 0, UBI_READWRITE);
	if(IS_ERR(dev->ubi_vol)) {
		printk("Failed to open ubi volume 0 on device %d\n", which);
		dev->ubi_vol = 0;
		goto out_vfree;
	}

	dev->hardsect_size = dev->ubi_dev.min_io_size;
	dev->size = nsectors*dev->hardsect_size;

	/* setup blk dev queuing */
	dev->queue = blk_init_queue(ubiblk_request, &dev->lock);
	if (dev->queue == NULL)
		goto out_vfree;

	blk_queue_hardsect_size(dev->queue, dev->hardsect_size);
	dev->queue->queuedata = dev;

	/* setup gendisk structure */
	dev->gd = alloc_disk(UBIBLK_MINORS);
	if (! dev->gd) {
		printk (KERN_NOTICE "alloc_disk failure\n");
		goto out_vfree;
	}
	dev->gd->major = ubiblk_major;
	dev->gd->first_minor = which*UBIBLK_MINORS;
	dev->gd->fops = &ubiblk_ops;
	dev->gd->queue = dev->queue;
	dev->gd->private_data = dev;
	snprintf (dev->gd->disk_name, 32, "ubiblk%c", which + 'a');
	set_capacity(dev->gd, nsectors*(dev->hardsect_size/KERNEL_SECTOR_SIZE));
	add_disk(dev->gd);
	return;

  out_vfree:
	if (dev->queue)
		blk_cleanup_queue(dev->queue);
}



static int __init ubiblk_init(void)
{
	int i, ret;

	ret = register_blkdev(ubiblk_major, "ubiblk");
	if (ret) {
		printk(KERN_WARNING "ubiblk: unable to get major number\n");
		return -EBUSY;
	}

	Devices = kmalloc(ndevices*sizeof (struct ubiblk_dev), GFP_KERNEL);
	if (Devices == NULL)
		goto out_unregister;
	for (i = 0; i < ndevices; i++) 
		setup_device(Devices + i, i);
    
	return 0;

 out_unregister:
	unregister_blkdev(ubiblk_major, "ubiblk");
	return -ENOMEM;
}

static void ubiblk_exit(void)
{
	int i;

	for (i = 0; i < ndevices; i++) {
		struct ubiblk_dev *dev = Devices + i;
		
		if (dev->gd) {
			del_gendisk(dev->gd);
			put_disk(dev->gd);
		}
		if (dev->queue) {
			blk_cleanup_queue(dev->queue);
		}
		if (dev->ubi_vol) {
			ubi_close_volume(dev->ubi_vol);
		}
	}
	unregister_blkdev(ubiblk_major, "ubiblk");
	kfree(Devices);
}
	
module_init(ubiblk_init);
module_exit(ubiblk_exit);
