/** \file      sd_fifo.h
 *  \brief     AXI4-Stream FIFO access 
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
#ifndef _INCLUDE_SD_FIFO_H_
#define _INCLUDE_SD_FIFO_H_
#include <linux/kernel.h>
#include <linux/completion.h>
#include <linux/dmaengine.h>
#include <linux/spinlock.h>
#include <linux/platform_device.h>


/** Bits for flags in sd_fifo */
#define SD_FL_TRACE     0x00000001
#define SD_FL_DEBUG     0x00000002
#define SD_FL_INFO      0x00000004
#define SD_FL_WARN      0x00000008
#define SD_FL_ERROR     0x00000010
#define SD_FL_NO_RX     0x00000100
#define SD_FL_NO_TX     0x00000200
#define SD_FL_NO_RSP    0x00000400


/** Bits for sd_fifo_reset mask parameter */
#define SD_FR_TX  1
#define SD_FR_RX  2
#define SD_FR_SRR 4
#define SD_FR_ALL 7


/* AXI4-Stream FIFO registers */
struct sd_fifo_regs
{
	uint32_t  isr;      /* 0x00: RC: Interrupt Status Register                      */
	uint32_t  ier;      /* 0x04: RW: Interrupt Enable Register                      */
	uint32_t  tdfr;     /* 0x08: WO: Transmit Data FIFO Reset                       */
	uint32_t  tdfv;     /* 0x0C: RO: Transmit Data FIFO Vacancy                     */
	uint32_t  tdfd;     /* 0x10: WO: Use sf->tdfd for offset                        */
	uint32_t  tlr;      /* 0x14: WO: Transmit Length Register                       */
	uint32_t  rdfr;     /* 0x18: WO: Receive Data FIFO reset                        */
	uint32_t  rdfo;     /* 0x1C: RO: Receive Data FIFO Occupancy                    */
	uint32_t  rdfd;     /* 0x20: RO: Use sf->rdfd for offset                        */
	uint32_t  rlr;      /* 0x24: RO: Receive Length Register                        */
	uint32_t  srr;      /* 0x28: WO: AXI4-Stream Reset                              */
	uint32_t  tdr;      /* 0x2C: WO: Transmit Destination Register                  */
	uint32_t  rdr;      /* 0x30: RO: Receive Destination Register                   */
	uint32_t  tx_id;    /* 0x34: WO: Transmit ID Register (not supported)           */
	uint32_t  tx_user;  /* 0x38: WO: Transmit USER Register (not supported)         */
	uint32_t  rx_id;    /* 0x3C: RO: Receive ID Register (not supported)            */
	uint32_t  rx_user;  /* 0x40: RO: Receive USER Register (not supported)          */
	uint32_t  rsvd[21]; /* 0x44: Reserved                                           */
};


struct srio_dev;
struct sd_fifo;
struct sd_desc;


/* RX callback is responsible for dispatching the message and freeing the descriptor. */
typedef void (* sd_rx_callback) (struct sd_fifo *fifo, struct sd_desc *desc);

/* TX callback notifies the caller of successful transmit or timeout; descriptor(s) are
 * freed inside the FIFO code.  Currently results are 0:success, 1:timeout */
typedef void (* sd_tx_callback) (struct sd_fifo *fifo, unsigned cookie, int result);


struct sd_fifo_stats
{
	unsigned long  starts;
	unsigned long  chunks;
	unsigned long  completes;
	unsigned long  timeouts;
	unsigned long  errors;
	unsigned long  resets;
};

struct sd_fifo_dir
{
	spinlock_t            lock;
	struct dma_chan      *chan;
	dma_addr_t            phys;
	dma_cookie_t          cookie;
	struct timer_list     timer;
	unsigned long         timeout;
	struct sd_fifo_stats  stats;
};

struct sd_fifo
{
	struct device               *dev;
	struct srio_dev             *sd;
	int                          irq;
	char                         name[32];
	unsigned                     flags;

