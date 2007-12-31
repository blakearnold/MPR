/*
 * mss_block.c - MMC/SD Card driver (block device driver)
 *
 * Copyright (C) 2007 Intel Corporation
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License Version 2 only 
 * for now as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/*
 * derived from previous mmc code in Linux kernel 
 * Copyright (c) 2002 Hewlett-Packard Company
 * Copyright (c) 2002 Andrew Christian 
 * Copyright (c) 2006 Bridge Wu
 */


#include <linux/module.h>
#include <linux/sched.h>
#include <linux/kernel.h> 
#include <linux/fs.h>     
#include <linux/errno.h> 
#include <linux/types.h> 
#include <linux/fcntl.h> 
#include <linux/hdreg.h> 
#include <linux/init.h>
#include <linux/blkdev.h>
//#include <linux/devfs_fs_kernel.h>
#include <asm/system.h>
#include <asm/uaccess.h>

#include <linux/mmc/mss_core.h>

#define MSS_SHIFT		3
#define MSS_SECTOR_SIZE		(512)

static int major;

struct mss_disk {
	struct request_queue	*queue;
	struct gendisk		*disk;
	struct mss_card 	*card;
	struct class_device	cdev;

	u32			flags;	/* for suspend/resume */
#define MSS_QUEUE_SUSPENDED	(1 << 1)
#define MSS_QUEUE_EXIT		(1 << 0)
	struct completion	thread_complete;
	struct semaphore	thread_sem;
	wait_queue_head_t	thread_wq;
	spinlock_t		request_lock;
	struct scatterlist	*sg;
	struct request		*req;
};

#define MSS_NUM_MINORS	(256 << MSS_SHIFT)

static unsigned long dev_use[MSS_NUM_MINORS/8*sizeof(unsigned long)];

static DECLARE_MUTEX(md_ref_mutex);

static struct mss_disk *mss_disk_get(struct gendisk *disk)
{
	struct mss_disk *md;
       
	down(&md_ref_mutex);
	md = disk->private_data;
	if (md) {
		if (mss_card_get(md->card) == 0)
			class_device_get(&md->cdev);
		else
			md = NULL;
	}
	up(&md_ref_mutex);

	return md;
}

static void mss_disk_put(struct mss_disk *md)
{
	struct mss_card *card = md->card;
	
	down(&md_ref_mutex);
	class_device_put(&md->cdev);
	mss_card_put(card);
	up(&md_ref_mutex);
}

static void mss_disk_release(struct class_device *cdev)
{
	struct mss_disk *md = container_of(cdev, struct mss_disk, cdev);
	struct gendisk *disk = md->disk;

	/* Release the minor number */
	__clear_bit(disk->first_minor >> MSS_SHIFT, dev_use);
	
	put_disk(md->disk);
	
	/* Terminate the request handler thread */
	md->flags |= MSS_QUEUE_EXIT;
	wake_up(&md->thread_wq);
	wait_for_completion(&md->thread_complete);
	
	kfree(md->sg);
	md->sg = NULL;
	
	blk_cleanup_queue(md->queue);
	
	put_device(&md->card->dev);
	md->card = NULL;
	kfree(md);
}

static struct class mss_disk_class = {
	.name		= "mss disk",
	.owner		= THIS_MODULE,
	.release	= mss_disk_release,
	//.class_dev_attrs = mss_disk_attrs,
};

