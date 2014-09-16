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


//#define pr_trace(fmt,...) do{ }while(0)
#define pr_trace pr_debug


static struct device  *dsm_dev;


// private per-transfer / per-block struct
struct dsm_xfer
{
	// For repeating / continuous transfers: each repetition transfers chunk_words using
	// repeated DMA transfers.  chunk_left is set to chunk_words when beginning the rep,
	// then debited the correct amount for each transfer.  when chunk_left reaches 0,
	// the repetition is finished.  
	unsigned long  words;
	unsigned long  chunk;
	unsigned long  left;
	unsigned long  bytes;

	// pages counts number of mapped from userspace; chain[] is an array of chained
	// scatterlists (one page each, each holding SG_MAX_SINGLE_ALLOC mappings to the
	// userspace pages)
	int                  pages;
	struct scatterlist  *chain[0];
};


// private per-channel state struct
struct dsm_user;
struct dsm_chan
{
	// fields setup in dsm_scan_channels() regardless whether the channel is in use; 
	unsigned long    slot;
	struct dsm_user *user;

	// fields below this are setup and cleared based on whether a channel is in use.
	const char         *name;
	struct task_struct *task;

	// transfer specifications
	size_t                  xfer_size;
	struct dsm_xfer       **xfer_list;
	struct dsm_chan_buffs  *user_buffs;

	// transfer order control
	size_t    xfer_curr;
	atomic_t  continuous;

	// statistics
	struct dsm_xfer_stats  stats;
	unsigned long long     bytes;

	// DMA engine glue
	enum dma_transfer_direction  dma_dir;
	struct dma_chan             *dma_chan;


	// for single-direction transfers, this is NULL.  for full-duplex transfers, this
	// points to the peer channel on the same controller, to synchronize RX and TX DMA
//	struct dsm_chan *fd_peer;
};


// private per-user state struct
struct dsm_user
{
	// Each thread increments busy at start, decrements it at stop, and wakes the caller
	// through wait.  Caller returns when awoken with busy == 0, all threads done. 
	atomic_t           busy;
	wait_queue_head_t  wait;

	// chan_mask is the bitmask of active transfers
	unsigned long          active_mask;


	size_t                 chan_size;
	struct dsm_chan       *chan_list;
	struct dsm_chan_list  *desc_list;

	unsigned long          stats_mask;

	unsigned long  timeout;
};



static void dsm_thread_cb (void *cmp)
{
	pr_debug("completion\n");
	complete(cmp);
}

