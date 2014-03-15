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
#include "dsm_xparameters.h"


#define ADI_NEW_TX_REG_CNTRL_1  0x0044
#define ADI_NEW_TX_ENABLE       (1 << 0)


static spinlock_t          dsm_lock;
static int                 dsm_users;
static int                 dsm_mapped;
static unsigned long       dsm_timeout = 100;
static struct device      *dsm_dev;

// Full-duplex thread synchronization: the caller starts either or both threads running.
// Each thread increments dsm_busy at start, decrements it at stop, and wakes the caller
// through dsm_wait.  Caller returns when awoken with dsm_busy == 0, all threads done.
static atomic_t            dsm_busy;
static wait_queue_head_t   dsm_wait;


// Per-thread state
struct dsm_chan;
struct dsm_xfer
{
	const char         *name;
	struct task_struct *task;

	// For repeating / continuous transfers: each repetition transfers chunk_words using
	// repeated DMA transfers.  chunk_left is set to chunk_words when beginning the rep,
	// then debited the correct amount for each transfer.  when chunk_left reaches 0,
	// the repetition is finished.  
	unsigned long  words;
	unsigned long  chunk;
	unsigned long  left;
	int            start;
	atomic_t       continuous;

	// DMA engine glue
	enum dma_transfer_direction  dir;
	struct dma_chan             *chan;

	// statistics
	struct dsm_xfer_stats  stats;
	unsigned long          bytes;

	// for single-direction transfers, this is NULL.  for full-duplex transfers, this
	// points to the peer channel on the same controller, to synchronize RX and TX DMA
	struct dsm_xfer *fd_peer;

	// points to the parent dsm_chan struct, currently only used for access to the TX/RX
	// interlock completion
	struct dsm_chan *parent;

	// pages counts number of mapped from userspace; chain[] is an array of chained
	// scatterlists (one page each, each holding SG_MAX_SINGLE_ALLOC mappings to the
	// userspace pages)
	int                 pages;
	struct scatterlist *chain[0];
};

struct dsm_chan
{
	char              *name;
	struct dsm_xfer   *tx;
	struct dsm_xfer   *rx;
	struct completion  txrx;

	// pointer to old and new FIFO control registers
	struct dsm_lvds_regs __iomem *old_regs;
	u32 __iomem                  *new_regs;

	// register value (old) and direction/channel mask (new)
	unsigned long                 ctrl;
};

static struct dsm_chan  dsm_adi1_state = { .name = "adi1" };
static struct dsm_chan  dsm_adi2_state = { .name = "adi2" };
static struct dsm_chan  dsm_dsxx_state = { .name = "dsxx" };


static void dsm_thread_cb (void *cmp)
{
	pr_debug("completion\n");
	complete(cmp);
}

static int dsm_thread (void *data)
{
	struct dsm_xfer                *state  = (struct dsm_xfer *)data;
	struct dma_chan                *chan   = state->chan;
	struct dma_device              *dev    = chan->device;
	struct dsm_chan                *parent = state->parent;
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
	u32                             reg;
	int                             continuous;

	smp_rmb();
	flags = DMA_CTRL_ACK | DMA_COMPL_SKIP_DEST_UNMAP | DMA_PREP_INTERRUPT;
	spin_lock_init(&irq_lock);
	pr_debug("%s: dsm_thread() starts:\n", state->name);
	if ( parent->old_regs )
		pr_debug("\told_regs %p\n", parent->old_regs);
	else if ( parent->new_regs )
		pr_debug("\tnew_regs %p\n", parent->new_regs);
	pr_debug("\tctrl %02lx\n", parent->ctrl);

	// reset TX checksum register
	if ( parent->old_regs && state->dir == DMA_MEM_TO_DEV )
		REG_WRITE(&parent->old_regs->cs_rst, 0xFFFFFFFF);

	continuous = atomic_read(&state->continuous);
	while ( atomic_read(&state->continuous) || state->left > 0 )
	{
		if ( !continuous )
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
		if ( state->dir == DMA_MEM_TO_DEV && state->fd_peer )
			wait_for_completion(&parent->txrx);

		// experimental: get start time a little earlier, then disable interrupts while
		// starting the FIFO and DMA
		getrawmonotonic(&beg);
		spin_lock_irqsave(&irq_lock, irq_flags);

		// start the DMA and wait for completion
		pr_debug("%s: dma_async_issue_pending()...\n", state->name);
		dma_async_issue_pending(chan);

		// the first rep through, start the FIFO controls
		if ( state->start )
		{
			// old TX-only: start FIFO; FD is started by RX thread
			if ( parent->old_regs && state->dir == DMA_MEM_TO_DEV && !state->fd_peer )
			{
				pr_debug("%s: set ADI old ctrl to TX\n", state->name);
				REG_WRITE(&parent->old_regs->ctrl, parent->ctrl);
			}
			// new TX-enabled: enable TX FIFOs (seems to be one bit for both)
			else if ( parent->new_regs && state->dir == DMA_MEM_TO_DEV )
			{
				if ( parent->ctrl & (DSM_NEW_CTRL_TX1 | DSM_NEW_CTRL_TX2) )
				{
					reg = REG_READ(ADI_NEW_RT_ADDR(parent->new_regs,
					               ADI_NEW_TX_REG_CNTRL_1, 1));
					pr_debug("TX: ADI_NEW_TX_REG_CNTRL_1 %08x -> %08x\n",
					         reg, reg | ADI_NEW_TX_ENABLE);
					reg |= ADI_NEW_TX_ENABLE;
					REG_WRITE(ADI_NEW_RT_ADDR(parent->new_regs,
					          ADI_NEW_TX_REG_CNTRL_1, 1), reg);
				}
				else
					pr_debug("No TX: ADI_NEW_TX_REG_CNTRL_1 unchanged\n");
			}
			else
				pr_debug("%s: skip start TX\n", state->name);

			// old RX-only or FD: start FIFO after DMA
			if ( parent->old_regs && state->dir == DMA_DEV_TO_MEM )
			{
				pr_debug("%s: set ADI old ctrl to RX\n", state->name);
				REG_WRITE(&parent->old_regs->ctrl, parent->ctrl);
			}
			// new RX enabled: enable RX FIFOS - seem to be separate, may have to combine 
			// like TX FIFOs above
			else if ( parent->new_regs && state->dir == DMA_DEV_TO_MEM )
			{
				pr_debug("%s: skip new RX ctrl: handled in userspace\n", state->name);
			}
			else
				pr_debug("%s: skip start RX\n", state->name);

			state->start = 0;
		}

		// experimental: re-enable interrupts after DMA/FIFO start
		spin_unlock_irqrestore(&irq_lock, irq_flags);

		// in FD, tx thread should start after RX
		if ( state->dir == DMA_DEV_TO_MEM && state->fd_peer )
			complete(&parent->txrx);

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

		if ( !continuous )
		{
			pr_debug("%s: left %lu - %lu -> ", state->name, state->left, state->words);
			state->left -= state->words;
			pr_debug("%lu\n", state->left);
		}
	}

done:
	// thread's done, decrement counter and wakeup caller
	atomic_sub(1, &dsm_busy);
	wake_up_interruptible(&dsm_wait);
	return ret;
}


