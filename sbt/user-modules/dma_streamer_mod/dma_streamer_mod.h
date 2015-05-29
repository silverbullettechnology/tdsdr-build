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

#define DSM_IOCTL_MAGIC 1


/* DMA Channel Description: DSM_IOCG_CHANNELS should be called with a pointer to a buffer
 * to a dsm_chan_list, which it will fill in.  dsm_chan_list.size should be set to
 * the number of dsm_chan_desc structs the dsm_chan_list has room for before the ioctl
 * call.  The driver code will not write more than dsm_chan_list.size elements into the
 * array, but will set dsm_chan_list.size to the actual number available before returning
 * to the caller. 
 *
 * Note the position of each element in dsm_chan_list.list[] is used to select the
 * channel in later calls and determines the position of bits used in the start/stop/wait
 * commands below.
 *
 * For example, these desc values: 
 *   ioctl(fd, DSM_IOCG_CHANNELS, &d);
 *   d.chan_size = 4
 *   d.list[0] = { .private = 0x00100001, .flags = DSM_CHAN_DIR_TX, ... }
 *   d.list[1] = { .private = 0x00100002, .flags = DSM_CHAN_DIR_RX, ... }
 *   d.list[2] = { .private = 0x10100001, .flags = DSM_CHAN_DIR_TX, ... }
 *   d.list[3] = { .private = 0x10100002, .flags = DSM_CHAN_DIR_RX, ... }
 * indicate two TX and two RX channels.
 *
 * To setup two transmit channels, the user maps a buffer for each channel, using the
 * offset in the list:
 *   b.dsm_user_buffs = { .slot = 0, .buff_size = 1, ... }
 *   ioctl(fd, DSM_IOCS_MAP_USER, &b);
 *   b.dsm_user_buffs = { .slot = 2, .buff_size = 1, ... }
 *   ioctl(fd, DSM_IOCS_MAP_USER, &b);
 *
 * The position in dsm_chan_list.list[] is used as a bit position in the bitmask for
 * the trigger commands, so to trigger the TX and wait for it to complete:
 *   m = (1 << 0) | (1 << 2);
 *   ioctl(fd, DSM_IOCS_ONESHOT_START, m);
 *   ioctl(fd, DSM_IOCS_ONESHOT_WAIT,  m);
 */
#define DSM_CHAN_DIR_MASK        0xC0000000
#define DSM_CHAN_DIR_TX          0x80000000
#define DSM_CHAN_DIR_RX          0x40000000
#define DSM_CHAN_SLAVE_CAPS      0x20000000
#define DSM_CHAN_XILINX_DMA      0x10000000
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
	unsigned long  private;     /* Channel private value */
	unsigned long  flags;       /* DSM_CHAN_* flags */
	unsigned long  words;       /* Max number of words per transfer */
	unsigned char  width;       /* Width of channel, 64bit: width 3 -> (1 << 3) -> 8 */
	unsigned char  align;       /* Alignment requirement, as above */
	char           device[32];  /* Device name */
	char           driver[32];  /* Driver name */
};

struct dsm_chan_list
{
	unsigned long         size;     /* Number of xfer buffers */
	struct dsm_chan_desc  list[0];  /* Array of xfer buffers */
};


/* DMA Buffer Description: For each channel to setup with zero-copy userspace DMA, call
 * DSM_IOCS_MAP_USER with a dsm_user_buffs structure with the fields set to:
 *   .slot   = the index of the desired channel in dsm_chan_list.list[]
 *   .tx     = nonzero if the transfer is mem-to-dev, zero for dev-to-mem
 *   .size   = number of dsm_user_xfer elements in list[]
 *   .list[] = array of dsm_user_xfer elements to setup
 *
 * The driver will use get_user_pages() for each element in dsm_user_buffs.list[] and
 * create a matching scatterlist for DMA.  Each element in dsm_user_buffs.list[] should
 * have:
 *   .addr = page-aligned userspace buffer address, cast to unsigned long
 *   .size = length of buffer in words (DSM_BUS_WIDTH bytes per word)
 *
 * The buffers should be cleaned up with a call to DSM_IOCS_CLEANUP before close.
 */
struct dsm_user_xfer
{
	unsigned long  addr;   /* Userspace address for get_user_pages() */
	unsigned long  size;   /* Size of userspace buffer in words */
};

struct dsm_user_buffs
{
	unsigned long         slot;     /* Channel index from DSM_IOCG_CHANNELS */
	unsigned long         tx;       /* Nonzero for TX, zero for RX */
	unsigned long         size;     /* Number of xfer buffers */
	struct dsm_user_xfer  list[0];  /* Array of xfer buffers */
};


/* DMA Transfer Statistics: DSM_IOCG_STATS should be passed a dsm_chan_stats buffer with
 * dsm_chan_stats.size set to the number of elements in dsm_chan_stats.list[].  It will
 * fill in as many stats structs as will fit, and reset dsm_chan_stats.size to the actual
 * number available, which may be higher.
 */