static int dsm_thread (void *data)
{
	struct dsm_chan                *chan = (struct dsm_chan *)data;
	struct dsm_user                *user = chan->user;
	struct dsm_xfer                *xfer = chan->xfer_list[0];
	struct dma_device              *dma_dev = chan->dma_chan->device;
	struct dma_async_tx_descriptor *dma_desc;
	enum dma_ctrl_flags             flags;
	struct completion               cmp;
	dma_cookie_t                    cookie;
	enum dma_status                 status;
	unsigned long                   timeout;
	struct timespec                 beg, end;
	int                             ret = 0;

	smp_rmb();
	flags = DMA_CTRL_ACK | DMA_COMPL_SKIP_DEST_UNMAP | DMA_PREP_INTERRUPT;
	pr_debug("%s: dsm_thread() starts:\n", chan->name);

	memset(&chan->stats, 0, sizeof(struct dsm_xfer_stats));
	chan->user->stats_mask |= 1 << chan->slot;

	xfer->left = xfer->words;
	while ( xfer->left > 0 )
	{
		pr_debug("%s: left %lu\n", chan->name, xfer->left);

		// prepare DMA transaction using scatterlist prepared by dsm_state_setup
		// TODO: try this in _xfer_setup()
		dma_desc = dma_dev->device_prep_slave_sg(chan->dma_chan, xfer->chain[0],
		                                         xfer->pages, chan->dma_dir, flags,
		                                         NULL);
		if ( !dma_desc )
		{
			pr_err("device_prep_slave_sg() failed, stop\n");
			goto done;
		}
		pr_debug("%s: device_prep_slave_sg() ok\n", chan->name);

		// prepare completion and callback and submit descriptor to get cookie
		init_completion(&cmp);
		dma_desc->callback       = dsm_thread_cb;
		dma_desc->callback_param = &cmp;
		cookie = dma_desc->tx_submit(dma_desc);
		if ( dma_submit_error(cookie) )
		{
			pr_err("tx_submit() failed, stop\n");
			goto done;
		}
		chan->stats.starts++;
		pr_debug("%s: tx_submit() ok, cookie %lx\n", chan->name, (unsigned long)cookie);

		// in FD, tx thread should start after RX
//		if ( chan->dir == DMA_MEM_TO_DEV && chan->fd_peer )
//			wait_for_completion(&parent->txrx);

		// experimental: get start time a little earlier, then disable interrupts while
		// starting the FIFO and DMA
		getrawmonotonic(&beg);

		// start the DMA and wait for completion
		pr_debug("%s: dma_async_issue_pending()...\n", chan->name);
		dma_async_issue_pending(chan->dma_chan);

		// in FD, tx thread should start after RX
//		if ( chan->dir == DMA_DEV_TO_MEM && chan->fd_peer )
//			complete(&parent->txrx);

		timeout = wait_for_completion_timeout(&cmp, user->timeout);
		getrawmonotonic(&end);
		status  = dma_async_is_tx_complete(chan->dma_chan, cookie, NULL, NULL);

		// check status
		if ( timeout == 0 )
		{
			pr_warn("DMA timeout, stop\n");
			chan->stats.timeouts++;
			goto done;
		}
		else if ( status != DMA_SUCCESS)
		{
			pr_warn("tx got completion callback, but status is \'%s\'\n",
		       	status == DMA_ERROR ? "error" : "in progress");
			chan->stats.errors++;
			goto done;
		}
		else
			pr_debug("%s: no timeout, status DMA_SUCCESS\n", chan->name);

		chan->stats.total  = timespec_add(chan->stats.total, timespec_sub(end, beg));
		chan->stats.bytes += xfer->bytes;
		chan->stats.completes++;

		pr_debug("%s: left %lu - %lu -> ", chan->name, xfer->left, xfer->words);
		xfer->left -= xfer->words;
		pr_debug("%lu\n", xfer->left);

		if ( !xfer->left && atomic_read(&chan->continuous) )
		{
			pr_debug("continuous: reset\n");
			xfer->left = xfer->words;
		}
	}

done:
	// thread's done, decrement counter and wakeup caller
	atomic_sub(1, &user->busy);
	wake_up_interruptible(&user->wait);
	return ret;
}



static void dsm_xfer_cleanup (struct dsm_chan *chan, struct dsm_xfer *xfer)
{
	struct scatterlist *sg;
	struct page        *pg;
	int                 sc;
	int                 pc;

	dma_unmap_sg(dsm_dev, xfer->chain[0], xfer->pages, chan->dma_dir);

	pc = xfer->pages;
	sg = xfer->chain[0];
	sc = pc / (SG_MAX_SINGLE_ALLOC - 1);
	if ( pc % (SG_MAX_SINGLE_ALLOC - 1) )
		sc++;

pr_debug("%s(): pc %d, sg %p, sc %d\n", __func__, pc, sg, sc);

	pr_trace("put_page() on %d user pages:\n", pc);
	while ( pc > 0 )
	{
		pg = sg_page(sg);
		pr_trace("  sg %p -> pg %p\n", sg, pg);
		if ( chan->dma_dir == DMA_DEV_TO_MEM && !PageReserved(pg) )
			set_page_dirty(pg);

		// equivalent to page_cache_release()
		put_page(pg);

		sg = sg_next(sg);
		pc--;
	}

	pr_trace("free_page() on %d sg pages:\n", sc);
	for ( pc = 0; pc < sc; pc++ )
		if ( xfer->chain[pc] )
		{
			pr_trace("  sg %p\n", xfer->chain[pc]);
			free_page((unsigned long)xfer->chain[pc]);
		}
}

