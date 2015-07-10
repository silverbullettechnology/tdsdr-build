/** \file      dsm_private.h
 *  \brief     Private structs and prototypes internal to DSM
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
#ifndef _INCLUDE_DSM_PRIVATE_H
#define _INCLUDE_DSM_PRIVATE_H
#include <linux/kernel.h>

#include "dma_streamer_mod.h"


#define pr_trace(fmt,...) do{ }while(0)
//#define pr_trace pr_debug


/******* Private structs ******/


// private per-transfer / per-block struct
struct dsm_xfer
{
	// For repeating / cyclic transfers: each repetition transfers chunk_words using
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
	// fields setup in dsm_chan_identify() regardless whether the channel is in use; 
	unsigned long    slot;
	struct dsm_user *user;

	// fields below this are setup and cleared based on whether a channel is in use.
	const char         *name;
	struct task_struct *task;

	// transfer specifications
	size_t                  xfer_size;
	struct dsm_xfer       **xfer_list;
	struct dsm_user_buffs  *user_buffs;

	// transfer order control
	size_t    xfer_curr;
	atomic_t  cyclic;

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

	unsigned long  xilinx_channels;
};


/******* dma_streamer_mod.c ******/

/** Generic device pointer for DMA registration */
extern struct device *dsm_dev;

/** Returns the total limit mappable for DMA at once
 *
 *  \param  xilinx  Calculate based on Xilinx AXI-DMA driver
 *
 *  \return  Number of words which can be mapped at once
 */
size_t dsm_limit_words (int xilinx);


/******* dsm_chan.c ******/

/** Clean up memory and resources for this DSM channel
 *
 *  \param  chan  DSM channel state structure pointer
 */
void dsm_chan_cleanup (struct dsm_chan *chan);

/** Setup channel with zero-copy userspace buffers
 *
 *  \param  user  DSM user state structure pointer
 *  \param  ucb   Zero-copy buffer list from userspace
 *
 *  \return  0 on success, <0 on failure
 */
int dsm_chan_map_user (struct dsm_user *user, struct dsm_user_buffs *ucb);

/** Scan DMA channels registered with dmaengine and populate 
 *
 *  \param  user  DSM user state structure pointer
 *
 *  \return  user->desc_list pointer on success, NULL on failure
 */
struct dsm_chan_list *dsm_chan_identify (struct dsm_user *user);


/******* dsm_xfer.c ******/

/** Scan DMA channels registered with dmaengine and populate 
 *
 *  \param  chan  DSM channel state structure pointer
 *  \param  xfer  DSM transfer state structure pointer
 */
void dsm_xfer_cleanup (struct dsm_chan *chan, struct dsm_xfer *xfer);

/** Setup transfer - currently assumes zero-copy userspace page
 *
 *  \param  chan  DSM channel state structure pointer
 *  \param  desc  DMA channel description pointer
 *  \param  buff  Userspace buffer specfication
 *
 *  \return  DSM transfer state structure
 */
struct dsm_xfer *dsm_xfer_map_user (struct dsm_chan            *chan,
                                    struct dsm_chan_desc       *desc,
                                    const struct dsm_user_xfer *buff);


/******* dsm_thread.c  ******/

/** Start single channel running
 *
 *  \param  chan  DSM channel state structure pointer
 *
 *  \return  0 on success, <0 on failure
 */
int dsm_start_chan (struct dsm_chan *chan);

/** Start active channels with bitmask
 *
 *  \param  user    DSM user state structure pointer
 *  \param  mask    Bitmask of channels to start
 *  \param  cyclic  If !0 set each channel's .cyclic field
 *
 *  \return  0 on success, or number of threads which failed to start
 */
int dsm_start_mask (struct dsm_user *user, unsigned long mask, int cyclic);

/** Halt single running channel
 *
 *  \param  chan  DSM channel state structure pointer
 */
void dsm_halt_chan (struct dsm_chan *chan);

/** Halt active channels with bitmask
 *
 *  \param  user  DSM user state structure pointer
 *  \param  mask  Bitmask of channels to start
 */
void dsm_halt_mask (struct dsm_user *user, unsigned long mask);

/** Halt all active channels
 *
 *  \param  user  DSM user state structure pointer
 */
void dsm_halt_all (struct dsm_user *user);

/** Stop a single running channel after its current transfer
 *
 *  \param  chan  DSM channel state structure pointer
 */
void dsm_stop_chan (struct dsm_chan *chan);

/** Stop active channels after their current transfers with bitmask
 *
 *  \param  user  DSM user state structure pointer
 *  \param  mask  Bitmask of channels to stop
 */
void dsm_stop_mask (struct dsm_user *user, unsigned long mask);


#endif // _INCLUDE_DSM_PRIVATE_H
