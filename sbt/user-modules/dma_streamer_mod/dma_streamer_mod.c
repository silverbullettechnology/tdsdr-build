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


#define pr_trace(fmt,...) do{ }while(0)
//#define pr_trace pr_debug



static int                 dsm_users;
static int                 dsm_mapped;
static unsigned long       dsm_timeout = 100;
static struct device      *dsm_dev;


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
	const char         *name;
	struct dsm_user    *user;
	struct task_struct *task;

	// transfer specifications
	size_t                 xfer_cnt;
	struct dsm_xfer       *xfer_list;
	struct dsm_chan_buffs *chan_buffs;

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

	size_t                 chan_cnt;
	struct dsm_chan       *chan_list;
	struct dsm_chan_list  *desc_list;
};


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

	cdp->ident   = (unsigned long)chan->device->dev + chan->chan_id;

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
static struct dsm_chan_list *dsm_scan_channels (struct dsm_user *du)
{
	struct dsm_chan_desc *cdp;
	dma_cap_mask_t        mask;
//	int                   i;

	if ( du->desc_list )
		return NULL;

	// first pass: just count channels registered with dmaengine
	du->chan_cnt = 0;
	dma_cap_zero(mask);
	dma_cap_set(DMA_SLAVE | DMA_PRIVATE, mask);
	BUG_ON(dma_request_channel(mask, dsm_scan_channels_size, &du->chan_cnt) != NULL);

	// allocate desc struct in same format exposed to userspace
	du->desc_list = kzalloc(offsetof(struct dsm_chan_list, chan_lst) +
	                       sizeof(struct dsm_chan_desc) * du->chan_cnt,
	                       GFP_KERNEL);

	// allocate channel state list
	du->chan_list = kzalloc(sizeof(struct dsm_chan) * du->chan_cnt, GFP_KERNEL);

	if ( !du->desc_list || !du->chan_list )
	{
		kfree(du->chan_list);
		kfree(du->desc_list);
		du->chan_cnt = 0;
		return NULL;
	}

	// second pass: record particulars of registered dma channels
	du->desc_list->chan_cnt = du->chan_cnt;
	cdp = du->desc_list->chan_lst;
	BUG_ON(dma_request_channel(mask, dsm_scan_channels_desc, &cdp) != NULL);

//	for ( i = 0; i < du->desc_list->chan_cnt; i++ )
//		printk("%2d: ident:%08lx flags:%08lx width:%02u align:%02u driver:%s\n", i,
//		       du->desc_list->chan_lst[i].ident,
//		       du->desc_list->chan_lst[i].flags,
//		       (1 << (du->desc_list->chan_lst[i].width + 3)),
//		       (1 << (du->desc_list->chan_lst[i].align + 3)),
//		       du->desc_list->chan_lst[i].name);

	return du->desc_list;
}

static int dsm_find_ident (struct dsm_user *du, unsigned long ident)
{
	int i;
	for ( i = 0; i < du->desc_list->chan_cnt; i++ )
		if ( du->desc_list->chan_lst[i].ident == ident )
			return i;

	return -1;
}











static void dsm_thread_cb (void *cmp)
{
	pr_debug("completion\n");
	complete(cmp);
}