static int mss_media_transfer(struct mss_disk *md, struct request *req)
{
	struct mss_request mreq;
	struct mss_rw_arg marg;
	struct mss_rw_result mres;
	int ret;

	memset(&mreq, 0x0, sizeof(mreq));
	memset(&marg, 0x0, sizeof(marg));
	memset(&mres, 0x0, sizeof(marg));

	mreq.arg = &marg;
	mreq.result = &mres;
	mreq.card = md->card;
	do {
		if (rq_data_dir(req) == READ)
			mreq.action = MSS_READ_MEM;
		else
			mreq.action = MSS_WRITE_MEM;

		marg.nob = req->nr_sectors;
		/* FIXME */
		marg.block_len = MSS_SECTOR_SIZE;
		marg.block = req->sector;
		marg.sg = md->sg;
		marg.sg_len = blk_rq_map_sg(req->q, req, marg.sg);	
	
		ret = mss_send_request(&mreq);
		if (ret)
			goto err;
		dbg5("successful, bytes transfer :%x\n", mres.bytes_xfered);
		ret = end_that_request_chunk(req, 1, mres.bytes_xfered);
		if (!ret) {
			add_disk_randomness(md->disk);
			blkdev_dequeue_request(req);
			end_that_request_last(req, 0);
		}
	} while (ret);

	return 1;
err:
	dbg5("error, bytes transfer :%x\n", mres.bytes_xfered);
	do {
		ret = end_that_request_chunk(req, 0, 
				req->current_nr_sectors << 9);
	} while (ret);
		
	add_disk_randomness(md->disk);
	blkdev_dequeue_request(req);
	end_that_request_last(req, 0);

	return 0;
}

static int mss_queue_thread(void *d)
{
	struct mss_disk *md = d;
	DECLARE_WAITQUEUE(wait, current);

	current->flags |= PF_MEMALLOC;

	daemonize("mmcqd%d", md->disk->first_minor >> MSS_SHIFT);

	complete(&md->thread_complete);

	down(&md->thread_sem);
	add_wait_queue(&md->thread_wq, &wait);
	do {
		struct request *req = NULL;
	

		/* masked by Feng */
		/* try_to_freeze(); */
		spin_lock_irq(&md->request_lock);
		set_current_state(TASK_INTERRUPTIBLE);
		if (!blk_queue_plugged(md->queue))
			md->req = req = elv_next_request(md->queue);
		spin_unlock_irq(&md->request_lock);

		if (!req) {
			if (md->flags & MSS_QUEUE_EXIT)
				break;
			up(&md->thread_sem);
			schedule();
			down(&md->thread_sem);
			continue;
		}
		set_current_state(TASK_RUNNING);

		mss_media_transfer(md, req);
	} while (1);
	remove_wait_queue(&md->thread_wq, &wait);
	up(&md->thread_sem);

	complete_and_exit(&md->thread_complete, 0);
	return 0;
}

static int mss_media_preq(struct request_queue *q, struct request *req)
{
	struct mss_disk *md = q->queuedata;
	if (!md || !md->card || 
		(md->card->state & (MSS_CARD_REMOVING | MSS_CARD_INVALID))) {
		return BLKPREP_KILL;
	}
	return BLKPREP_OK;
}


/**
 *  mss_media_request
 *  @q: request queue
 *
 *  entry function to request handling of MMC/SD block device driver.
 *  handle a request from request queue generated by upper layer.
 */ 
static void mss_media_request(request_queue_t *q)
{
	struct mss_disk *md = q->queuedata;
	
	if (md && !md->req)
		wake_up(&md->thread_wq);
}


static int mss_blk_open(struct inode *inode, struct file *filp)
{
	struct gendisk *disk = inode->i_bdev->bd_disk;
	struct mss_disk *md;

	md = mss_disk_get(disk);
	if (!md)
		return -ENXIO;

	if ((filp->f_mode & FMODE_WRITE) && (md->card->state & MSS_CARD_WP))
		return -EROFS;
	/* FIXME check media change */
	check_disk_change(inode->i_bdev);
	return 0;
}

static int mss_blk_release(struct inode *inode, struct file *filep)
{
	struct gendisk *disk = inode->i_bdev->bd_disk;
	struct mss_disk *md = disk->private_data;

	mss_disk_put(md);
	return 0;
}

static struct block_device_operations mss_bdops = {
	.open			= mss_blk_open,
	.release		= mss_blk_release,
	//.media_changed		= mss_media_changed,
	//.revalidate_disk	= mss_media_revalidate_disk,
	.owner			= THIS_MODULE,
};

/*****************************************************************************
 *
 *   device driver functions
 *
 ****************************************************************************/