static void dsm_chan_cleanup (struct dsm_chan *chan)
{
	int i;

	for ( i = 0; i < chan->xfer_size; i++ )
		dsm_xfer_cleanup(chan, chan->xfer_list[i]);

	kfree(chan->user_buffs);
	chan->user_buffs = NULL;

	kfree(chan->xfer_list);
	chan->xfer_list = NULL;

	dma_release_channel(chan->dma_chan);
	chan->dma_chan = NULL;
}



static struct dsm_xfer *dsm_xfer_setup (struct dsm_chan            *chan,
                                        struct dsm_chan_desc       *desc,
                                        const struct dsm_xfer_buff *buff)
{
	struct dsm_xfer     *xfer;

	unsigned long        us_bytes;
	unsigned long        us_words;
	unsigned long        us_addr;
	struct page        **us_pages;
	int                  us_count;

	struct scatterlist  *sg_walk;
	int                  sg_count;
	int                  sg_index;

	int                  idx;
	int                  ret;

	pr_debug("dsm_xfer_setup(chan %p, desc %s, buff.addr %08lx, .size %08lx, .reps %lu)\n",
	         chan, desc->device, buff->addr, buff->size, buff->reps);

	// address alignment check
	if ( buff->addr & ~PAGE_MASK )
	{
		pr_err("bad page alignment: addr %08lx\n", buff->addr);
		return NULL;
	}
	us_addr = buff->addr;

	// size / granularity check unnecessary now that size is specified in words
	us_bytes = buff->size << desc->width;
	us_words = buff->size;
	pr_debug("%lu bytes / %lu words per DMA\n", us_bytes, us_words);

	// pagelist for looped get_user_pages() to fill
	if ( !(us_pages = (struct page **)__get_free_page(GFP_KERNEL)) )
	{
		pr_err("out of memory getting userspace page list\n");
		return NULL;
	}

	// count userspace pages, the last may be partial
	us_count = us_bytes >> PAGE_SHIFT;
	if ( us_bytes & ~PAGE_MASK )
		us_count++;
	pr_debug("%d user pages\n", us_count);

	// count of scatterlist pages, allowing an extra entry per page for chaining
	sg_count = us_count / (SG_MAX_SINGLE_ALLOC - 1);
	if ( us_count % (SG_MAX_SINGLE_ALLOC - 1) )
		sg_count++;

	// single struct with variable-sized chain[] array following fixed members; each array
	// element is a single page-sized scatterlist
	xfer = kzalloc(offsetof(struct dsm_xfer, chain) +
	                sizeof(struct scatterlist *) * sg_count,
	                GFP_KERNEL);
	if ( !xfer )
	{
		pr_err("failed to kmalloc dsm_xfer\n");
		goto free;
	}
	xfer->pages = us_count;
	xfer->bytes = us_bytes;
	xfer->words = us_words;

	pr_debug("%lu words per DMA, %lu per rep = %lu + %lu reps\n",
	         xfer->words, us_words, us_words / xfer->words, us_words % xfer->words);

	// allocate scatterlists and init
	pr_debug("%d sg pages\n", sg_count);
	for ( idx = 0; idx < sg_count; idx++ )
		if ( !(xfer->chain[idx] = (struct scatterlist *)__get_free_page(GFP_KERNEL)) )
		{
			pr_err("failed to get page for chain[%d]\n", idx);
			goto free;
		}
		else
			sg_init_table(xfer->chain[idx], SG_MAX_SINGLE_ALLOC);

	// chain together before adding pages
	for ( idx = 1; idx < sg_count; idx++ )
		sg_chain(xfer->chain[idx - 1], SG_MAX_SINGLE_ALLOC, xfer->chain[idx]);

	// get_user_pages in blocks and convert to chained scatterlists
	sg_index = 0;;
	sg_walk  = xfer->chain[0];
	while ( us_count )
	{
		int want = min_t(int, us_count, SG_MAX_SINGLE_ALLOC - 1);

		down_read(&current->mm->mmap_sem);
		ret = get_user_pages(current, current->mm, us_addr, want, 
		                     (chan->dma_dir == DMA_DEV_TO_MEM),
		                     0, us_pages, NULL);
		up_read(&current->mm->mmap_sem);
		pr_trace("get_user_pages(..., us_addr %08lx, want %04x, ...) returned %d\n",
		         us_addr, want, ret);

		if ( ret < want )
		{
			pr_err("get_user_pages(): want %d, got %d, stop\n", want, ret);
			goto free;
		}

		for ( idx = 0; idx < ret; idx++ )
		{
			unsigned int len = min_t(unsigned int, us_bytes, PAGE_SIZE);
			sg_set_page(sg_walk, us_pages[idx], len, 0);
			us_bytes -= len;
			if ( us_bytes )
				sg_walk = sg_next(sg_walk);
			else
				sg_mark_end(sg_walk);
		}

		us_addr  += ret << PAGE_SHIFT;
		us_count -= ret;
	}

	// assumes that map_sg uses chain-aware iteration (sg_next/for_each_sg)
	dma_map_sg(dsm_dev, xfer->chain[0], xfer->pages, chan->dma_dir);

	pr_trace("Mapped scatterlist chain:\n");
	for_each_sg(xfer->chain[0], sg_walk, xfer->pages, idx)
		pr_trace("  %d: sg %p -> phys %08lx\n", idx, sg_walk,
		         (unsigned long)sg_dma_address(sg_walk));

	pr_debug("done\n");
	free_page((unsigned long)us_pages);
	return xfer;

free:
	free_page((unsigned long)us_pages);
	for ( idx = 0; idx < sg_count; idx++ )
		if ( xfer->chain[idx] )
			free_page((unsigned long)xfer->chain[idx]);
	kfree(xfer);
	return NULL;
}


