/** \file      dma_streamer_mod.c
 *  \brief     Zero-copy DMA with userspace buffers for SDRDC sample data, using Xilinx
 *             AXI-DMA or ADI DMAC driver / PL
 *
 *  \copyright Copyright 2013,2014 Silver Bullet Technologies
 *
 *             This program is free software; you can redistribute it and/or modify it
 *             under the terms of the GNU General Public License as published by the Free
 *             Software Foundation; either version 2 of the License, or (at your option)
 *             any later version.
 *
 *             This program is distributed in the hope that it will be useful, but WITHOUT
 *             ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 *             FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 *             more details.
 *
 * vim:ts=4:noexpandtab
 */
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/mm.h>
#include <linux/ioport.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/sched.h>
#include <linux/uaccess.h>
#include <linux/pagemap.h>
#include <linux/kthread.h>
#include <linux/scatterlist.h>
#include <linux/dmaengine.h>
#include <linux/amba/xilinx_dma.h>

#include "dma_streamer_mod.h"
#include "dsm_private.h"


struct device  *dsm_dev;



size_t dsm_limit_words (int xilinx)
{
	size_t dma;

	// For Xilinx DMA the constraint is the atomic pool for allocating descriptors (must
	// be 64-byte aligned) at one per 4Kbyte page 
	if ( xilinx && (dma = atomic_pool_size()) )
		dma /= 64;	// 64 bytes coherent memory per descriptor / page

	// Otherwise assume flexible descriptors and the limit is based on total pages, assume
	// 7/8 of RAM is available (tested 950MB of 1GB, plus descriptor overhead) and convert
	// to words 
	else
		dma = (totalram_pages / 8) * 7;

	dma *= 512;	// 4Kbytes / 512 words per page
	return dma;
}



/******** Userspace interface ********/

static int dsm_open (struct inode *inode_p, struct file *f)
{
	struct dsm_user *user;
	int              ret = -EACCES;

	pr_debug("%s()\n", __func__);

	if ( !(f->private_data = kzalloc(sizeof(struct dsm_user), GFP_KERNEL)) )
	{
		pr_err("%s: failed to alloc user data\n", __func__);
		return -ENOMEM;
	}

	// scan once at open, use/return values later
	user = f->private_data;
	if ( !dsm_scan_channels(user) )
	{
		pr_err("%s: failed to scan DMA channels\n", __func__);
		kfree(f->private_data);
		return -ENODEV;
	}

	// other init as needed
	user->timeout = HZ;
	atomic_set(&user->busy, 0);
	init_waitqueue_head(&user->wait);
	ret = 0;


	return ret;
}

static void dsm_cleanup (struct dsm_user *user)
{
	int slot;

	for ( slot = 0; slot < user->chan_size; slot++ )
		if ( user->active_mask & (1 << slot) )
		{
			user->active_mask &= ~(1 << slot);
			dsm_chan_cleanup(&user->chan_list[slot]);
		}
}


static int dsm_wait_active (struct dsm_user *user)
{
	if ( atomic_read(&user->busy) == 0 )
	{
		pr_debug("thread(s) completed already\n");
		return 0;
	}

	// using a waitqueue here rather than a completion to wait for all threads
	pr_debug("wait for active threads to complete...\n");
	if ( wait_event_interruptible(user->wait, (atomic_read(&user->busy) == 0) ) )
	{
		pr_warn("interrupted, expect a crash.\n");
		dsm_halt_all(user);
		return -EINTR;
	}

	pr_debug("thread(s) completed after wait.\n");
	return 0;
}


