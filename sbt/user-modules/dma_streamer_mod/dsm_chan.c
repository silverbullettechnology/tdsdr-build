/** \file      dsm_chan.c
 *  \brief     DMA channel handling for DSM
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


/** Clean up memory and resources for this DSM channel
 *
 *  \param  chan  DSM channel state structure pointer
 */
void dsm_chan_cleanup (struct dsm_chan *chan)
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


/** DMA channel private value, abused to identify channel attached to specific endpoint
 *
 *  \param  chan   Linux dmaengine DMA channel pointer
 *
 *  \return  Value of chan->private, or 0
 */
static u32 dsm_dma_chan_private (struct dma_chan *chan)
{
	// Linux DMA infrastructure doesn't seem to have the concept of a DMA channel being
	// tied to a particular endpoint, so various drivers check the private pointer.  The
	// two which have been used in the Xilinx AXI-DMA world are to cast the pointer to a
	// u32 or to point it at an actual u32 variable in the driver's internal state...
	// In the case of multiple DMA controllers, we put the ID of the attached endpoint
	// into the top nibble of the private value
	if ( (unsigned long)chan->private > 0x80000000 )
		return *(u32 *)chan->private;

	return (u32)chan->private;
}


/** Callback for dma_request_channel to match specific channel */
static bool dsm_chan_map_user_find (struct dma_chan *chan, void *param)
{
	u32  criterion = *(unsigned long *)param;
	u32  candidate = dsm_dma_chan_private(chan);

	pr_trace("%s: chan %p -> %08x vs %08x\n", __func__, chan, candidate, criterion);
	return candidate == criterion;
}


/** Setup channel with zero-copy userspace buffers
 *
 *  \param  user  DSM user state structure pointer
 *  \param  ucb   Zero-copy buffer list from userspace
 *
 *  \return  0 on success, <0 on failure
 */
int dsm_chan_map_user (struct dsm_user *user, struct dsm_user_buffs *ucb)
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
	chan->dma_chan = dma_request_channel(mask, dsm_chan_map_user_find, &desc->private);
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
	atomic_set(&chan->cyclic, 0);

	// setup xfer buffs
	for ( xfer = 0; xfer < chan->xfer_size; xfer++ )
	{
		chan->xfer_list[xfer] = dsm_xfer_map_user(chan, desc, &ucb->list[xfer]);
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


/** Callback for dma_request_channel to count channels registered with dmaengine */
static bool dsm_chan_identify_size (struct dma_chan *chan, void *param)
{
	if ( chan->device && chan->device->dev && chan->device->dev->driver )
		(*(size_t *)param)++;

	return 0;
}


/** Callback for dma_request_channel to record particulars of registered dma channels */
static bool dsm_chan_identify_desc (struct dma_chan *chan, void *param)
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

	// special flag for Xilinx DMA for estimating transfer size based on descriptor memory
	if ( strstr(chan->device->dev->driver->name, "xilinx-dma") )
	{
		cdp->flags |= DSM_CHAN_XILINX_DMA;
		cdp->words = dsm_limit_words(1);
	}
	else
		cdp->words = dsm_limit_words(0);

	(*cdpp)++;
	return 0;
}


/** Scan DMA channels registered with dmaengine and populate 
 *
 *  \param  user  DSM user state structure pointer
 *
 *  \return  user->desc_list pointer on success, NULL on failure
 */
struct dsm_chan_list *dsm_chan_identify (struct dsm_user *user)
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
	BUG_ON(dma_request_channel(mask, dsm_chan_identify_size, &user->chan_size) != NULL);

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
	BUG_ON(dma_request_channel(mask, dsm_chan_identify_desc, &cdp) != NULL);

	// setup permanent fields
	user->xilinx_channels = 0;
	for ( slot = 0; slot < user->desc_list->size; slot++ )
	{
		user->chan_list[slot].user = user;
		user->chan_list[slot].slot = slot;
		if ( user->desc_list->list[slot].flags & DSM_CHAN_XILINX_DMA )
			user->xilinx_channels++;
		pr_debug("%2d: flags:%08lx width:%02u align:%02u driver:%s device:%s\n",
		         slot,
		         user->desc_list->list[slot].flags,
		         (1 << (user->desc_list->list[slot].width + 3)),
		         (1 << (user->desc_list->list[slot].align + 3)),
		         user->desc_list->list[slot].driver,
		         user->desc_list->list[slot].device);
	}

	return user->desc_list;
}