static bool dsm_chan_setup_find (struct dma_chan *chan, void *param)
{
	u32  criterion = *(unsigned long *)param;
	u32  candidate = ~criterion;

	candidate = *((u32 *)chan->private);

	pr_trace("%s: chan %p -> %08x vs %08x\n", __func__, chan, candidate, criterion);
	return candidate == criterion;
}

static int dsm_chan_setup (struct dsm_user *user, struct dsm_chan_buffs *ucb)
{
	struct dsm_chan_desc *desc;
	struct dsm_chan      *chan;
	dma_cap_mask_t        mask;
	int                   xfer;

	// validate requested channel slot and that it's not already setup
	if ( ucb->slot > user->chan_size )
	{
		pr_err("bad requested channel %lu, stop\n", ucb->slot);
		return -EINVAL;
	}
	if ( user->active_mask & (1 << ucb->slot) )
	{
		pr_err("channel %lu already active, stop\n", ucb->slot);
		return -EINVAL;
	}

	user->active_mask &= ~(1 << ucb->slot);
	user->stats_mask  &= ~(1 << ucb->slot);
	desc = &user->desc_list->list[ucb->slot];
	chan = &user->chan_list[ucb->slot];
	pr_debug("%s: using %s for slot %lu\n", __func__, desc->device, ucb->slot);

	// temporary limitation
	if ( ucb->size > 1 )
	{
		pr_err("%s multiple buffers not yet supported\n", __func__);
		return -ENOSYS;
	}

	// check requested channel can operate in requested direction
	if ( !(desc->flags & (ucb->tx ? DSM_CHAN_DIR_TX : DSM_CHAN_DIR_RX)) )
	{
		pr_err("%s xfer, channel %lu supports { %s%s}, stop\n", 
		       ucb->tx ? "TX" : "RX", ucb->slot,
		       desc->flags & DSM_CHAN_DIR_TX ? "TX " : "",
		       desc->flags & DSM_CHAN_DIR_RX ? "RX " : "");
		return -EINVAL;
	}
	pr_debug("%s: flags match for %s\n", __func__, ucb->tx ? "TX" : "RX");

	// get DMA device - the xilinx_axidma puts a "device-id" in the top nibble
	dma_cap_zero(mask);
	dma_cap_set(DMA_SLAVE | DMA_PRIVATE, mask);
	chan->dma_chan = dma_request_channel(mask, dsm_chan_setup_find, &desc->private);
	if ( !chan->dma_chan )
	{
		pr_err("dma_request_channel() for slot %lu failed, stop\n", ucb->slot);
		return -ENOSYS;
	}
	pr_debug("dma channel %p\n", chan->dma_chan);

//		/* Only one interrupt */
//		xil_conf.coalesc = 1;
//		xil_conf.delay   = 0;
//		dev->device_control(chan->dma_chan, DMA_SLAVE_CONFIG, (unsigned long)&xil_conf);

	// memory alloc
	chan->xfer_size = ucb->size;
	chan->xfer_list = kzalloc(sizeof(struct dsm_xfer *) * chan->xfer_size, GFP_KERNEL);
	if ( !chan->xfer_list )
	{
		pr_err("%s: failed to alloc xfer_list %zu\n", __func__, 
		       sizeof(struct dsm_xfer *) * chan->xfer_size);
//		kfree(chan->xfer_list);
		chan->xfer_size = 0;
		return -ENOMEM;
	}

	// setup channel 
	chan->dma_dir = ucb->tx ? DMA_MEM_TO_DEV : DMA_DEV_TO_MEM;
	chan->name    = kstrdup(ucb->tx ? "tx" : "rx", GFP_KERNEL);
	atomic_set(&chan->continuous, 0);

	// setup xfer buffs
	for ( xfer = 0; xfer < chan->xfer_size; xfer++ )
	{
		chan->xfer_list[xfer] = dsm_xfer_setup(chan, desc, &ucb->list[xfer]);
		if ( !chan->xfer_list[xfer] )
		{
			pr_err("%s: failed to setup xfer_list[%d]\n", __func__, xfer);
			dsm_chan_cleanup(chan);
			return -ENOMEM;  // dsm_chan_cleanup() frees xfer_list and releases dma_chan
		}
	}

	// reset stats
	chan->bytes = 0;
	memset(&chan->stats, 0, sizeof(struct dsm_xfer_stats));

	// ready to go
	chan->user_buffs = ucb;
	user->active_mask |= 1 << ucb->slot;
	return 0;
}