static long dsm_ioctl (struct file *f, unsigned int cmd, unsigned long arg)
{
	struct dsm_user       *user = f->private_data;
	size_t                 max;
	int                    slot;
	int                    ret = 0;

	pr_trace("%s(cmd %x, arg %08lx)\n", __func__, cmd, arg);
	BUG_ON(user == NULL);
	BUG_ON( _IOC_TYPE(cmd) != DSM_IOCTL_MAGIC);

	switch ( cmd )
	{
		/* DMA Channel Description: DSM_IOCG_CHANNELS should be called with a pointer to a
		 * buffer to a dsm_chan_list, which it will fill in.  dsm_chan_list.size
		 * should be set to the number of dsm_chan_desc structs the dsm_chan_list has room
		 * for before the ioctl call.  The driver code will not write more than size
		 * elements into the array, but will set size to the actual number available
		 * before returning to the caller. */
		case DSM_IOCG_CHANNELS:
		{
			struct dsm_chan_list  clb;

			pr_debug("DSM_IOCG_CHANNELS\n");
			ret = copy_from_user(&clb, (void *)arg,
			                     offsetof(struct dsm_chan_list, list));
			if ( ret )
			{
				pr_err("copy_from_user(%p, %p, %zu): %d\n",
				       &clb, (void *)arg, offsetof(struct dsm_chan_list, list), ret);
				return -EFAULT;
			}

			max = min(user->desc_list->size, clb.size);
			ret = copy_to_user((void *)arg, user->desc_list,
			                   offsetof(struct dsm_chan_list, list) +
			                   sizeof(struct dsm_chan_desc) * max);
			if ( ret )
			{
				pr_err("copy_to_user(%p, %p, %zu): %d\n", (void *)arg, user->desc_list,
				       offsetof(struct dsm_chan_list, list) +
				       sizeof(struct dsm_chan_desc) * max, ret);
				return -EFAULT;
			}

			pr_debug("success\n");
			return 0;
		}


		/* DMA Transfer Limits: limitations of the DMA stack for information of the
		 * userspace tools; currently only the limit of DMAable words across all transfers
		 * at once.  Other fields may be added as needed
		 */
		case DSM_IOCG_LIMITS:
		{
			struct dsm_limits  lim;
			struct sysinfo     sib;
			size_t             dma;

			pr_debug("DSM_IOCG_LIMITS\n");

			memset(&lim, 0, sizeof(lim));
			lim.channels    = user->chan_size;
			lim.total_words = dsm_limit_words(user->xilinx_channels);
			pr_debug("Total words %lu (%lu MB) with %sXilinx driver\n",
			         lim.total_words, lim.total_words >> 17,
				     user->xilinx_channels ? "" : "non-");

			ret = copy_to_user((void *)arg, &lim, sizeof(lim));
			if ( ret )
			{
				pr_err("copy_to_user(%p, %p, %zu): %d\n",
				       (void *)arg, &lim, sizeof(lim), ret);
				return -EFAULT;
			}

			pr_debug("success\n");
			return 0;
		}


		/* Set the (userspace) addresses and sizes of the buffers.  These must be
		 * page-aligned (ie allocated with posix_memalign()), locked in with mlock(), and
		 * size a multiple of DSM_BUS_WIDTH.  
		 */
		case DSM_IOCS_MAP_USER:
		{
			struct dsm_user_buffs *ubp;
			struct dsm_user_buffs  ubb;

			pr_debug("DSM_IOCS_MAP_USER\n");
			ret = copy_from_user(&ubb, (void *)arg,
			                     offsetof(struct dsm_user_buffs, list));
			if ( ret )
			{
				pr_err("copy_from_user(%p, %p, %lu): %d\n", &ubb, (void *)arg,
				       offsetof(struct dsm_user_buffs, list), ret);
				return -EFAULT;
			}

			ubp = kzalloc(offsetof(struct dsm_user_buffs, list) +
			              sizeof(struct dsm_user_xfer) * ubb.size,
			              GFP_KERNEL);
			if ( !ubp )
			{
				pr_err("ubp kmalloc(%lu) failed\n",
				       offsetof(struct dsm_user_buffs, list) +
				       sizeof(struct dsm_user_xfer) * ubb.size);
				return -ENOMEM;
			}

			ret = copy_from_user(ubp, (void *)arg,
			                     offsetof(struct dsm_user_buffs, list) +
			                     sizeof(struct dsm_user_xfer) * ubb.size);
			if ( ret )
			{
				pr_err("copy_from_user(%p, %p, %lu): %d\n", &ubp, (void *)arg,
				       offsetof(struct dsm_user_buffs, list) +
				       sizeof(struct dsm_user_xfer) * ubb.size, ret);
				kfree(ubp);
				return -EFAULT;
			}

			if ( dsm_chan_setup(user, ubp) )
			{
				pr_err("dsm_chan_setup(user, ubp) failed\n");
				kfree(ubp);
				return -EINVAL;
			}

			pr_debug("success\n");
			return 0;
		}


		// Start a one-shot transaction, after setting up the buffers with a successful
		// DSM_IOCS_MAP.  Returns immediately to the calling process, which should issue
		// DSM_IOCS_ONESHOT_WAIT to wait for the transaction to complete or timeout
		case DSM_IOCS_ONESHOT_START:
			pr_debug("DSM_IOCS_ONESHOT_START\n");
			ret = 0;
			if ( dsm_start_mask(user, arg, 0) )
			{
				dsm_halt_mask(user, arg);
				ret = -EBUSY;
			}
			break;

		// Wait for a oneshot transaction started with DSM_IOCS_ONESHOT_START.  The
		// calling process blocks until all transfers are complete, or timeout.
		case DSM_IOCS_ONESHOT_WAIT:
			pr_debug("DSM_IOCS_ONESHOT_WAIT\n");
			ret = dsm_wait_active(user);
			break;


		// Start a transaction, after setting up the buffers with a successful
		// DSM_IOCS_MAP. The calling process does not block, which is only really useful
		// with a continuous transfer.  The calling process should stop those with
		// DSM_IOCS_CYCLIC_STOP before unmapping the buffers. 
		case DSM_IOCS_CYCLIC_START:
			pr_debug("DSM_IOCS_CYCLIC_START\n");
			ret = 0;
			if ( dsm_start_mask(user, arg, 1) )
			{
				dsm_halt_mask(user, arg);
				ret = -EBUSY;
			}
			break;

		// Stop a running transaction started with DSM_IOCS_CYCLIC_START.  The calling
		// process blocks until both transfers are complete, or timeout.
		case DSM_IOCS_CYCLIC_STOP:
			pr_debug("DSM_IOCS_CYCLIC_STOP\n");
			dsm_stop_mask(user, arg);
			ret = dsm_wait_active(user);
			break;


		// Read statistics from the transfer triggered with DSM_IOCS_TRIGGER
		case DSM_IOCG_STATS:
		{
			struct dsm_chan_stats  csb;

			pr_debug("DSM_IOCG_STATS\n");
			ret = copy_from_user(&csb, (void *)arg,
			                     offsetof(struct dsm_chan_stats, list));
			if ( ret )
			{
				pr_err("copy_from_user(%p, %p, %zu): %d\n", &csb,
				       (void *)arg, offsetof(struct dsm_chan_stats, list), ret);
				return -EFAULT;
			}

			// set user's buffer size as max, then reset size returned for counting
			// available; available can exceed max to notify user there's more data than
			// the buffer they provided will fit
			max = min(user->desc_list->size, csb.size);
			csb.mask = user->stats_mask;
			csb.size = user->chan_size;
			ret = copy_to_user((void *)arg, &csb,
			                   offsetof(struct dsm_chan_stats, list));
			if ( ret )
			{
				pr_err("copy_to_user(%p, %p, %zu): %d\n", (void *)arg, &csb,
				       offsetof(struct dsm_chan_stats, list), ret);
				return -EFAULT;
			}

			// copy stats for each slot which is active, upto max slots.  note stats
			// follow the same numbering as 

			for ( slot = 0; slot < max; slot++ )
				if ( user->stats_mask & (1 << slot) )
				{
					void *dst = (void *)arg;
					dst += offsetof(struct dsm_chan_stats, list);
					dst += sizeof(struct dsm_xfer_stats) * slot;
					ret = copy_to_user(dst, &user->chan_list[slot].stats,
					                   sizeof(struct dsm_xfer_stats));
					if ( ret )
					{
						pr_err("copy_to_user(%p, %p, %zu): %d\n", dst,
						       &user->chan_list[slot].stats,
						       sizeof(struct dsm_xfer_stats), ret);
						return -EFAULT;
					}
				}

			return 0;
		}


		/* Cleanup the buffers for all active channels, mapped with DSM_IOCS_MAP_USER. */
		case DSM_IOCS_CLEANUP:
			pr_debug("DSM_IOCS_CLEANUP\n");
			dsm_cleanup(user);
			ret = 0;
			break;

		// Set a timeout in jiffies on the DMA transfer
		case DSM_IOCS_TIMEOUT:
			pr_debug("DSM_IOCS_TIMEOUT %lu\n", arg);
			user->timeout = arg;
			ret = 0;
			break;

		default:
			ret = -ENOSYS;
	}

	if ( ret )
		pr_debug("%s(): return %d\n", __func__, ret);

	return ret;
}

