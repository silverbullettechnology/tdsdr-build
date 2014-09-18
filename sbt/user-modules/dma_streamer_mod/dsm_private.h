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
	// fields setup in dsm_scan_channels() regardless whether the channel is in use; 
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
extern struct device *dsm_dev;
size_t dsm_limit_words (int xilinx);


/******* dsm_chan.c ******/
void dsm_chan_cleanup (struct dsm_chan *chan);
bool dsm_chan_setup_find (struct dma_chan *chan, void *param);
int dsm_chan_setup (struct dsm_user *user, struct dsm_user_buffs *ucb);
struct dsm_chan_list *dsm_scan_channels (struct dsm_user *user);


/******* dsm_xfer.c ******/
void dsm_xfer_cleanup (struct dsm_chan *chan, struct dsm_xfer *xfer);
struct dsm_xfer *dsm_xfer_setup (struct dsm_chan            *chan,
                                 struct dsm_chan_desc       *desc,
                                 const struct dsm_user_xfer *buff);


/******* dsm_thread.c  ******/
int dsm_thread (void *data);
int dsm_start_chan (struct dsm_chan *chan);
int dsm_start_mask (struct dsm_user *user, unsigned long mask, int cyclic);
void dsm_halt_chan (struct dsm_chan *chan);
void dsm_halt_mask (struct dsm_user *user, unsigned long mask);
void dsm_halt_all (struct dsm_user *user);
void dsm_stop_chan (struct dsm_chan *chan);
void dsm_stop_mask (struct dsm_user *user, unsigned long mask);


#endif // _INCLUDE_DSM_PRIVATE_H