// callback for first pass: just count channels registered with dmaengine
static bool dsm_scan_channels_size (struct dma_chan *chan, void *param)
{
	if ( chan->device && chan->device->dev && chan->device->dev->driver )
		(*(size_t *)param)++;

	return 0;
}

// callback for second pass: record particulars of registered dma channels
static bool dsm_scan_channels_desc (struct dma_chan *chan, void *param)
{
	struct dma_slave_caps  caps;
	struct dsm_chan_desc **cdpp = param;
	struct dsm_chan_desc  *cdp  = *cdpp;
	int                    ret;

	if ( !chan->device || !chan->device->dev || !chan->device->dev->driver )
		return 0;

	// Linux DMA infrastructure doesn't seem to have the concept of a DMA channel being
	// tied to a particular endpoint, so various drivers check the private pointer.  The
	// two which have been used in the Xilinx AXI-DMA world are to cast the pointer to a
	// u32 or to point it at an actual u32 variable in the driver's internal state...
	// In the case of multiple DMA controllers, we put the ID of the attached endpoint
	// into the top nibble of the private value
	if ( (unsigned long)chan->private > 0x80000000 )
		cdp->private = *(u32 *)chan->private;
	else
		cdp->private = (u32)chan->private;

	// prefer to get the direction capabilities from slave_caps if possible, otherwise
	// infer from the private value
	cdp->flags = 0;
	if ( !(ret = dma_get_slave_caps(chan, &caps)) )
	{
		cdp->flags |= DSM_CHAN_SLAVE_CAPS;
		if ( caps.directions & (1 << DMA_MEM_TO_DEV) ) cdp->flags |= DSM_CHAN_DIR_TX;
		if ( caps.directions & (1 << DMA_DEV_TO_MEM) ) cdp->flags |= DSM_CHAN_DIR_RX;
	}
	else
	{
		switch ( cdp->private & 0xFF )
		{
			case DMA_MEM_TO_DEV:
				cdp->flags |= DSM_CHAN_DIR_TX;
				break;
			case DMA_DEV_TO_MEM:
				cdp->flags |= DSM_CHAN_DIR_RX;
				break;
		}
	}

	if ( dma_has_cap(DMA_MEMCPY,     chan->device->cap_mask) )  cdp->flags |= DSM_CHAN_CAP_MEMCPY;
	if ( dma_has_cap(DMA_XOR,        chan->device->cap_mask) )  cdp->flags |= DSM_CHAN_CAP_XOR;
	if ( dma_has_cap(DMA_PQ,         chan->device->cap_mask) )  cdp->flags |= DSM_CHAN_CAP_PQ;
	if ( dma_has_cap(DMA_XOR_VAL,    chan->device->cap_mask) )  cdp->flags |= DSM_CHAN_CAP_XOR_VAL;
	if ( dma_has_cap(DMA_PQ_VAL,     chan->device->cap_mask) )  cdp->flags |= DSM_CHAN_CAP_PQ_VAL;
	if ( dma_has_cap(DMA_INTERRUPT,  chan->device->cap_mask) )  cdp->flags |= DSM_CHAN_CAP_INTERRUPT;
	if ( dma_has_cap(DMA_SG,         chan->device->cap_mask) )  cdp->flags |= DSM_CHAN_CAP_SG;
	if ( dma_has_cap(DMA_PRIVATE,    chan->device->cap_mask) )  cdp->flags |= DSM_CHAN_CAP_PRIVATE;
	if ( dma_has_cap(DMA_ASYNC_TX,   chan->device->cap_mask) )  cdp->flags |= DSM_CHAN_CAP_ASYNC_TX;
	if ( dma_has_cap(DMA_SLAVE,      chan->device->cap_mask) )  cdp->flags |= DSM_CHAN_CAP_SLAVE;
	if ( dma_has_cap(DMA_CYCLIC,     chan->device->cap_mask) )  cdp->flags |= DSM_CHAN_CAP_CYCLIC;
	if ( dma_has_cap(DMA_INTERLEAVE, chan->device->cap_mask) )  cdp->flags |= DSM_CHAN_CAP_INTERLEAVE;

	cdp->width = chan->device->copy_align;
	cdp->align = chan->device->copy_align;
	strncpy(cdp->device, dev_name(chan->device->dev), sizeof(cdp->device) - 1);
	strncpy(cdp->driver, chan->device->dev->driver->name, sizeof(cdp->driver) - 1);

	(*cdpp)++;
	return 0;
}

