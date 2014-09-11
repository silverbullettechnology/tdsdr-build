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


/* DMA Channel Description: DSM_IOCG_CHANNELS should be called with a pointer to a buffer
 * to a dsm_chan_list, which it will fill in.  dsm_chan_list.chan_cnt should be set to the
 * number of dsm_chan_desc structs the dsm_chan_list has room for before the ioctl call.
 * The driver code will not write more than chan_cnt elements into the array, but will set
 * chan_cnt to the actual number available before returning to the caller. 
 */
#define DSM_CHAN_DIR_MASK        0xC0000000
#define DSM_CHAN_DIR_TX          0x80000000
#define DSM_CHAN_DIR_RX          0x40000000
#define DSM_CHAN_SLAVE_CAPS      0x20000000
#define DSM_CHAN_CAP_MEMCPY      0x00000001
#define DSM_CHAN_CAP_XOR         0x00000002
#define DSM_CHAN_CAP_PQ          0x00000004
#define DSM_CHAN_CAP_XOR_VAL     0x00000008
#define DSM_CHAN_CAP_PQ_VAL      0x00000010
#define DSM_CHAN_CAP_INTERRUPT   0x00000020
#define DSM_CHAN_CAP_SG          0x00000040
#define DSM_CHAN_CAP_PRIVATE     0x00000080
#define DSM_CHAN_CAP_ASYNC_TX    0x00000100
#define DSM_CHAN_CAP_SLAVE       0x00000200
#define DSM_CHAN_CAP_CYCLIC      0x00000400
#define DSM_CHAN_CAP_INTERLEAVE  0x00000800

struct dsm_chan_desc
{
	unsigned long  ident;       /* Channel ident value */
	unsigned long  private;     /* Channel private value */
	unsigned long  flags;       /* DSM_CHAN_* flags */
	unsigned char  width;       /* Width of channel, 64bit: width 3 -> (1 << 3) -> 8 */
	unsigned char  align;       /* Alignment requirement, as above */
	char           device[32];  /* Device name */
	char           driver[32];  /* Driver name */
};

struct dsm_chan_list
{
	unsigned long         chan_cnt;     /* Number of xfer buffers */
	struct dsm_chan_desc  chan_lst[0];  /* Array of xfer buffers */
};


/* DMA Buffer Description: DSM_IOCS_MAP should be called for each DMA channel a transfer
 * is to be setup for.  dsm_chan_buffs.ident should match the value of a channel returned
 * from DSM_IOCG_CHANNELS, and dsm_chan_buffs.buff_cnt should give the number of
 * dsm_xfer_buff structs.  dsm_xfer_buff.reps is 1 for a single shot, 0 for continuous.
 */
struct dsm_xfer_buff
{
	unsigned long  addr;   /* Userspace address for get_user_pages() */
	unsigned long  size;   /* Size of userspace buffer in words */
	unsigned long  reps;   /* Number of times to transfer this buff */
};

struct dsm_chan_buffs
{
	unsigned long         ident;        /* Channel ident from DSM_IOCG_CHANNELS */
	unsigned long         buff_cnt;     /* Number of xfer buffers */
	struct dsm_xfer_buff  buff_lst[0];  /* Array of xfer buffers */
};


/* DMA Transfer Statistics: DSM_IOCG_STATS should be passed a buffer with chan_cnt set to
 * the number of dsm_xfer_stats structs available in chan_lst[].  It will fill in as many
 * stats structs as will fit, and reset chan_cnt to the actual number available, which may
 * be higher.
 */
struct dsm_xfer_stats
{
	unsigned long       ident;        /* Channel ident from DSM_IOCG_CHANNELS */
	unsigned long long  bytes;        /* Number of bytes transferred */
	struct timespec     total;        /* Sum of time measurements */
	unsigned long       completes;    /* Number of successful DMA transfers */
	unsigned long       errors;       /* Number of DMA transfer errors */
	unsigned long       timeouts;     /* Number of DMA transfer timeouts */
};

struct dsm_chan_stats
{
	unsigned long          chan_cnt;     /* Number of per-channel stats buffers */
	struct dsm_xfer_stats  chan_lst[0];  /* Array of per-channel stats buffers */
};


/* DMA Channel Description: DSM_IOCG_CHANNELS should be called with a pointer to a buffer
 * to a dsm_chan_list, which it will fill in.  dsm_chan_list.chan_cnt should be set to the
 * number of dsm_chan_desc structs the dsm_chan_list has room for before the ioctl call.
 * The driver code will not write more than chan_cnt elements into the array, but will set
 * chan_cnt to the actual number available before returning to the caller. 
 */
#define  DSM_IOCG_CHANNELS  _IOR(DSM_IOCTL_MAGIC, 0, struct dsm_chan_list *)


// Set the (userspace) addresses and sizes of the buffers.  These must be page-aligned (ie
// allocated with posix_memalign()), locked in with mlock(), and size a multiple of
// DSM_BUS_WIDTH.  
#define  DSM_IOCS_MAP_CHAN  _IOW(DSM_IOCTL_MAGIC, 0, struct dsm_chan_buffs *)


// Start a one-shot transaction, after setting up the buffers with a successful
// DSM_IOCS_MAP.  Returns immediately to the calling process, which should issue 
// DSM_IOCS_ONESHOT_WAIT to wait for the transaction to complete or timeout
#define  DSM_IOCS_ONESHOT_START  _IOW(DSM_IOCTL_MAGIC, 1, unsigned long)

// Wait for a oneshot transaction started with DSM_IOCS_ONESHOT_START.  The
// calling process blocks until all transfers are complete, or timeout.
#define  DSM_IOCS_ONESHOT_WAIT  _IOW(DSM_IOCTL_MAGIC, 2, unsigned long)


// Start a transaction, after setting up the buffers with a successful DSM_IOCS_MAP. The
// calling process does not block, which is only really useful with a continuous transfer.
// The calling process should stop those with DSM_IOCS_CONTINUOUS_STOP before unmapping
// the buffers. 
#define  DSM_IOCS_CONTINUOUS_START  _IOW(DSM_IOCTL_MAGIC, 3, unsigned long)

// Stop a running transaction started with DSM_IOCS_CONTINUOUS_START.  The calling process
// blocks until both transfers are complete, or timeout.
#define  DSM_IOCS_CONTINUOUS_STOP  _IOW(DSM_IOCTL_MAGIC, 4, unsigned long)


// Read statistics from the transfer triggered with DSM_IOCS_TRIGGER
#define  DSM_IOCG_STATS  _IOR(DSM_IOCTL_MAGIC, 5, struct dsm_user_stats *)

// Unmap the buffers buffer mapped with DSM_IOCS_MAP, before DSM_IOCS_TRIGGER.  
#define  DSM_IOCS_UNMAP  _IOW(DSM_IOCTL_MAGIC, 6, unsigned long)

// Set a timeout in jiffies on the DMA transfer
#define  DSM_IOCS_TIMEOUT  _IOW(DSM_IOCTL_MAGIC, 7, unsigned long)


#endif // _INCLUDE_DMA_STREAMER_MOD_H
/* Ends    : dma_streamer_mod.h */