/**
 *  mss_card_probe
 *  @dev: device
 *
 *  probe method to initialize block device.
 *  initialize mss_block_device, set capacity, load block driver(add_disk).
 *
 *  invoked by bus_match (invoked by device_register or driver_register)
 *  must have device and driver, or this function cannot be invoked.
 */
static int mss_blk_probe(struct device *dev)
{	
	struct mss_disk *md;
	struct mss_card *card;
	struct mss_host *host;
	int devidx, ret = 0;
	u64 limit = BLK_BOUNCE_HIGH;

	dbg5("read to probe card");
	devidx = find_first_zero_bit(dev_use, MSS_NUM_MINORS);
	if (devidx >= MSS_NUM_MINORS) {
		printk(KERN_ERR "can not find available minors");
		return -ENOSPC;
	}
	__set_bit(devidx, dev_use);
	
	card = container_of(dev, struct mss_card, dev);

	host = card->slot->host;	
	if (card->card_type != MSS_MMC_CARD && card->card_type != MSS_SD_CARD) {
		printk(KERN_ERR "card(slot%d, host%d) is not memory card\n",
			       	card->slot->id, host->id);
		ret = -ENOTBLK;
		goto clear_bit;
	}

	md = kzalloc(sizeof(*md), GFP_KERNEL);
	if (!md) {
		printk(KERN_ERR "card(slot%d, host%d) alloc block_dev failed!"
				"\n", card->slot->id, host->id);
		ret = -ENOMEM;
		goto clear_bit;
	}
	md->card = card;
	md->disk = alloc_disk(1 << MSS_SHIFT);
	if (md->disk == NULL) {
		printk(KERN_ERR "card(slot%d, host%d) alloc disk failed!\n", 
				card->slot->id, host->id);
		ret = -ENOMEM;
		goto free_data;
	}
	md->disk->major = major;
	md->disk->first_minor = devidx << MSS_SHIFT;
	md->disk->fops = &mss_bdops;
	md->disk->driverfs_dev = &card->dev;
	md->disk->private_data = md;
//	sprintf(md->disk->devfs_name, "mssblk%d", devidx);
	sprintf(md->disk->disk_name, "mss/blk%d", devidx);

	class_device_initialize(&md->cdev);
	md->cdev.dev = &card->dev;
	md->cdev.class = &mss_disk_class;
	strncpy(md->cdev.class_id, card->dev.bus_id, BUS_ID_SIZE);
	ret = class_device_add(&md->cdev);
	if (ret) {
		goto free_disk;
	}
	get_device(&card->dev);

	spin_lock_init(&md->request_lock);
	md->queue = blk_init_queue(mss_media_request, &md->request_lock);
	if (!md->queue) {
		ret = -ENOMEM;
		goto remove_cdev;
	}
	if (host->dev->dma_mask && *host->dev->dma_mask)
		limit = *host->dev->dma_mask;

	blk_queue_prep_rq(md->queue, mss_media_preq);	
	blk_queue_bounce_limit(md->queue, limit);
	blk_queue_max_sectors(md->queue, host->max_sectors);
	blk_queue_max_phys_segments(md->queue, host->max_phys_segs);
	blk_queue_max_hw_segments(md->queue, host->max_hw_segs);
	blk_queue_max_segment_size(md->queue, host->max_seg_size);

	md->queue->queuedata = md;
	
	md->sg = kmalloc(sizeof(struct scatterlist) * host->max_phys_segs,
			 GFP_KERNEL);
	if (!md->sg) {
		ret = -ENOMEM;
		goto clean_queue;
	}

	init_completion(&md->thread_complete);
	init_waitqueue_head(&md->thread_wq);
	init_MUTEX(&md->thread_sem);

	ret = kernel_thread(mss_queue_thread, md, CLONE_KERNEL);
	if (ret < 0) {
		goto free_sg;
	}
	wait_for_completion(&md->thread_complete);
	init_completion(&md->thread_complete);

	md->disk->queue = md->queue;
	dev_set_drvdata(dev, md);
	blk_queue_hardsect_size(md->queue, MSS_SECTOR_SIZE);
	set_capacity(md->disk, mss_get_capacity(card)/(MSS_SECTOR_SIZE/512) - 4);
	add_disk(md->disk);
	return 0;

free_sg:
	kfree(md->sg);	
clean_queue:
	blk_cleanup_queue(md->queue);
remove_cdev:
	class_device_del(&md->cdev);
	put_device(&card->dev);
free_disk:
	put_disk(md->disk);
free_data:
	kfree(md);
clear_bit:
	__clear_bit(devidx, dev_use);
	
	return ret;
}