// probe channels registered with dmaengine
static struct dsm_chan_list *dsm_scan_channels (struct dsm_user *user)
{
	struct dsm_chan_desc *cdp;
	dma_cap_mask_t        mask;
	int                   slot;

	if ( user->desc_list )
		return NULL;

	// first pass: just count channels registered with dmaengine
	user->chan_size = 0;
	dma_cap_zero(mask);
	dma_cap_set(DMA_SLAVE | DMA_PRIVATE, mask);
	BUG_ON(dma_request_channel(mask, dsm_scan_channels_size, &user->chan_size) != NULL);

	// allocate desc struct in same format exposed to userspace
	user->desc_list = kzalloc(offsetof(struct dsm_chan_list, list) +
	                       sizeof(struct dsm_chan_desc) * user->chan_size,
	                       GFP_KERNEL);

	// allocate channel state list
	user->chan_list = kzalloc(sizeof(struct dsm_chan) * user->chan_size, GFP_KERNEL);

	if ( !user->desc_list || !user->chan_list )
	{
		kfree(user->chan_list);
		kfree(user->desc_list);
		user->chan_size = 0;
		return NULL;
	}

	// second pass: record particulars of registered dma channels
	user->desc_list->size = user->chan_size;
	cdp = user->desc_list->list;
	BUG_ON(dma_request_channel(mask, dsm_scan_channels_desc, &cdp) != NULL);

	// setup permanent fields
	for ( slot = 0; slot < user->desc_list->size; slot++ )
	{
		user->chan_list[slot].user = user;
		user->chan_list[slot].slot = slot;
//		printk("%2d: flags:%08lx width:%02u align:%02u driver:%s\n", slot,
//		       user->desc_list->list[slot].flags,
//		       (1 << (user->desc_list->list[slot].width + 3)),
//		       (1 << (user->desc_list->list[slot].align + 3)),
//		       user->desc_list->list[slot].name);
	}

