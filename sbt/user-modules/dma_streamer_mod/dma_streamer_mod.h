/** \file      dma_streamer_mod.h
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
#ifndef _INCLUDE_DMA_STREAMER_MOD_H
#define _INCLUDE_DMA_STREAMER_MOD_H

#ifdef __KERNEL__
#include <linux/time.h>
#endif


#define DSM_DRIVER_NODE "dma_streamer_mod"


#define DSM_BUS_WIDTH  8
#define DSM_MAX_SIZE   320000000

#define DSM_IOCTL_MAGIC 1


struct dsm_xfer_buff
{
	unsigned long  addr;   /* Userspace address for get_user_pages() */
	unsigned long  size;   /* Size of userspace buffer in bytes */
	unsigned long  words;  /* Number of words to transfer per repetition */
};

struct dsm_chan_buffs
{
	struct dsm_xfer_buff  tx;
	struct dsm_xfer_buff  rx;
};

struct dsm_user_buffs
{
	struct dsm_chan_buffs  adi1;
	struct dsm_chan_buffs  adi2;
	struct dsm_chan_buffs  dsx0;
	struct dsm_chan_buffs  dsx1;
};


struct dsm_xfer_stats
{
	unsigned long long  bytes;
	struct timespec     total;
	unsigned long       completes;
	unsigned long       errors;
	unsigned long       timeouts;
};

struct dsm_chan_stats
{
	struct dsm_xfer_stats  tx;
	struct dsm_xfer_stats  rx;
};

struct dsm_user_stats
{
	struct dsm_chan_stats  adi1;
	struct dsm_chan_stats  adi2;
	struct dsm_chan_stats  dsx0;
	struct dsm_chan_stats  dsx1;
};


// Set the (userspace) addresses and sizes of the buffers.  These must be page-aligned (ie
// allocated with posix_memalign()), locked in with mlock(), and size a multiple of
// DSM_BUS_WIDTH.  
#define  DSM_IOCS_MAP  _IOW(DSM_IOCTL_MAGIC, 0, struct dsm_user_buffs *)

// Trigger a transaction, after setting up the buffers with a successful DSM_IOCS_MAP.  If
// the mapped buffers have a nonzero tx_size, a TX transfer is started.  If the mapped
// buffers have a nonzero rx_size, a RX transfer is started.  The two directions may be
// run in parallel.  The calling process will block until both transfers are complete, or
// a timeout occurs.
#define  DSM_IOCS_TRIGGER  _IOW(DSM_IOCTL_MAGIC, 1, unsigned long)

// Read statistics from the transfer triggered with DSM_IOCS_TRIGGER
#define  DSM_IOCG_STATS  _IOR(DSM_IOCTL_MAGIC, 2, struct dsm_user_stats *)

// Unmap the buffers buffer mapped with DSM_IOCS_MAP, before DSM_IOCS_TRIGGER.  
#define  DSM_IOCS_UNMAP  _IOW(DSM_IOCTL_MAGIC, 3, unsigned long)

// Set a timeout in jiffies on the DMA transfer
#define  DSM_IOCS_TIMEOUT  _IOW(DSM_IOCTL_MAGIC, 4, unsigned long)

// Start a continuous transaction, after setting up the buffers with a successful
// DSM_IOCS_MAP.  The calling process does not block, and should stop the transfer with
// DSM_IOCS_STOP before unmapping the buffers.
#define  DSM_IOCS_START  _IOW(DSM_IOCTL_MAGIC, 5, unsigned long)

// Stop a running transaction started with DSM_IOCS_START.  The calling process will block
// until all transfers are complete, or a timeout occurs.
#define  DSM_IOCS_STOP  _IOW(DSM_IOCTL_MAGIC, 6, unsigned long)


#endif // _INCLUDE_DMA_STREAMER_MOD_H
/* Ends    : dma_streamer_mod.h */