static void dsm_xfer_cleanup (struct dsm_xfer *state)
{
	struct scatterlist *sg;
	struct page        *pg;
	int                 sc;
	int                 pc;

	if ( !state )
		return;

	dma_unmap_sg(dsm_dev, state->chain[0], state->pages, state->dir);

	pc = state->pages;
	sg = state->chain[0];
	sc = pc / (SG_MAX_SINGLE_ALLOC - 1);
	if ( pc % (SG_MAX_SINGLE_ALLOC - 1) )
		sc++;

pr_debug("%s(): pc %d, sg %p, sc %d\n", __func__, pc, sg, sc);

//pr_debug("put_page() on %d user pages:\n", pc);
	while ( pc > 0 )
	{
		pg = sg_page(sg);
//pr_debug("  sg %p -> pg %p\n", sg, pg);
		if ( state->dir == DMA_DEV_TO_MEM && !PageReserved(pg) )
			set_page_dirty(pg);

		// equivalent to page_cache_release()
		put_page(pg);

		sg = sg_next(sg);
		pc--;
	}

	dma_release_channel(state->chan);

//pr_debug("free_page() on %d sg pages:\n", sc);
	for ( pc = 0; pc < sc; pc++ )
		if ( state->chain[pc] )
		{
//pr_debug("  sg %p\n", state->chain[pc]);
			free_page((unsigned long)state->chain[pc]);
		}

	kfree(state->name);
	kfree(state);
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


static bool xdma_filter(struct dma_chan *chan, void *param)
{
	u32  criterion = *(u32 *)param;
	u32  candidate = ~criterion;

// Petalinux-supplied AXI DMA driver in 3.6.0 points chan->private at a u32 within struct
// xilinx_dma_chan, which contains the candidate value
#if (LINUX_VERSION_CODE == KERNEL_VERSION(3,6,0))
	candidate = *((u32 *)chan->private);

// ADI-supplied AXI DMA driver in 3.12.0 points sets chan->private to the actual candidate
// value.
#elif (LINUX_VERSION_CODE == KERNEL_VERSION(3,12,0))
	candidate = (u32)chan->private;

// These hacks are specific to particular vendor-supplied kernels: be relatively strict
#else
#error Compiling for unknown kernel, check the setting of *private in your dma_chan
#endif

	pr_debug("xdma_filter chan %p -> %08x vs %08x\n", chan, candidate, criterion);
	return candidate == criterion;
}



//TODO: pass channel number here for AD1/AD2, rip buffs struct down to single set
static struct dsm_xfer *dsm_xfer_setup (int rx, int adi, int dma, 
                                        const struct dsm_xfer_buff *buff)
{
	dma_cap_mask_t       mask;
	struct dma_chan     *chan;
	struct dsm_xfer     *state;

	unsigned long        us_bytes;
	unsigned long        us_addr;
	struct page        **us_pages;
	int                  us_count;

	struct scatterlist  *sg_walk;
	int                  sg_count;
	int                  sg_index;

	int                  idx;
	int                  ret;
	u32                  match;

	pr_debug("dsm_state_setup(rx %d, adi %d, dma %d, buff.addr %08lx, .size %08lx)\n",
	         rx, adi, dma, buff->addr, buff->size);

	// simplify calling logic
	if ( !buff->size )
		return NULL;

	// get DMA device - the xilinx_axidma puts a "device-id" in the top nibble
	dma_cap_zero(mask);
	dma_cap_set(DMA_SLAVE | DMA_PRIVATE, mask);
	match  = rx ? DMA_DEV_TO_MEM : DMA_MEM_TO_DEV;
	match &= 0xFF;
	match |= XILINX_DMA_IP_DMA;
	match |= dma << 28;
	chan = dma_request_channel(mask, xdma_filter, (void *)&match);
	if ( !chan )
	{
		pr_err("dma_request_channel() failed, stop\n");
		return NULL;
	}
	pr_debug("dma channel %p\n", chan);

	// address alignment check
	if ( buff->addr & ~PAGE_MASK )
	{
		pr_err("bad page alignment: addr %08lx\n", buff->addr);
		goto release;
	}
	us_addr = buff->addr;

	// size / granularity check
	pr_debug("size 0x%lx vs align %d\n",
	         buff->size, 1 << chan->device->copy_align);
	if ( buff->size & ((1 << chan->device->copy_align) - 1) )
	{
		pr_err("bad size alignment: size %08lx must be a multiple of %u\n",
		       buff->size, 1 << chan->device->copy_align);
		goto release;
	}
	us_bytes = buff->size;
	pr_debug("%lu bytes per DMA\n", us_bytes);

	// pagelist for looped get_user_pages() to fill
	if ( !(us_pages = (struct page **)__get_free_page(GFP_KERNEL)) )
	{
		pr_err("out of memory getting userspace page list\n");
		goto release;
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
	state = kzalloc(offsetof(struct dsm_xfer, chain) +
	                sizeof(struct scatterlist *) * sg_count,
	                GFP_KERNEL);
	if ( !state )
	{
		pr_err("failed to kmalloc state struct\n");
		goto free;
	}
	state->pages   = us_count;
	state->dir     = rx ? DMA_DEV_TO_MEM : DMA_MEM_TO_DEV;
	state->chan    = chan;
	state->name    = kstrdup(rx ? "rx" : "tx", GFP_KERNEL);
	state->bytes   = us_bytes;
	state->words   = us_bytes >> chan->device->copy_align;
	state->start   = 1;

pr_debug("%lu words per DMA, %lu per rep = %lu + %lu reps\n",
         state->words, buff->words, 
		 buff->words / state->words,
		 buff->words % state->words);

	state->chunk = buff->words;
	if ( state->chunk < buff->words )
		state->chunk = buff->words;
	state->left = state->chunk;

	// default is non-continuous
	atomic_set(&state->continuous, 0);

	// allocate scatterlists and init
	pr_debug("%d sg pages\n", sg_count);
	for ( idx = 0; idx < sg_count; idx++ )
		if ( !(state->chain[idx] = (struct scatterlist *)__get_free_page(GFP_KERNEL)) )
		{
			pr_err("failed to get page for chain[%d]\n", idx);
			goto free;
		}
		else
			sg_init_table(state->chain[idx], SG_MAX_SINGLE_ALLOC);

	// chain together before adding pages
	for ( idx = 1; idx < sg_count; idx++ )
		sg_chain(state->chain[idx - 1], SG_MAX_SINGLE_ALLOC, state->chain[idx]);


	sg_index = 0;;
	sg_walk  = state->chain[0];
	while ( us_count )
	{
		int want = min_t(int, us_count, SG_MAX_SINGLE_ALLOC - 1);

		down_read(&current->mm->mmap_sem);
		ret = get_user_pages(current, current->mm, us_addr, want, rx, 0, us_pages, NULL);
		up_read(&current->mm->mmap_sem);
		pr_debug("get_user_pages(..., us_addr %08lx, want %04x, ...) returned %d\n",
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
	dma_map_sg(dsm_dev, state->chain[0], state->pages, state->dir);

	pr_debug("Mapped scatterlist chain:\n");
	for_each_sg(state->chain[0], sg_walk, state->pages, idx)
		pr_debug("  %d: sg %p -> phys %08lx\n", idx, sg_walk,
		         (unsigned long)sg_dma_address(sg_walk));

	pr_debug("done\n");
	free_page((unsigned long)us_pages);
	return state;

free:
	free_page((unsigned long)us_pages);
	for ( idx = 0; idx < sg_count; idx++ )
		if ( state->chain[idx] )
			free_page((unsigned long)state->chain[idx]);
	kfree(state);
release:
	dma_release_channel(chan);
	return NULL;
}




/******** Userspace interface ********/

static int dsm_open (struct inode *inode_p, struct file *file_p)
{
	unsigned long  flags;
	int            ret = -EACCES;

	spin_lock_irqsave(&dsm_lock, flags);
	if ( !dsm_users )
	{
		pr_debug("%s()\n", __func__);
		// other init as needed
		dsm_users = 1;

		ret = 0;
	}
	spin_unlock_irqrestore(&dsm_lock, flags);

	return ret;
}

static void dsm_cleanup (void)
{
	dsm_xfer_cleanup(dsm_adi1_state.tx);
	dsm_xfer_cleanup(dsm_adi1_state.rx);
	dsm_xfer_cleanup(dsm_adi2_state.tx);
	dsm_xfer_cleanup(dsm_adi2_state.rx);
	dsm_xfer_cleanup(dsm_dsxx_state.tx);
	dsm_xfer_cleanup(dsm_dsxx_state.rx);

	dsm_adi1_state.tx = NULL;
	dsm_adi1_state.rx = NULL;
	dsm_adi2_state.tx = NULL;
	dsm_adi2_state.rx = NULL;
	dsm_dsxx_state.tx = NULL;
	dsm_dsxx_state.rx = NULL;

	atomic_set(&dsm_busy, 0);
	dsm_mapped = 0;
}

static int dsm_start_chan (struct dsm_chan *chan)
{
	static char name[32];

	if ( chan->tx )
	{
		snprintf(name, sizeof(name), "dsm_%s_tx", chan->name);
		if ( !(chan->tx->task = kthread_run(dsm_thread, chan->tx, name)) )
		{
			pr_err("%s failed to start\n", name);
			return -1;
		}

		atomic_add(1, &dsm_busy);
	}

	if ( chan->rx )
	{
		snprintf(name, sizeof(name), "dsm_%s_rx", chan->name);
		if ( !(chan->rx->task = kthread_run(dsm_thread, chan->rx, name)) )
		{
			pr_err("%s failed to start\n", name);
			return -1;
		}

		atomic_add(1, &dsm_busy);
	}

	return 0;
}

static int dsm_start_all (int continuous)
{
	int ret = 0;

	// continuous option for TX only
	if ( dsm_adi1_state.tx ) atomic_set(&dsm_adi1_state.tx->continuous, continuous);
	if ( dsm_adi2_state.tx ) atomic_set(&dsm_adi2_state.tx->continuous, continuous);
	if ( dsm_dsxx_state.tx ) atomic_set(&dsm_dsxx_state.tx->continuous, continuous);

	if ( dsm_start_chan(&dsm_adi1_state) ) ret++;
	if ( dsm_start_chan(&dsm_adi2_state) ) ret++;
	if ( dsm_start_chan(&dsm_dsxx_state) ) ret++;

	return ret;
}

static void dsm_halt_chan (struct dsm_chan *chan)
{
	if ( chan->tx && chan->tx->task )
	{
		kthread_stop(chan->tx->task);
		chan->tx->task = NULL;
	}
	if ( chan->rx && chan->rx->task )
	{
		kthread_stop(chan->rx->task);
		chan->rx->task = NULL;
	}
}

static void dsm_halt_all (void)
{
	dsm_halt_chan(&dsm_adi1_state);
	dsm_halt_chan(&dsm_adi2_state);
	dsm_halt_chan(&dsm_dsxx_state);
}

static void dsm_stop_chan (struct dsm_chan *chan)
{
	if ( chan->tx )
		atomic_set(&chan->tx->continuous, 0);
}

static void dsm_stop_all (void)
{
	dsm_stop_chan(&dsm_adi1_state);
	dsm_stop_chan(&dsm_adi2_state);
	dsm_stop_chan(&dsm_dsxx_state);
}

static int dsm_wait_all (void)
{
	if ( atomic_read(&dsm_busy) == 0 )
	{
		pr_debug("thread(s) completed already\n");
		return 0;
	}

	// using a waitqueue here rather than a completion to wait for all threads in
	// the full-duplex case
	pr_debug("thread(s) started, wait for completion...\n");
	if ( wait_event_interruptible(dsm_wait, (atomic_read(&dsm_busy) == 0) ) )
	{
		pr_warn("interrupted, expect a crash...\n");
		dsm_halt_all();
		return -EINTR;
	}

	pr_debug("thread(s) completed after wait...\n");
	return 0;
}


static inline int dsm_setup (struct dsm_chan *chan, int adi, int dma, 
                             const struct dsm_chan_buffs *buff)
{
	// no action needed for this channel
	if ( !buff->tx.size && !buff->rx.size )
		return 0;
	
	// Checks for channel/direction mask
	if ( !buff->ctrl )
		return -1;

	// Get pointers to register maps for FIFO control
	switch ( adi )
	{
		case DSM_TARGT_ADI1:
			if ( (chan->old_regs = dsm_adi1_old_regs) )
				pr_debug("Transfer to AD1 via old FIFOs\n");
			else if ( (chan->new_regs = dsm_adi1_new_regs) )
				pr_debug("Transfer to AD1 via new FIFOs\n");
			else
			{
				pr_err("AD1 transfer but no FIFO regs, check your USER_MODULES_DMA_STREAMER_MOD_BSP\n");
				return -1;
			}
			break;

		case DSM_TARGT_ADI2:
			if ( (chan->old_regs = dsm_adi2_old_regs) )
				pr_debug("Transfer to AD1 via old FIFOs\n");
			else if ( (chan->new_regs = dsm_adi2_new_regs) )
				pr_debug("Transfer to AD1 via new FIFOs\n");
			else
			{
				pr_err("AD2 transfer but no FIFO regs, check your USER_MODULES_DMA_STREAMER_MOD_BSP\n");
				return -1;
			}
			break;
	}

	if ( buff->tx.size && !(chan->tx = dsm_xfer_setup(0, adi, dma, &buff->tx)) )
		return -1;

	if ( buff->rx.size && !(chan->rx = dsm_xfer_setup(1, adi, dma, &buff->rx)) )
		return -1;

	if ( chan->tx ) chan->tx->parent = chan;
	if ( chan->rx ) chan->rx->parent = chan;

	// special setup for FD experiments
	if ( chan->tx && chan->rx )
	{
		chan->tx->fd_peer = chan->rx;
		chan->rx->fd_peer = chan->tx;
	}

	// FIFO trigger value now passed from userspace
	chan->ctrl = buff->ctrl;

	return 0;
}


static long dsm_ioctl (struct file *filp, unsigned int cmd, unsigned long arg)
{
	unsigned long  reg;
	int            ret = -ENOSYS;
	int            max = 10;

	pr_debug("%s(cmd %x, arg %08lx)\n", __func__, cmd, arg);

	if ( _IOC_TYPE(cmd) != DSM_IOCTL_MAGIC )
		return -EINVAL;

	switch ( cmd )
	{
		// Set the (userspace) addresses and sizes of the buffers.  These must be page-
		// aligned (ie allocated with posix_memalign()), locked in with mlock(), and sized
		// in full pages.  Set either of tx_size or rx_size for a simplex transfer, or
		// both for a full-duplex. tx_reps and rx_reps give the number of times to loop 
		// through the respective buffers, if greater than 1.  Currently (rx_reps *
		// rx_size) and (tx_reps * tx_size) must be equal.  
		case DSM_IOCS_MAP:
		{
			struct dsm_user_buffs dsm_user_buffs;
			pr_debug("DSM_IOCS_MAP %08lx\n", arg);

			if ( dsm_mapped )
			{
				pr_err("pages already mapped; UNMAP or TRIGGER first\n");
				return -EBUSY;
			}

			ret = copy_from_user(&dsm_user_buffs, (void *)arg, sizeof(dsm_user_buffs));
			if ( ret )
			{
				pr_err("failed to copy %d bytes, stop\n", ret);
				return -EFAULT;
			}

			if ( dsm_setup(&dsm_adi1_state, DSM_TARGT_ADI1, 0, &dsm_user_buffs.adi1) ||
			     dsm_setup(&dsm_adi2_state, DSM_TARGT_ADI2, 1, &dsm_user_buffs.adi2) ||
			     dsm_setup(&dsm_dsxx_state, DSM_TARGT_DSXX, 0, &dsm_user_buffs.dsxx) )
			{
				dsm_cleanup();
				return -EINVAL;
			}

			dsm_mapped = 1;
			atomic_set(&dsm_busy, 0);
			pr_debug("setup buffers: { %s%s%s%s%s%s}, ok\n", 
			         dsm_adi1_state.tx ? "adi1_tx " : "",
			         dsm_adi1_state.rx ? "adi1_rx " : "",
			         dsm_adi2_state.tx ? "adi2_tx " : "",
			         dsm_adi2_state.rx ? "adi2_rx " : "",
			         dsm_dsxx_state.tx ? "dsxx_tx " : "",
			         dsm_dsxx_state.rx ? "dsxx_rx " : "");

			ret = 0;
			break;
		}


		// Trigger a transaction, after setting up the buffers with a successful
		// DSM_IOCS_MAP.  If the mapped buffers have a nonzero tx_size, a TX transfer is
		// started.  If the mapped buffers have a nonzero rx_size, a RX transfer is
		// started.  The two directions may be run in parallel.  The calling process will
		// block until both transfers are complete, or a timeout occurs. 
		case  DSM_IOCS_TRIGGER:
			pr_debug("DSM_IOCS_TRIGGER\n");

			if ( dsm_start_all(0) )
			{
				dsm_halt_all();
				ret = -EBUSY;
			}
			else
				ret = dsm_wait_all();

			break;


		// Start a transaction, after setting up the buffers with a successful
		// DSM_IOCS_MAP. The calling process does not block, which is only really useful
		// with a continuous transfer.  The calling process should stop those with
		// DSM_IOCS_STOP before unmapping the buffers. 
		case DSM_IOCS_START:
			pr_debug("DSM_IOCS_START\n");
			ret = 0;

			// continuous mode not supported with old PL
			if ( dsm_adi1_old_regs )
				ret = -EBUSY;

			else if ( dsm_start_all(1) )
			{
				dsm_halt_all();
				ret = -EBUSY;
			}
			break;

		// Stop a running transaction started with DSM_IOCS_START.  The calling process
		// will block until both transfers are complete, or a timeout occurs.
		case DSM_IOCS_STOP:
			pr_debug("DSM_IOCS_STOP\n");

			dsm_stop_all();
			ret = dsm_wait_all();
			break;


		case  DSM_IOCG_FIFO_CNT:
		{
			struct dsm_fifo_counts buff;
			pr_debug("DSM_IOCG_FIFO_CNT\n");

			memset(&buff, 0xFF, sizeof(buff));
			if ( dsm_rx_fifo1_cnt )
			{
				buff.rx_1_ins = REG_READ(dsm_rx_fifo1_cnt);
				buff.rx_1_ext = REG_READ(&dsm_rx_fifo1_cnt[1]);
			}
			if ( dsm_rx_fifo2_cnt )
			{
				buff.rx_2_ins = REG_READ(dsm_rx_fifo2_cnt);
				buff.rx_2_ext = REG_READ(&dsm_rx_fifo2_cnt[1]);
			}
			if ( dsm_tx_fifo1_cnt )
			{
				buff.tx_1_ins = REG_READ(dsm_tx_fifo1_cnt);
				buff.tx_1_ext = REG_READ(&dsm_tx_fifo1_cnt[1]);
			}
			if ( dsm_tx_fifo2_cnt )
			{
				buff.tx_2_ins = REG_READ(dsm_tx_fifo2_cnt);
				buff.tx_2_ext = REG_READ(&dsm_tx_fifo2_cnt[1]);
			}

			ret = copy_to_user((void *)arg, &buff, sizeof(buff));
			break;
		}


		case DSM_IOCG_ADI1_OLD_CLK_CNT:
			pr_debug("DSM_IOCG_CLK_CNT\n");
			if ( dsm_adi1_old_regs )
			{
				reg = REG_READ(&dsm_adi1_old_regs->cs_rst);
				ret = arg ? put_user(reg, (unsigned long *)arg) : 0;
			}
			else
				printk ("dsm_adi1_old_regs NULL, no clk\n");
			break;

		case DSM_IOCG_ADI2_OLD_CLK_CNT:
			pr_debug("DSM_IOCG_CLK_CNT\n");
			if ( dsm_adi2_old_regs )
			{
				reg = REG_READ(&dsm_adi2_old_regs->cs_rst);
				ret = arg ? put_user(reg, (unsigned long *)arg) : 0;
			}
			else
				printk ("dsm_adi2_old_regs NULL, no clk\n");
			break;


		// Read statistics from the transfer triggered with DSM_IOCS_TRIGGER
		case DSM_IOCG_STATS:
		{
			struct dsm_user_stats dsm_user_stats;
			pr_debug("DSM_IOCG_STATS %08lx\n", arg);

			memset(&dsm_user_stats, 0, sizeof(dsm_user_stats));
			if ( dsm_adi1_state.tx ) dsm_user_stats.adi1.tx = dsm_adi1_state.tx->stats;
			if ( dsm_adi1_state.rx ) dsm_user_stats.adi1.rx = dsm_adi1_state.rx->stats;
			if ( dsm_adi2_state.tx ) dsm_user_stats.adi2.tx = dsm_adi2_state.tx->stats;
			if ( dsm_adi2_state.rx ) dsm_user_stats.adi2.rx = dsm_adi2_state.rx->stats;
			if ( dsm_dsxx_state.tx ) dsm_user_stats.dsxx.tx = dsm_dsxx_state.tx->stats;
			if ( dsm_dsxx_state.rx ) dsm_user_stats.dsxx.rx = dsm_dsxx_state.rx->stats;

			ret = copy_to_user((void *)arg, &dsm_user_stats, sizeof(dsm_user_stats));
			if ( ret )
			{
				pr_err("failed to copy %d bytes, stop\n", ret);
				return -EFAULT;
			}

			ret = 0;
			break;
		}

		// Unmap the buffers buffer mapped with DSM_IOCS_MAP, before DSM_IOCS_TRIGGER.
		case DSM_IOCS_UNMAP:
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


		// Access to data source regs
		case DSM_IOCS_DSRC_CTRL:
			if ( dsm_dsrc_regs )
			{
				pr_debug("DSM_IOCS_DSRC_CTRL %08lx\n", arg);
				REG_WRITE(&dsm_dsrc_regs->ctrl, arg);
				ret = 0;
			}
			else
				ret = -ENODEV;
			break;

		case DSM_IOCG_DSRC_STAT:
			if ( dsm_dsrc_regs )
			{
				reg = REG_READ(&dsm_dsrc_regs->ctrl);
				pr_debug("DSM_IOCG_DSRC_STAT %08lx\n", reg);
				ret = put_user(reg, (unsigned long *)arg);
			}
			else
				ret = -ENODEV;
			break;

		case DSM_IOCG_DSRC_BYTES:
			if ( dsm_dsrc_regs )
			{
				reg = REG_READ(&dsm_dsrc_regs->bytes);
				pr_debug("DSM_IOCG_DSRC_BYTES %08lx\n", reg);
				ret = put_user(reg, (unsigned long *)arg);
			}
			else
				ret = -ENODEV;
			break;

		case DSM_IOCS_DSRC_BYTES:
			if ( dsm_dsrc_regs )
			{
				pr_debug("DSM_IOCS_DSRC_BYTES %08lx\n", arg);
				REG_WRITE(&dsm_dsrc_regs->bytes, arg);
				ret = 0;
			}
			else
				ret = -ENODEV;
			break;

		case DSM_IOCG_DSRC_SENT:
			if ( dsm_dsrc_regs )
			{
				pr_debug("DSM_IOCG_DSRC_SENT %08lx\n", arg);
				reg = REG_READ(&dsm_dsrc_regs->sent);
				ret = put_user(reg, (unsigned long *)arg);
			}
			else
				ret = -ENODEV;
			break;

		case DSM_IOCG_DSRC_TYPE:
			if ( dsm_dsrc_regs )
			{
				reg = REG_READ(&dsm_dsrc_regs->type);
				pr_debug("DSM_IOCG_DSRC_TYPE %08lx\n", reg);
				ret = put_user(reg, (unsigned long *)arg);
			}
			else
				ret = -ENODEV;
			break;

		case DSM_IOCS_DSRC_TYPE:
			if ( dsm_dsrc_regs )
			{
				pr_debug("DSM_IOCS_DSRC_TYPE %08lx\n", arg);
				REG_WRITE(&dsm_dsrc_regs->type, arg);
				ret = 0;
			}
			else
				ret = -ENODEV;
			break;


		// Access to data sink regs
		case DSM_IOCS_DSNK_CTRL:
			if ( dsm_dsnk_regs )
			{
				pr_debug("DSM_IOCS_DSNK_CTRL %08lx\n", arg);
				REG_WRITE(&dsm_dsnk_regs->ctrl, arg);
				ret = 0;
			}
			else
				ret = -ENODEV;
			break;

		case DSM_IOCG_DSNK_STAT:
			if ( dsm_dsnk_regs )
			{
				reg = REG_READ(&dsm_dsnk_regs->stat);
				pr_debug("DSM_IOCG_DSNK_STAT %08lx\n", reg);
				ret = put_user(reg, (unsigned long *)arg);
			}
			else
				ret = -ENODEV;
			break;

		case DSM_IOCG_DSNK_BYTES:
			if ( dsm_dsnk_regs )
			{
				reg = REG_READ(&dsm_dsnk_regs->bytes);
				pr_debug("DSM_IOCG_DSNK_BYTES %08lx\n", reg);
				ret = put_user(reg, (unsigned long *)arg);
			}
			else
				ret = -ENODEV;
			break;

		case DSM_IOCG_DSNK_SUM:
			if ( dsm_dsnk_regs )
			{
				unsigned long sum[2];
				
				sum[0] = REG_READ(&dsm_dsnk_regs->sum_h);
				sum[1] = REG_READ(&dsm_dsnk_regs->sum_l);

				pr_debug("DSM_IOCG_DSNK_SUM %08lx.%08lx\n", sum[0], sum[1]);
				ret = copy_to_user((void *)arg, sum, sizeof(sum));
			}
			else
				ret = -ENODEV;
			break;


		// Access to ADI LVDS regs
		case DSM_IOCS_ADI1_OLD_CTRL:
			if ( dsm_adi1_old_regs )
			{
				pr_debug("DSM_IOCS_ADI1_OLD_CTRL %08lx\n", arg);
				REG_WRITE(&dsm_adi1_old_regs->ctrl, arg);
				ret = 0;
			}
			else
				ret = -ENODEV;
			break;

		case DSM_IOCG_ADI1_OLD_CTRL:
			if ( dsm_adi1_old_regs )
			{
				reg = REG_READ(&dsm_adi1_old_regs->ctrl);
				pr_debug("DSM_IOCG_ADI1_OLD_CTRL %08lx\n", reg);
				ret = put_user(reg, (unsigned long *)arg);
			}
			else
				ret = -ENODEV;
			break;

		case DSM_IOCS_ADI1_OLD_TX_CNT:
			if ( dsm_adi1_old_regs )
			{
				pr_debug("DSM_IOCS_ADI1_OLD_TX_CNT %08lx (%d)\n", arg, max);
				REG_WRITE(&dsm_adi1_old_regs->tx_cnt, arg);
				ret = 0;
			}
			else
				ret = -ENODEV;
			break;

		case DSM_IOCG_ADI1_OLD_TX_CNT:
			if ( dsm_adi1_old_regs )
			{
				reg = REG_READ(&dsm_adi1_old_regs->tx_cnt);
				pr_debug("DSM_IOCG_ADI1_OLD_TX_CNT %08lx\n", reg);
				ret = put_user(reg, (unsigned long *)arg);
			}
			else
				ret = -ENODEV;
			break;

		case DSM_IOCS_ADI1_OLD_RX_CNT:
			if ( dsm_adi1_old_regs )
			{
				pr_debug("DSM_IOCS_ADI1_OLD_RX_CNT %08lx\n", arg);
				REG_WRITE(&dsm_adi1_old_regs->rx_cnt, arg);
				ret = 0;
			}
			else
				ret = -ENODEV;
			break;

		case DSM_IOCG_ADI1_OLD_RX_CNT:
			if ( dsm_adi1_old_regs )
			{
				reg = REG_READ(&dsm_adi1_old_regs->rx_cnt);
				pr_debug("DSM_IOCG_ADI1_OLD_RX_CNT %08lx\n", reg);
				ret = put_user(reg, (unsigned long *)arg);
			}
			else
				ret = -ENODEV;
			break;

		case DSM_IOCG_ADI1_OLD_SUM:
			if ( dsm_adi1_old_regs )
			{
				unsigned long sum[2];
				
				sum[0] = REG_READ(&dsm_adi1_old_regs->cs_hi);
				sum[1] = REG_READ(&dsm_adi1_old_regs->cs_low);

				pr_debug("DSM_IOCG_ADI1_OLD_SUM %08lx.%08lx\n", sum[0], sum[1]);
				ret = copy_to_user((void *)arg, sum, sizeof(sum));
			}
			else
				ret = -ENODEV;
			break;

		case DSM_IOCG_ADI1_OLD_LAST:
			if ( dsm_adi1_old_regs )
			{
				unsigned long last[2];
				
				last[0] = REG_READ(&dsm_adi1_old_regs->tx_hi);
				last[1] = REG_READ(&dsm_adi1_old_regs->tx_low);

				pr_debug("DSM_IOCG_ADI1_OLD_LAST %08lx.%08lx\n", last[0], last[1]);
				ret = copy_to_user((void *)arg, last, sizeof(last));
			}
			else
				ret = -ENODEV;
			break;


		case DSM_IOCS_ADI2_OLD_CTRL:
			if ( dsm_adi2_old_regs )
			{
				pr_debug("DSM_IOCS_ADI2_OLD_CTRL %08lx\n", arg);
				REG_WRITE(&dsm_adi2_old_regs->ctrl, arg);
				ret = 0;
			}
			else
				ret = -ENODEV;
			break;

		case DSM_IOCG_ADI2_OLD_CTRL:
			if ( dsm_adi2_old_regs )
			{
				reg = REG_READ(&dsm_adi2_old_regs->ctrl);
				pr_debug("DSM_IOCG_ADI2_OLD_CTRL %08lx\n", reg);
				ret = put_user(reg, (unsigned long *)arg);
			}
			else
				ret = -ENODEV;
			break;

		case DSM_IOCS_ADI2_OLD_TX_CNT:
			if ( dsm_adi2_old_regs )
			{
				pr_debug("DSM_IOCS_ADI2_OLD_TX_CNT %08lx (%d)\n", arg, max);
				REG_WRITE(&dsm_adi2_old_regs->tx_cnt, arg);
				ret = 0;
			}
			else
				ret = -ENODEV;
			break;

		case DSM_IOCG_ADI2_OLD_TX_CNT:
			if ( dsm_adi2_old_regs )
			{
				reg = REG_READ(&dsm_adi2_old_regs->tx_cnt);
				pr_debug("DSM_IOCG_ADI2_OLD_TX_CNT %08lx\n", reg);
				ret = put_user(reg, (unsigned long *)arg);
			}
			else
				ret = -ENODEV;
			break;

		case DSM_IOCS_ADI2_OLD_RX_CNT:
			if ( dsm_adi2_old_regs )
			{
				pr_debug("DSM_IOCS_ADI2_OLD_RX_CNT%08lx\n", arg);
				REG_WRITE(&dsm_adi2_old_regs->rx_cnt, arg);
				ret = 0;
			}
			else
				ret = -ENODEV;
			break;

		case DSM_IOCG_ADI2_OLD_RX_CNT:
			if ( dsm_adi2_old_regs )
			{
				reg = REG_READ(&dsm_adi2_old_regs->rx_cnt);
				pr_debug("DSM_IOCG_ADI2_OLD_RX_CNT %08lx\n", reg);
				ret = put_user(reg, (unsigned long *)arg);
			}
			else
				ret = -ENODEV;
			break;

		case DSM_IOCG_ADI2_OLD_SUM:
			if ( dsm_adi2_old_regs )
			{
				unsigned long sum[2];
				
				sum[0] = REG_READ(&dsm_adi2_old_regs->cs_hi);
				sum[1] = REG_READ(&dsm_adi2_old_regs->cs_low);

				pr_debug("DSM_IOCG_ADI2_OLD_SUM %08lx.%08lx\n", sum[0], sum[1]);
				ret = copy_to_user((void *)arg, sum, sizeof(sum));
			}
			else
				ret = -ENODEV;
			break;

		case DSM_IOCG_ADI2_OLD_LAST:
			if ( dsm_adi2_old_regs )
			{
				unsigned long last[2];
				
				last[0] = REG_READ(&dsm_adi2_old_regs->tx_hi);
				last[1] = REG_READ(&dsm_adi2_old_regs->tx_low);

				pr_debug("DSM_IOCG_ADI2_OLD_LAST %08lx.%08lx\n", last[0], last[1]);
				ret = copy_to_user((void *)arg, last, sizeof(last));
			}
			else
				ret = -ENODEV;
			break;


		// Target list bitmap from xparameters
		case DSM_IOCG_TARGET_LIST:
			reg = 0;
			if ( dsm_dsrc_regs ) reg |= DSM_TARGT_DSXX;
			if ( dsm_dsnk_regs ) reg |= DSM_TARGT_DSXX;
			if ( dsm_adi1_old_regs ) reg |= DSM_TARGT_ADI1;
			if ( dsm_adi2_old_regs ) reg |= DSM_TARGT_ADI2;
			if ( dsm_adi1_new_regs ) reg |= DSM_TARGT_ADI1|DSM_TARGT_NEW;
			if ( dsm_adi2_new_regs ) reg |= DSM_TARGT_ADI2|DSM_TARGT_NEW;
			pr_debug("DSM_IOCG_TARGET_LIST %08lx { %s%s%s%s}\n", reg, 
			         reg & DSM_TARGT_DSXX ? "dsxx " : "",
			         reg & DSM_TARGT_NEW  ? "new: " : "",
			         reg & DSM_TARGT_ADI1 ? "adi1 " : "",
			         reg & DSM_TARGT_ADI2 ? "adi2 " : "");
			ret = put_user(reg, (unsigned long *)arg);
			break;


		// Arbitrary register access for now 
		case DSM_IOCG_ADI_NEW_REG:
		case DSM_IOCS_ADI_NEW_REG_SO:
		case DSM_IOCS_ADI_NEW_REG_RB:
		{
			struct dsm_new_adi_regs  regs;
			void __iomem            *addr = NULL;

			if ( (ret = copy_from_user(&regs, (void *)arg, sizeof(regs))) )
			{
				pr_err("failed to copy %d bytes, stop\n", ret);
				return -EFAULT;
			}

			// validate device and regs pointer
			switch ( regs.adi )
			{
				case 0: addr = dsm_adi1_new_regs; break;
				case 1: addr = dsm_adi2_new_regs; break;
				default:
					pr_err("regs.dev %lu invalid, stop\n", regs.adi);
					return -EINVAL;
			}
			if ( !addr )
			{
				pr_err("register access pointers not setup, stop.\n");
				return -ENODEV;
			}

			switch ( cmd )
			{
				case DSM_IOCS_ADI_NEW_REG_SO:
				case DSM_IOCS_ADI_NEW_REG_RB:
					REG_WRITE(ADI_NEW_RT_ADDR(addr, regs.ofs, regs.tx), regs.val);
					pr_debug("DSM_IOCS_ADI_NEW_REG AD%c %cX +%04lx WRITE %08lx\n",
					         (unsigned char)regs.adi + '1', regs.tx ? 'T' : 'R',
					         regs.ofs, regs.val);
					// DSM_IOCS_ADI_NEW_REG_SO: set-only, break here
					// DSM_IOCS_ADI_NEW_REG_RB: fall-through, readback
					if ( cmd == DSM_IOCS_ADI_NEW_REG_SO )
						break;

				case DSM_IOCG_ADI_NEW_REG:
					regs.val = REG_READ(ADI_NEW_RT_ADDR(addr, regs.ofs, regs.tx));
					pr_debug("DSM_IOCG_ADI_NEW_REG AD%c %cX +%04lx READ %08lx\n",
					         (unsigned char)regs.adi + '1', regs.tx ? 'T' : 'R',
					         regs.ofs, regs.val);
					break;

			}

			// set-only: no need to copy back to userspace
			if ( cmd == DSM_IOCS_ADI_NEW_REG_SO )
				ret = 0;
			// read or write/readback: copy back to userspace
			else if ( (ret = copy_to_user((void *)arg, &regs, sizeof(regs))) )
			{
				pr_err("failed to copy %d bytes, stop\n", ret);
				return -EFAULT;
			}
			break;
		}
	}

	pr_debug("%s(): return %d\n", __func__, ret);
	return ret;
}

static int dsm_release (struct inode *inode_p, struct file *file_p)
{
	unsigned long  flags;
	int            ret = -EBADF;

	spin_lock_irqsave(&dsm_lock, flags);
	if ( dsm_users )
	{
		pr_debug("%s()\n", __func__);
		dsm_halt_all();
		dsm_cleanup();

		dsm_users = 0;

		ret = 0;
	}
	spin_unlock_irqrestore(&dsm_lock, flags);

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

	if ( (ret = dsm_xparameters_init()) )
		return ret;

	if ( misc_register(&mdev) < 0 )
	{
		pr_err("misc_register() failed\n");
		ret = -EIO;
		goto error2;
	}

	dsm_dev = mdev.this_device;
	init_waitqueue_head(&dsm_wait);
	init_completion(&dsm_adi1_state.txrx);
	init_completion(&dsm_adi2_state.txrx);
	init_completion(&dsm_dsxx_state.txrx);

	pr_info("registered successfully\n");
	return 0;

error2:
	dsm_xparameters_exit();
	return ret;
}

static void __exit dma_streamer_mod_exit(void)
{
	dsm_halt_all();
	dsm_cleanup();

	dsm_dev = NULL;
	misc_deregister(&mdev);

	dsm_xparameters_exit();
}

module_init(dma_streamer_mod_init);
module_exit(dma_streamer_mod_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Morgan Hughes <morgan.hughes@silver-bullet-tech.com>");
MODULE_DESCRIPTION("DMA streaming test module");

