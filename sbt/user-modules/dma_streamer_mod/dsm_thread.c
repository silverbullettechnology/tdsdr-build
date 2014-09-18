/** \file      dsm_thread.c
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



static void dsm_thread_cb (void *cmp)
{
	pr_debug("completion\n");
	complete(cmp);
}

int dsm_thread (void *data)
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
			chan->stats.errors++;
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
			chan->stats.errors++;
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

		if ( !xfer->left && atomic_read(&chan->cyclic) )
		{
			pr_debug("cyclic: reset\n");
			xfer->left = xfer->words;
		}
	}

done:
	// thread's done, decrement counter and wakeup caller
	atomic_sub(1, &user->busy);
	wake_up_interruptible(&user->wait);
	return ret;
}




int dsm_start_chan (struct dsm_chan *chan)
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

int dsm_start_mask (struct dsm_user *user, unsigned long mask, int cyclic)
{
	int  ret = 0;
	int  slot;

	mask &= user->active_mask;
	pr_debug("%s: mask %08lx\n", __func__, mask);

	if ( cyclic )
		for ( slot = 0; slot < user->chan_size; slot++ )
			if ( mask & (1 << slot) && user->chan_list[slot].dma_dir == DMA_MEM_TO_DEV )
			{
				pr_debug("atomic_set(%p, 1)\n", &user->chan_list[slot].cyclic);
				atomic_set(&user->chan_list[slot].cyclic, 1);
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


void dsm_halt_chan (struct dsm_chan *chan)
{
	if ( chan->task )
	{
		kthread_stop(chan->task);
		chan->task = NULL;
	}
}

void dsm_halt_mask (struct dsm_user *user, unsigned long mask)
{
	int  slot;

	mask &= user->active_mask;

	for ( slot = 0; slot < user->chan_size; slot++ )
		if ( mask & (1 << slot) )
			dsm_halt_chan(&user->chan_list[slot]);
}

void dsm_halt_all (struct dsm_user *user)
{
	return dsm_halt_mask(user, ~0);
}


void dsm_stop_chan (struct dsm_chan *chan)
{
	atomic_set(&chan->cyclic, 0);
}

void dsm_stop_mask (struct dsm_user *user, unsigned long mask)
{
	int  slot;

	mask &= user->active_mask;

	for ( slot = 0; slot < user->chan_size; slot++ )
		if ( mask & (1 << slot) )
			dsm_stop_chan(&user->chan_list[slot]);
}
