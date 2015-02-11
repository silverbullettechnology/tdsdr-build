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


#include "srio_dev.h"
#include "sd_desc.h"


/* FIFO config, filled in by xparameters for memory addresses and IRQ numbers, passed to
 * sd_fifo_init().  Temporary, to be replaced by a device-tree child node in the end */
struct sd_fifo_config
{
	resource_size_t  regs;
	resource_size_t  data;
	int              irq;
	int              dma;
};
extern struct sd_fifo_config sd_dsxx_fifo_cfg;


/* AXI4-Stream FIFO registers */
struct sd_fifo_regs
{
	uint32_t  isr;      /* 0x00: RC: Interrupt Status Register                      */
	uint32_t  ier;      /* 0x04: RW: Interrupt Enable Register                      */
	uint32_t  tdfr;     /* 0x08: WO: Transmit Data FIFO Reset                       */
	uint32_t  tdfv;     /* 0x0C: RO: Transmit Data FIFO Vacancy                     */
	uint32_t  tdfd;     /* 0x10: WO: Transmit Data FIFO 32-bit Wide Data Write Port */
	uint32_t  tlr;      /* 0x14: WO: Transmit Length Register                       */
	uint32_t  rdfr;     /* 0x18: WO: Receive Data FIFO reset                        */
	uint32_t  rdfo;     /* 0x1C: RO: Receive Data FIFO Occupancy                    */
	uint32_t  rdfd;     /* 0x20: RO: Receive Data FIFO 32-bit Wide Data Read Port   */
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


struct sd_fifo;


/* Callback: each desc will be detached from its queue on TX or RX complete and passed to
 * the registered callback.  RX callback is responsible for dispatching the message and 
 * enqueueing a replacement (which may be the same desc).  TX callback is responsible for
 * freeing the desc. */
typedef void (* sd_callback) (struct sd_fifo *fifo, struct sd_desc *desc);


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
	sd_callback           func;
	struct dma_chan      *chan;
	dma_addr_t            phys;
	dma_cookie_t          cookie;
	struct timer_list     timer;
	unsigned long         timeout;
	struct list_head      queue;
	struct sd_fifo_stats  stats;
};

struct sd_fifo
{
	/* Note: sd_fifo_regs always maps to an AXI-Lite interface for register access, but 
	 * sd_fifo_data may point to an AXI-Full interface for burst data. */
	struct sd_fifo_regs __iomem *regs;
	struct sd_fifo_regs __iomem *data;
	dma_addr_t                   phys;

	struct sd_fifo_config        cfg;

	struct sd_fifo_dir           rx;
	struct sd_fifo_dir           tx;

	struct sd_desc               rx_ring[RX_RING_SIZE];
};


/** Init FIFO
 *
 *  \param  sf   Pointer to sd_fifo struct
 *  \param  cfg  Fifo config structure
 *
 *  \return  0 on success, <0 on error
 */
int sd_fifo_init (struct sd_fifo *sf, struct sd_fifo_config *cfg);


/** Exit FIFO
 *
 *  \param  sf   Pointer to sd_fifo struct
 */
void sd_fifo_exit (struct sd_fifo *sf);


/** Setup RX or TX direction struct
 *
 *  \param  fd       Pointer to sd_fifo_dir struct
 *  \param  func     Pointer to callback function
 *  \param  timeout  Timeout for DMA operations in jiffies
 */
void sd_fifo_init_dir (struct sd_fifo_dir *fd, sd_callback func, unsigned long timeout);


/** Enqueue a TX descriptor at the tail of the TX queue, and start TX if the queue was
 *  empty
 *
 *  \note  Takes &sf->tx.lock, caller should not hold the lock
 *
 *  \param  sf    Pointer to sd_fifo struct
 *  \param  desc  Pointer to sd_desc struct
 */
void sd_fifo_tx_enqueue (struct sd_fifo *sf, struct sd_desc *desc);


/** Enqueue an RX descriptor at the tail of the RX queue, so it's available for RX
 *
 *  \note  Takes &sf->rx.lock, caller should not hold the lock
 *
 *  \param  sf    Pointer to sd_fifo struct
 *  \param  desc  Pointer to sd_desc struct
 */
void sd_fifo_rx_enqueue (struct sd_fifo *sf, struct sd_desc *desc);


#endif /* _INCLUDE_SD_FIFO_H_ */