static int dsm_release (struct inode *inode_p, struct file *f)
{
	struct dsm_user *user = f->private_data;
	int              ret = -EBADF;

	pr_debug("%s()\n", __func__);
	dsm_halt_all(user);
	dsm_cleanup(user);

	kfree(user->chan_list);
	kfree(user->desc_list);
	kfree(user);

	ret = 0;

	return ret;
}


static struct file_operations fops = 
{
	open:           dsm_open,
	unlocked_ioctl: dsm_ioctl,
	release:        dsm_release,
};

static struct miscdevice mdev = { MISC_DYNAMIC_MINOR, DSM_DRIVER_NODE, &fops };

static int __init dma_streamer_mod_init(void)
{
	int ret = 0;

	if ( misc_register(&mdev) < 0 )
	{
		pr_err("misc_register() failed\n");
		ret = -EIO;
		goto error2;
	}
	dsm_dev = mdev.this_device;

	pr_info("registered successfully\n");
	return 0;

error2:
	return ret;
}

static void __exit dma_streamer_mod_exit(void)
{
	dsm_dev = NULL;
	misc_deregister(&mdev);
}

module_init(dma_streamer_mod_init);
module_exit(dma_streamer_mod_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Morgan Hughes <morgan.hughes@silver-bullet-tech.com>");
MODULE_DESCRIPTION("DMA streaming test module");