	return user->desc_list;
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

static int dsm_start_chan (struct dsm_chan *chan)
{
	static char name[32];

	snprintf(name, sizeof(name), "dsm_%s", chan->name);
	pr_debug("%s: name '%s'\n", __func__, name);

	if ( !(chan->task = kthread_run(dsm_thread, chan, name)) )
	{
		pr_err("%s failed to start\n", name);
		return -1;
	}
	pr_debug("%s: atomic_add(1, %p)\n", __func__, &chan->user->busy);
	atomic_add(1, &chan->user->busy);

	return 0;
}

static int dsm_start_mask (struct dsm_user *user, unsigned long mask, int continuous)
{
	int  ret = 0;
	int  slot;

	mask &= user->active_mask;
	pr_debug("%s: mask %08lx\n", __func__, mask);

	if ( continuous )
		for ( slot = 0; slot < user->chan_size; slot++ )
			if ( mask & (1 << slot) && user->chan_list[slot].dma_dir == DMA_MEM_TO_DEV )
			{
				pr_debug("atomic_set(%p, 1)\n", &user->chan_list[slot].continuous);
				atomic_set(&user->chan_list[slot].continuous, 1);
			}

	for ( slot = 0; slot < user->chan_size; slot++ )
		if ( mask & (1 << slot) )
		{
			pr_debug("dsm_start_chan(slot %d)\n", slot);
			if ( dsm_start_chan(&user->chan_list[slot]) )
				ret++;
		}

	return ret;
}


static void dsm_halt_chan (struct dsm_chan *chan)
{
	if ( chan->task )
	{
		kthread_stop(chan->task);
		chan->task = NULL;
	}
}

static void dsm_halt_mask (struct dsm_user *user, unsigned long mask)
{
	int  slot;

	mask &= user->active_mask;

	for ( slot = 0; slot < user->chan_size; slot++ )
		if ( mask & (1 << slot) )
			dsm_halt_chan(&user->chan_list[slot]);
}

static void dsm_halt_all (struct dsm_user *user)
{
	return dsm_halt_mask(user, ~0);
}


static void dsm_stop_chan (struct dsm_chan *chan)
{
	atomic_set(&chan->continuous, 0);
}

static void dsm_stop_mask (struct dsm_user *user, unsigned long mask)
{
	int  slot;

	mask &= user->active_mask;

	for ( slot = 0; slot < user->chan_size; slot++ )
		if ( mask & (1 << slot) )
			dsm_stop_chan(&user->chan_list[slot]);
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
	struct dsm_limits      lim;
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
			pr_debug("DSM_IOCG_LIMITS\n");

			memset(&lim, 0, sizeof(lim));
			lim.channels    = user->chan_size;
			lim.total_words = DSM_MAX_SIZE / DSM_BUS_WIDTH;

			ret = copy_to_user((void *)arg, &lim, sizeof(lim));
			if ( ret )
			{
				pr_err("copy_to_user(%p, %p, %zu): %d\n",
				       (void *)arg, &lim, sizeof(lim), ret);
				return -EFAULT;
			}

			pr_debug("success\n");
			return 0;


		/* Set the (userspace) addresses and sizes of the buffers.  These must be
		 * page-aligned (ie allocated with posix_memalign()), locked in with mlock(), and
		 * size a multiple of DSM_BUS_WIDTH.  
		 */
		case DSM_IOCS_MAP_CHAN:
		{
			struct dsm_chan_buffs *cbp;
			struct dsm_chan_buffs  cbb;

			pr_debug("DSM_IOCS_MAP_CHAN\n");
			ret = copy_from_user(&cbb, (void *)arg,
			                     offsetof(struct dsm_chan_buffs, list));
			if ( ret )
			{
				pr_err("copy_from_user(%p, %p, %lu): %d\n", &cbb, (void *)arg,
				       offsetof(struct dsm_chan_buffs, list), ret);
				return -EFAULT;
			}

			cbp = kzalloc(offsetof(struct dsm_chan_buffs, list) +
			              sizeof(struct dsm_xfer_buff) * cbb.size,
			              GFP_KERNEL);
			if ( !cbp )
			{
				pr_err("cbp kmalloc(%lu) failed\n",
				       offsetof(struct dsm_chan_buffs, list) +
				       sizeof(struct dsm_xfer_buff) * cbb.size);
				return -ENOMEM;
			}

			ret = copy_from_user(cbp, (void *)arg,
			                     offsetof(struct dsm_chan_buffs, list) +
			                     sizeof(struct dsm_xfer_buff) * cbb.size);
			if ( ret )
			{
				pr_err("copy_from_user(%p, %p, %lu): %d\n", &cbp, (void *)arg,
				       offsetof(struct dsm_chan_buffs, list) +
				       sizeof(struct dsm_xfer_buff) * cbb.size, ret);
				kfree(cbp);
				return -EFAULT;
			}

			if ( dsm_chan_setup(user, cbp) )
			{
				pr_err("dsm_chan_setup(user, cbp) failed\n");
				kfree(cbp);
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
		// DSM_IOCS_CONTINUOUS_STOP before unmapping the buffers. 
		case DSM_IOCS_CONTINUOUS_START:
			pr_debug("DSM_IOCS_CONTINUOUS_START\n");
			ret = 0;
			if ( dsm_start_mask(user, arg, 1) )
			{
				dsm_halt_mask(user, arg);
				ret = -EBUSY;
			}
			break;

		// Stop a running transaction started with DSM_IOCS_CONTINUOUS_START.  The calling
		// process blocks until both transfers are complete, or timeout.
		case DSM_IOCS_CONTINUOUS_STOP:
			pr_debug("DSM_IOCS_CONTINUOUS_STOP\n");
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


		// Unmap the buffers buffer mapped with DSM_IOCS_MAP, before DSM_IOCS_TRIGGER.
		case DSM_IOCS_UNMAP:
			pr_debug("DSM_IOCS_UNMAP\n");
			dsm_cleanup(user);
			ret = 0;
			break;

		// Set a timeout in jiffies on the DMA transfer
		case DSM_IOCS_TIMEOUT:
			pr_debug("DSM_IOCS_TIMEOUT %lu\n", arg);
			user->timeout = arg;
			ret = 0;
			break;







		// Target list bitmap from xparameters
#if 0
		case DSM_IOCG_TARGET_LIST:
			reg = 0;
			if ( dsm_dsrc0_regs ) reg |= DSM_TARGT_DSX0;
			if ( dsm_dsnk0_regs ) reg |= DSM_TARGT_DSX0;
			if ( dsm_dsrc1_regs ) reg |= DSM_TARGT_DSX1;
			if ( dsm_dsnk1_regs ) reg |= DSM_TARGT_DSX1;
			if ( dsm_adi1_old_regs ) reg |= DSM_TARGT_ADI1;
			if ( dsm_adi2_old_regs ) reg |= DSM_TARGT_ADI2;
			if ( dsm_adi1_new_regs ) reg |= DSM_TARGT_ADI1|DSM_TARGT_NEW;
			if ( dsm_adi2_new_regs ) reg |= DSM_TARGT_ADI2|DSM_TARGT_NEW;
			pr_debug("DSM_IOCG_TARGET_LIST %08lx { %s%s%s%s%s}\n", reg, 
			         reg & DSM_TARGT_DSX0 ? "dsx0 " : "",
			         reg & DSM_TARGT_DSX1 ? "dsx1 " : "",
			         reg & DSM_TARGT_NEW  ? "new: " : "",
			         reg & DSM_TARGT_ADI1 ? "adi1 " : "",
			         reg & DSM_TARGT_ADI2 ? "adi2 " : "");
			ret = put_user(reg, (unsigned long *)arg);
			break;
#endif

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
//	init_completion(&dsm_adi1_state.txrx);
//	init_completion(&dsm_adi2_state.txrx);
//	init_completion(&dsm_dsx0_state.txrx);
//	init_completion(&dsm_dsx1_state.txrx);

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