	/* Note: regs always points to an AXI-Lite interface for register access.  data may be
	 * the same if the AXI-Lite interface is used for data, or may instead point to an 
	 * AXI-Full interface.  From v4.1 of the FIFO core the TDFD/RDFD regs may be at a
	 * different offset for AXI-Full, so calculate the offsets at init time and add to
	 * data when accessing */
	struct sd_fifo_regs __iomem *regs;
	void                __iomem *data;
	dma_addr_t                   phys;
	unsigned                     tdfd;
	unsigned                     rdfd;

	struct sd_fifo_dir           rx;
	struct sd_fifo_dir           tx;

	struct sd_desc              *rx_current;
	sd_rx_callback               rx_func;

	struct sd_desc              *tx_current;
	struct list_head             tx_queue;
	struct list_head             tx_retry;
	unsigned                     tx_cookie;
	sd_tx_callback               tx_func;
	struct timer_list            rt_timer;
};


/** Init FIFO
 *
 *  \param  pdev  Platform device struct pointer
 *  \param  pref  Prefix for names in devicetree
 *
 *  \return  sd_fifo struct on success, NULL on error
 */
struct sd_fifo *sd_fifo_probe (struct platform_device *pdev, char *pref, unsigned flags);


/** Free FIFO
 *
 *  \param  sf   Pointer to sd_fifo struct
 */
void sd_fifo_free (struct sd_fifo *sf);


/** Reset FIFO's TX, RX, and/or AXI-Stream 
 *
 *  \note  Caller should hold &sf->rx.lock if SD_FR_RX is set in mask
 *  \note  SD_FR_TX and SD_FR_RX will cause TRC and RRC IRQs respectively when reset is complete
 *
 *  \param  sf    Pointer to sd_fifo struct
 *  \param  mask  Bitmask of SD_FR_* bits
 */
void sd_fifo_reset (struct sd_fifo *sf, int mask);


/** Setup RX or TX direction struct
 *
 *  \param  fd       Pointer to sd_fifo_dir struct
 *  \param  func     Pointer to callback function
 *  \param  timeout  Timeout for DMA operations in jiffies
 */
void sd_fifo_init_tx (struct sd_fifo *sd, sd_tx_callback func, unsigned long timeout);
void sd_fifo_init_rx (struct sd_fifo *sd, sd_rx_callback func, unsigned long timeout);


/** Enqueue a burst of TX descriptors to the tail of the TX queue, and start TX if the
 *  queue was empty
 *
 *  \note  Takes &sf->tx.lock, caller should not hold the lock
 *
 *  \note  For bursts with num > 1, the cookie value returned will be assigned to the
 *         first descriptor and passed to the tx.func if registered.  The cookie value
 *         will increment for each subsequent descriptor.  Thus a burst of 3 descriptors
 *         may return a cookie value of 17, and tx.func will be called three times with
 *         cookie values 17, 18, and 19 as the TX burst completes.
 *
 *  \param  sf    Pointer to sd_fifo struct
 *  \param  desc  Array of sd_desc struct pointers
 *  \param  num   Length of descriptor pointer array
 *
 *  \return  An opaque cookie for the TX transaction, which will be passed to tx.func
 */
unsigned sd_fifo_tx_burst (struct sd_fifo *sf, struct sd_desc **desc, int num);


/** Enqueue a TX descriptor at the tail of the TX queue, and start TX if the queue was
 *  empty
 *
 *  \note  Takes &sf->tx.lock, caller should not hold the lock
 *
 *  \param  sf    Pointer to sd_fifo struct
 *  \param  desc  Pointer to sd_desc struct
 *
 *  \return  An opaque cookie for the TX transaction, which will be passed to tx.func
 */
static inline unsigned sd_fifo_tx_enqueue (struct sd_fifo *sf, struct sd_desc *desc)
{
	return sd_fifo_tx_burst(sf, &desc, 1);
}

/** Flush all TX descriptors
 *
 *  \param  sf  Pointer to sd_fifo struct
 */
void sd_fifo_tx_discard (struct sd_fifo *sf);


#endif /* _INCLUDE_SD_FIFO_H_ */