static int dsm_thread (void *data)
{
#if 0
	struct dsm_chan                *state  = (struct dsm_chan *)data;
	struct dma_chan                *chan   = state->chan;
	struct dma_device              *dev    = chan->device;
	struct dsm_user                *user   = state->user;
	struct dma_async_tx_descriptor *desc;
	struct xilinx_dma_config        xil_conf;
	enum dma_ctrl_flags             flags;
	struct completion               cmp;
	dma_cookie_t                    cookie;
	enum dma_status                 status;
	unsigned long                   timeout;
	unsigned long                   irq_flags;
	struct timespec                 beg, end;
	spinlock_t                      irq_lock;
	int                             ret = 0;

	smp_rmb();
	flags = DMA_CTRL_ACK | DMA_COMPL_SKIP_DEST_UNMAP | DMA_PREP_INTERRUPT;
	spin_lock_init(&irq_lock);
	pr_debug("%s: dsm_thread() starts:\n", state->name);


	state->left = state->words;
	memset(&state->stats, 0, sizeof(struct dsm_xfer_stats));
	while ( state->left > 0 )
	{
		pr_debug("%s: left %lu, start %d\n", state->name, state->left, state->start);

		/* Only one interrupt */
		xil_conf.coalesc = 1;
		xil_conf.delay   = 0;
		dev->device_control(chan, DMA_SLAVE_CONFIG, (unsigned long)&xil_conf);

		// prepare DMA transaction using scatterlist prepared by dsm_state_setup
		desc = dev->device_prep_slave_sg(chan, state->chain[0], state->pages,
		                                 state->dir, flags, NULL);
		if ( !desc )
		{
			pr_err("device_prep_slave_sg() failed, stop\n");
			goto done;
		}
		pr_debug("%s: device_prep_slave_sg() ok\n", state->name);

		// prepare completion and callback and submit descriptor to get cookie
		init_completion(&cmp);
		desc->callback       = dsm_thread_cb;
		desc->callback_param = &cmp;
		cookie = desc->tx_submit(desc);
		if ( dma_submit_error(cookie) )
		{
			pr_err("tx_submit() failed, stop\n");
			goto done;
		}
		pr_debug("%s: tx_submit() ok, cookie %lx\n", state->name, (unsigned long)cookie);

		// in FD, tx thread should start after RX
//		if ( state->dir == DMA_MEM_TO_DEV && state->fd_peer )
//			wait_for_completion(&parent->txrx);

		// experimental: get start time a little earlier, then disable interrupts while
		// starting the FIFO and DMA
		getrawmonotonic(&beg);
		spin_lock_irqsave(&irq_lock, irq_flags);

		// start the DMA and wait for completion
		pr_debug("%s: dma_async_issue_pending()...\n", state->name);
		dma_async_issue_pending(chan);


		// experimental: re-enable interrupts after DMA/FIFO start
		spin_unlock_irqrestore(&irq_lock, irq_flags);

		// in FD, tx thread should start after RX
//		if ( state->dir == DMA_DEV_TO_MEM && state->fd_peer )
//			complete(&parent->txrx);

		timeout = wait_for_completion_timeout(&cmp, dsm_timeout);
		getrawmonotonic(&end);
		status  = dma_async_is_tx_complete(chan, cookie, NULL, NULL);

		// check status
		if ( timeout == 0 )
		{
			pr_warn("DMA timeout, stop\n");
			state->stats.timeouts++;
			goto done;
		}
		else if ( status != DMA_SUCCESS)
		{
			pr_warn("tx got completion callback, but status is \'%s\'\n",
		       	status == DMA_ERROR ? "error" : "in progress");
			state->stats.errors++;
			goto done;
		}
		else
			pr_debug("%s: no timeout, status DMA_SUCCESS\n", state->name);

		state->stats.total  = timespec_add(state->stats.total, timespec_sub(end, beg));
		state->stats.bytes += state->bytes;
		state->stats.completes++;

		pr_debug("%s: left %lu - %lu -> ", state->name, state->left, state->words);
		state->left -= state->words;
		pr_debug("%lu\n", state->left);

		if ( !state->left && atomic_read(&state->continuous) )
		{
			pr_debug("continuous: reset\n");
			state->left = state->words;
		}
	}

done:
	// thread's done, decrement counter and wakeup caller
	atomic_sub(1, &user->busy);
	wake_up_interruptible(&user->wait);
	return ret;
#endif
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

	dma_release_channel(chan->dma_chan);
}


#if 0
static void dump_line (unsigned char *ptr, int len)
{
	int i;

	for ( i = 0; i < len; i++ )
		pr_debug("%02x ", ptr[i]);

	while ( i++ < 16 )
		pr_debug("   ");

	for ( i = 0; i < len; i++ )
		pr_debug("%c", ptr[i] >= 0x20 && ptr[i] < 0x7F ? ptr[i] : '.');

	pr_debug("\n");
}
#endif





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