/**
 *  mss_card_remove
 *  @dev: device
 *
 *  remove method to remove block device.
 *  invoked by device_unregister or driver_unregister.
 */
static int mss_blk_remove(struct device *dev)
{
	struct mss_disk *md;
	struct mss_card *card;

	card = container_of(dev, struct mss_card, dev);
	md = dev_get_drvdata(dev);

	class_device_del(&md->cdev);
	del_gendisk(md->disk);
	md->disk->queue = NULL;
	
	down(&md_ref_mutex);
	dev_set_drvdata(dev, NULL);
	class_device_put(&md->cdev);
	up(&md_ref_mutex);
	
	return 0;
}

/**
 *  mss_card_suspend
 *  @dev: device
 *  @state: suspend state
 *  @level: suspend level
 *
 *  card specific suspend.
 *  invoke blk_stop_queue to suspend request queue.
 */
static int mss_blk_suspend(struct device *dev, pm_message_t state)//, u32 level)
{
	struct mss_disk *md;
	struct mss_card *card;
	unsigned long flags;

	card = container_of(dev, struct mss_card, dev);
	md = dev_get_drvdata(dev);

	if (!(md->flags & MSS_QUEUE_SUSPENDED)) {
		md->flags |= MSS_QUEUE_SUSPENDED;
		spin_lock_irqsave(&md->request_lock, flags);
		blk_stop_queue(md->queue);
		spin_unlock_irqrestore(&md->request_lock, flags); 
	}

	return 0;
}

/**
 *  mss_card_resume
 *  @dev: device
 *  @level: suspend level
 *
 *  card specific resume.
 *  invoke blk_start_queue to resume request queue.
 */
static int mss_blk_resume(struct device *dev)//, u32 level)
{
	struct mss_disk *md;
	struct mss_card *card;
	unsigned long flags;

	card = container_of(dev, struct mss_card, dev);
	md = dev_get_drvdata(dev);

	if (md->flags & MSS_QUEUE_SUSPENDED) {
		md->flags &= ~MSS_QUEUE_SUSPENDED;
		spin_lock_irqsave(&md->request_lock, flags);
		blk_start_queue(md->queue);
		spin_unlock_irqrestore(&md->request_lock, flags);
	}

	return 0;
}

static struct mss_driver mss_block_driver = {
	.driver	=	{ 
		.name =	"MMC_SD Block Driver",
		.probe = mss_blk_probe,
		.remove = mss_blk_remove,
		.suspend = mss_blk_suspend,
		.resume = mss_blk_resume,
	},
};

/*****************************************************************************
 *
 *   module init and exit functions
 *
 ****************************************************************************/
static int mss_card_driver_init(void)
{
	int ret = -ENOMEM;

	ret = register_blkdev(major, "mmc");
	if (ret < 0) {
		printk(KERN_ERR "Unable to get major %d for MMC media: %d\n",
		       major, ret);
		return ret;
	}
	if (major == 0)
		major = ret;

	dbg5("Get major number :%d\n", major);
//	devfs_mk_dir("mmc");

	class_register(&mss_disk_class);
	return register_mss_driver(&mss_block_driver);
}

static void mss_card_driver_exit(void)
{
	unregister_mss_driver(&mss_block_driver);
//	devfs_remove("mmc");
	unregister_blkdev(major, "mmc");
	class_unregister(&mss_disk_class);
}


module_init(mss_card_driver_init);
module_exit(mss_card_driver_exit);

MODULE_AUTHOR("Bridge Wu");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Block device driver for MMC/SD card");