struct dsm_xfer_stats
{
	unsigned long long  bytes;        /* Number of bytes transferred */
	struct timespec     total;        /* Sum of time measurements */
	unsigned long       starts;       /* Number of DMA transfers started */
	unsigned long       completes;    /* Number of successful DMA transfers */
	unsigned long       errors;       /* Number of DMA transfer errors */
	unsigned long       timeouts;     /* Number of DMA transfer timeouts */
};

struct dsm_chan_stats
{
	unsigned long          mask;     /* Bitmask of channels which were active */
	unsigned long          size;     /* Number of per-channel stats buffers */
	struct dsm_xfer_stats  list[0];  /* Array of per-channel stats buffers */
};


/* DMA Transfer Limits: limitations of the DMA stack for information of the userspace
 * tools; currently the maximum number of DMA channels and aggregate limit of DMAable
 * words across all transfers at once.  Other fields may be added as needed */
struct dsm_limits
{
	unsigned long  channels;       /* Total count of DMA channels */
	unsigned long  total_words;    /* Aggregate total of DMAable words */
};



/* DMA Channel Description: DSM_IOCG_CHANNELS should be called with a pointer to a buffer
 * to a dsm_chan_list, which it will fill in.  dsm_chan_list.chan_size should be set to the
 * number of dsm_chan_desc structs the dsm_chan_list has room for before the ioctl call.
 * The driver code will not write more than chan_size elements into the array, but will set
 * chan_size to the actual number available before returning to the caller. 
 */
#define  DSM_IOCG_CHANNELS  _IOR(DSM_IOCTL_MAGIC, 0, struct dsm_chan_list *)


/* DMA Transfer Limits: limitations of the DMA stack for information of the userspace
 * tools; currently only the limit of DMAable words across all transfers at once.  Other
 * fields may be added as needed
 */
#define  DSM_IOCG_LIMITS    _IOR(DSM_IOCTL_MAGIC, 1, struct dsm_limits *)


/* Set the (userspace) addresses and sizes of the buffers.  These must be page-aligned (ie
 * allocated with posix_memalign()), locked in with mlock(), and size a multiple of
 * DSM_BUS_WIDTH.  
 */
#define  DSM_IOCS_MAP_USER  _IOW(DSM_IOCTL_MAGIC, 2, struct dsm_user_buffs *)


/* Start a one-shot transaction, after setting up the buffers with a successful
 * DSM_IOCS_MAP.  Returns immediately to the calling process, which should issue 
 * DSM_IOCS_ONESHOT_WAIT to wait for the transaction to complete or timeout.  The value
 * passed is a bitmask indicating which channels to start; it will be masked by the
 * channels which have been setup with DSM_IOCS_MAP_CHAN calls so it's safe to use
 * ~0 for most cases.
 */
#define  DSM_IOCS_ONESHOT_START  _IOW(DSM_IOCTL_MAGIC, 3, unsigned long)

/* Wait for a oneshot transaction started with DSM_IOCS_ONESHOT_START.  The
 * calling process blocks until all transfers are complete, or timeout.  The value
 * passed is a bitmask indicating which channels to wait for; it will be masked by the
 * channels which have been started with DSM_IOCS_ONESHOT_START so it's safe to use
 * ~0 for most cases.
 */
#define  DSM_IOCS_ONESHOT_WAIT  _IOW(DSM_IOCTL_MAGIC, 4, unsigned long)


/* Start a transaction, after setting up the buffers with a successful DSM_IOCS_MAP. The
 * calling process does not block, which is only really useful with a repeated transfer.
 * The calling process should stop those with DSM_IOCS_CYCLIC_STOP before unmapping
 * the buffers. 
 */
#define  DSM_IOCS_CYCLIC_START  _IOW(DSM_IOCTL_MAGIC, 5, unsigned long)

/* Stop a running transaction started with DSM_IOCS_CYCLIC_START.  The calling process
 * blocks until both transfers are complete, or timeout.
 */
#define  DSM_IOCS_CYCLIC_STOP  _IOW(DSM_IOCTL_MAGIC, 6, unsigned long)


/* DMA Transfer Statistics: DSM_IOCG_STATS should be passed a buffer with chan_size set to
 * the number of dsm_xfer_stats structs available in chan_list[].  It will fill in as many
 * stats structs as will fit, and reset chan_size to the actual number available, which may
 * be higher.
 */
#define  DSM_IOCG_STATS  _IOR(DSM_IOCTL_MAGIC, 7, struct dsm_chan_stats *)


/* Cleanup the buffers for all active channels, mapped with DSM_IOCS_MAP_USER. */
#define  DSM_IOCS_CLEANUP  _IOW(DSM_IOCTL_MAGIC, 8, unsigned long)

/* Set a timeout in jiffies on the DMA transfer */
#define  DSM_IOCS_TIMEOUT  _IOW(DSM_IOCTL_MAGIC, 9, unsigned long)


#endif // _INCLUDE_DMA_STREAMER_MOD_H
/* Ends    : dma_streamer_mod.h */