//	pr_debug("dsm_xfer_setup(rx %d, dma %d, buff.addr %08lx, .size %08lx)\n",
//	         rx, dma, buff->addr, buff->size);

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
		pr_err("failed to kmalloc state struct\n");
		goto free;
	}
	xfer->pages   = us_count;
	xfer->bytes   = us_bytes;
	xfer->words   = us_words;

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

	pr_debug("xdma_filter chan %p -> %08x vs %08x\n", chan, candidate, criterion);
	return candidate == criterion;
}

static int dsm_chan_setup (struct dsm_user *du, struct dsm_chan_buffs *ucb)
{
	struct dsm_xfer_buff *uxb;
	struct dsm_chan_desc *desc;
	struct dsm_chan      *chan;
	struct dsm_xfer      *xfer;
	dma_cap_mask_t        mask;
	unsigned long         ident;
	int                   ret = 0;
	int                   i;

	// map requested ident first to locate dma details
	if ( (i = dsm_find_ident(du, ucb->ident)) < 0 )
	{
		pr_err("dsm_find_ident(%08lx) failed, stop\n", ucb->ident);
		return -EINVAL;
	}
	desc = &du->desc_list->chan_lst[i];

	// check requested channel can operate in requested direction
	if ( !(desc->flags & (ucb->tx ? DSM_CHAN_DIR_TX : DSM_CHAN_DIR_RX)) )
	{
		pr_err("%s xfer, channel %08lx supports { %s%s}, stop\n", 
		       ucb->tx ? "TX" : "RX", ucb->ident,
		       desc->flags & DSM_CHAN_DIR_TX ? "TX " : "",
		       desc->flags & DSM_CHAN_DIR_RX ? "RX " : "");
		return -EINVAL;
	}

	// get DMA device - the xilinx_axidma puts a "device-id" in the top nibble
	dma_cap_zero(mask);
	dma_cap_set(DMA_SLAVE | DMA_PRIVATE, mask);
	chan->dma_chan = dma_request_channel(mask, dsm_chan_setup_find, &ucb->ident);
	if ( !chan->dma_chan )
	{
		pr_err("dma_request_channel(%08lx) failed, stop\n", ident);
		return -ENOSYS;
	}
	pr_debug("dma channel %p\n", chan->dma_chan);

	chan->dma_dir = ucb->tx ? DMA_MEM_TO_DEV : DMA_DEV_TO_MEM;
	chan->name    = kstrdup(ucb->tx ? "tx" : "rx", GFP_KERNEL);


	// default is non-continuous
	atomic_set(&chan->continuous, 0);

	return 0;

release:
	dma_release_channel(chan);
	return ret;
}




/******** Userspace interface ********/

static int dsm_open (struct inode *inode_p, struct file *f)
{
	struct dsm_user *du;
	unsigned long    flags;
	int              ret = -EACCES;

	if ( !dsm_users )
	{
		pr_debug("%s()\n", __func__);

		if ( !(f->private_data = kzalloc(sizeof(struct dsm_user), GFP_KERNEL)) )
			return -ENOMEM;

		// scan once at open, use/return values later
		du = f->private_data;
		if ( !dsm_scan_channels(du) )
		{
			kfree(f->private_data);
			return -ENODEV;
		}

		// other init as needed
		atomic_set(&du->busy, 0);
		init_waitqueue_head(&du->wait);
		dsm_users = 1;
		ret = 0;
	}

	return ret;
}

static void dsm_cleanup (void)
{
#if 0 // TODO: needs to be a loop
	dsm_xfer_cleanup(dsm_adi1_state.tx);
	dsm_xfer_cleanup(dsm_adi1_state.rx);
	dsm_xfer_cleanup(dsm_adi2_state.tx);
	dsm_xfer_cleanup(dsm_adi2_state.rx);
	dsm_xfer_cleanup(dsm_dsx0_state.tx);
	dsm_xfer_cleanup(dsm_dsx0_state.rx);
	dsm_xfer_cleanup(dsm_dsx1_state.tx);
	dsm_xfer_cleanup(dsm_dsx1_state.rx);

	dsm_adi1_state.tx = NULL;
	dsm_adi1_state.rx = NULL;
	dsm_adi2_state.tx = NULL;
	dsm_adi2_state.rx = NULL;
	dsm_dsx0_state.tx = NULL;
	dsm_dsx0_state.rx = NULL;
	dsm_dsx1_state.tx = NULL;
	dsm_dsx1_state.rx = NULL;
#endif

//	atomic_set(&dsm_busy, 0);
	dsm_mapped = 0;
}

static int dsm_start_chan (struct dsm_chan *chan)
{
	static char name[32];

	snprintf(name, sizeof(name), "dsm_%s", chan->name);
	if ( !(chan->task = kthread_run(dsm_thread, chan, name)) )
	{
		pr_err("%s failed to start\n", name);
		return -1;
	}
	atomic_add(1, &chan->user->busy);

	return 0;
}

static int dsm_start_all (int continuous)
{
#if 0 // TODO: needs to be a loop
	int ret = 0;

	// continuous option for TX only
	if ( dsm_adi1_state.tx ) atomic_set(&dsm_adi1_state.tx->continuous, continuous);
	if ( dsm_adi2_state.tx ) atomic_set(&dsm_adi2_state.tx->continuous, continuous);
	if ( dsm_dsx0_state.tx ) atomic_set(&dsm_dsx0_state.tx->continuous, continuous);
	if ( dsm_dsx1_state.tx ) atomic_set(&dsm_dsx1_state.tx->continuous, continuous);

	if ( dsm_start_chan(&dsm_adi1_state) ) ret++;
	if ( dsm_start_chan(&dsm_adi2_state) ) ret++;
	if ( dsm_start_chan(&dsm_dsx0_state) ) ret++;
	if ( dsm_start_chan(&dsm_dsx1_state) ) ret++;

	return ret;
#endif
	return -1;
}

static void dsm_halt_chan (struct dsm_chan *chan)
{
	if ( chan && chan->task )
	{
		kthread_stop(chan->task);
		chan->task = NULL;
	}
}

static void dsm_halt_all (void)
{
#if 0 // TODO: needs to be a loop
	dsm_halt_chan(&dsm_adi1_state);
	dsm_halt_chan(&dsm_adi2_state);
	dsm_halt_chan(&dsm_dsx0_state);
	dsm_halt_chan(&dsm_dsx1_state);
#endif
}

static void dsm_stop_chan (struct dsm_chan *chan)
{
	atomic_set(&chan->continuous, 0);
}

static void dsm_stop_all (void)
{
#if 0 // TODO: needs to be a loop
	dsm_stop_chan(&dsm_adi1_state);
	dsm_stop_chan(&dsm_adi2_state);
	dsm_stop_chan(&dsm_dsx0_state);
	dsm_stop_chan(&dsm_dsx1_state);
#endif
}

static int dsm_wait_all (struct dsm_user *du)
{
	if ( atomic_read(&du->busy) == 0 )
	{
		pr_debug("thread(s) completed already\n");
		return 0;
	}

	// using a waitqueue here rather than a completion to wait for all threads in
	// the full-duplex case
	pr_debug("thread(s) started, wait for completion...\n");
	if ( wait_event_interruptible(du->wait, (atomic_read(&du->busy) == 0) ) )
	{
		pr_warn("interrupted, expect a crash...\n");
		dsm_halt_all();
		return -EINTR;
	}

	pr_debug("thread(s) completed after wait...\n");
	return 0;
}


static inline int dsm_setup (struct dsm_chan *chan, int dma, 
                             const struct dsm_chan_buffs *buff)
{
#if 0
	// no action needed for this channel
	if ( !buff->tx.size && !buff->rx.size )
		return 0;
	
	if ( buff->tx.size && !(chan->tx = dsm_xfer_setup(0, dma, &buff->tx)) )
		return -1;

	if ( buff->rx.size && !(chan->rx = dsm_xfer_setup(1, dma, &buff->rx)) )
		return -1;

	if ( chan->tx ) chan->tx->parent = chan;
	if ( chan->rx ) chan->rx->parent = chan;

	// special setup for FD experiments
	if ( chan->tx && chan->rx )
	{
		chan->tx->fd_peer = chan->rx;
		chan->rx->fd_peer = chan->tx;
	}

#endif
	return 0;
}


static long dsm_ioctl (struct file *f, unsigned int cmd, unsigned long arg)
{
	struct dsm_chan_buffs  cbb;
	struct dsm_chan_buffs *cbp;
	struct dsm_chan_list   clb;
	struct dsm_limits      lim;
	struct dsm_user       *du = f->private_data;
	size_t                 max;
	int                    ret = 0;

	pr_trace("%s(cmd %x, arg %08lx)\n", __func__, cmd, arg);
	BUG_ON(du == NULL);
	BUG_ON( _IOC_TYPE(cmd) != DSM_IOCTL_MAGIC);

	switch ( cmd )
	{
		/* DMA Channel Description: DSM_IOCG_CHANNELS should be called with a pointer to a
		 * buffer to a dsm_chan_list, which it will fill in.  dsm_chan_list.chan_cnt
		 * should be set to the number of dsm_chan_desc structs the dsm_chan_list has room
		 * for before the ioctl call.  The driver code will not write more than chan_cnt
		 * elements into the array, but will set chan_cnt to the actual number available
		 * before returning to the caller. */
		case DSM_IOCG_CHANNELS:
			ret = copy_from_user(&clb, (void *)arg,
			                     offsetof(struct dsm_chan_list, chan_lst));
			if ( ret )
				return -EFAULT;

			max = min(du->desc_list->chan_cnt, clb.chan_cnt);
			ret = copy_to_user((void *)arg, du->desc_list,
			                   offsetof(struct dsm_chan_list, chan_lst) +
			                   sizeof(struct dsm_chan_desc) * max);
			if ( ret )
				return -EFAULT;

			return 0;


		/* DMA Transfer Limits: limitations of the DMA stack for information of the
		 * userspace tools; currently only the limit of DMAable words across all transfers
		 * at once.  Other fields may be added as needed
		 */
		case DSM_IOCG_LIMITS:
			memset(&lim, 0, sizeof(lim));
			lim.total_words = DSM_MAX_SIZE / DSM_BUS_WIDTH;
			ret = copy_to_user((void *)arg, &lim, sizeof(lim));
			return ret;



		/* Set the (userspace) addresses and sizes of the buffers.  These must be
		 * page-aligned (ie allocated with posix_memalign()), locked in with mlock(), and
		 * size a multiple of DSM_BUS_WIDTH.  
		 */
		case DSM_IOCS_MAP_CHAN:
			ret = copy_from_user(&cbb, (void *)arg,
			                     offsetof(struct dsm_chan_buffs, buff_lst));
			if ( ret )
				return -EFAULT;

			cbp = kmalloc(offsetof(struct dsm_chan_buffs, buff_lst) +
			              sizeof(struct dsm_xfer_buff) * cbb.buff_cnt,
			              GFP_KERNEL);
			if ( !cbp )
				return -ENOMEM;

			ret = copy_from_user(&cbp, (void *)arg,
			                     offsetof(struct dsm_chan_buffs, buff_lst) +
			                     sizeof(struct dsm_xfer_buff) * cbb.buff_cnt);
			if ( ret )
			{
				kfree(cbp);
				return -EFAULT;
			}

			if ( dsm_chan_setup(du, cbp) )
			{
				kfree(cbp);
				return -EINVAL;
			}

			return 0;


		// Start a one-shot transaction, after setting up the buffers with a successful
		// DSM_IOCS_MAP.  Returns immediately to the calling process, which should issue
		// DSM_IOCS_ONESHOT_WAIT to wait for the transaction to complete or timeout
		case DSM_IOCS_ONESHOT_START:
			pr_debug("DSM_IOCS_ONESHOT_START\n");
			ret = 0;
			if ( dsm_start_all(0) )
			{
				dsm_halt_all();
				ret = -EBUSY;
			}
			break;

		// Wait for a oneshot transaction started with DSM_IOCS_ONESHOT_START.  The
		// calling process blocks until all transfers are complete, or timeout.
		case DSM_IOCS_ONESHOT_WAIT:
			pr_debug("DSM_IOCS_WAIT\n");
			ret = dsm_wait_all(du);
			break;


		// Start a transaction, after setting up the buffers with a successful
		// DSM_IOCS_MAP. The calling process does not block, which is only really useful
		// with a continuous transfer.  The calling process should stop those with
		// DSM_IOCS_CONTINUOUS_STOP before unmapping the buffers. 
		case DSM_IOCS_CONTINUOUS_START:
			pr_debug("DSM_IOCS_CONTINUOUS_START\n");
			ret = 0;
			if ( dsm_start_all(1) )
			{
				dsm_halt_all();
				ret = -EBUSY;
			}
			break;

		// Stop a running transaction started with DSM_IOCS_CONTINUOUS_START.  The calling
		// process blocks until both transfers are complete, or timeout.
		case DSM_IOCS_CONTINUOUS_STOP:
			pr_debug("DSM_IOCS_CONTINUOUS_STOP\n");
			dsm_stop_all();
			ret = dsm_wait_all(du);
			break;


		// Read statistics from the transfer triggered with DSM_IOCS_TRIGGER
		case DSM_IOCG_STATS:
		{
#if 0
			struct dsm_user_stats dsm_user_stats;
			pr_debug("DSM_IOCG_STATS %08lx\n", arg);

			memset(&dsm_user_stats, 0, sizeof(dsm_user_stats));
			if ( dsm_adi1_state.tx ) dsm_user_stats.adi1.tx = dsm_adi1_state.tx->stats;
			if ( dsm_adi1_state.rx ) dsm_user_stats.adi1.rx = dsm_adi1_state.rx->stats;
			if ( dsm_adi2_state.tx ) dsm_user_stats.adi2.tx = dsm_adi2_state.tx->stats;
			if ( dsm_adi2_state.rx ) dsm_user_stats.adi2.rx = dsm_adi2_state.rx->stats;
			if ( dsm_dsx0_state.tx ) dsm_user_stats.dsx0.tx = dsm_dsx0_state.tx->stats;
			if ( dsm_dsx0_state.rx ) dsm_user_stats.dsx0.rx = dsm_dsx0_state.rx->stats;
			if ( dsm_dsx1_state.tx ) dsm_user_stats.dsx1.tx = dsm_dsx1_state.tx->stats;
			if ( dsm_dsx1_state.rx ) dsm_user_stats.dsx1.rx = dsm_dsx1_state.rx->stats;

			ret = copy_to_user((void *)arg, &dsm_user_stats, sizeof(dsm_user_stats));
			if ( ret )
			{
				pr_err("failed to copy %d bytes, stop\n", ret);
				return -EFAULT;
			}

			ret = 0;
			break;
#endif
			return -ENOSYS;
		}

		// Unmap the buffers buffer mapped with DSM_IOCS_MAP, before DSM_IOCS_TRIGGER.
		case DSM_IOCS_UNMAP_CHAN:
			pr_debug("DSM_IOCS_UNMAP\n");
			dsm_cleanup();
			ret = 0;
			break;

		// Set a timeout in jiffies on the DMA transfer
		case DSM_IOCS_TIMEOUT:
			pr_debug("DSM_IOCS_TIMEOUT %lu\n", arg);
			dsm_timeout = arg;
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
	struct dsm_user *du = f->private_data;
	unsigned long    flags;
	int              ret = -EBADF;

	if ( dsm_users )
	{
		struct dsm_user *du = f->private_data;

		pr_debug("%s()\n", __func__);
		dsm_halt_all();
		dsm_cleanup();

		kfree(du->chan_list);
		kfree(du->desc_list);
		kfree(du);

		dsm_users = 0;

		ret = 0;
	}

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
	dsm_halt_all();
	dsm_cleanup();

	dsm_dev = NULL;
	misc_deregister(&mdev);
}

module_init(dma_streamer_mod_init);
module_exit(dma_streamer_mod_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Morgan Hughes <morgan.hughes@silver-bullet-tech.com>");
MODULE_DESCRIPTION("DMA streaming test module");

