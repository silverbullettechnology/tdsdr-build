/** \file      srio_exp.h
 *  \brief     SRIO experiments harness - for developing the FIFO/DMA primitives for
 *             later use in a proper SRIO driver for Xilinx SRIO in Zynq PL.
 *
 *  \copyright Copyright 2014 Silver Bullet Technologies
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
#ifndef _INCLUDE_SRIO_DEV_H
#define _INCLUDE_SRIO_DEV_H
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/rio.h>
#include <linux/io.h>

#include "sd_mbox.h"
#include "sd_fifo.h"
#include "sd_desc.h"
#include "sd_dhcp.h"


/* Need room for 3 words of header, once that's moved to the desc header this can be
 * reduced to RIO_MAX_MSG_SIZE */
#ifndef BUFF_SIZE
#define BUFF_SIZE 8192
#endif


struct sd_mbox_reasm
{
	uint16_t           bits;
	uint16_t           num;
	uint16_t           len;
	uint16_t           last;
	struct sd_mesg    *mesg;
	struct timer_list  timer;
};


struct srio_dev
{
	struct device     *dev;
	struct rio_mport   mport;
	struct rio_ops     ops;

	struct sd_fifo    *init_fifo;
	struct sd_fifo    *targ_fifo;
	struct kmem_cache *desc_pool;

	uint16_t           devid;
	struct sd_dhcp     dhcp;

	uint8_t            tid;
	uint32_t           pef;

	struct sd_mbox_reasm  mbox_reasm[RIO_MAX_MBOX][4];

	/* RX MESSAGE (type 11): number of listeners and reassembly cache */
	int                mbox_users[RIO_MAX_MBOX];
	struct sd_desc    *mbox_cache[RIO_MAX_MBOX];

	/* RX DBELL (type 10) and SWRITE (type 6): number of listeners */
	int                dbell_users;
	int                swrite_users;

	u32                gt_loopback;
	u32                gt_diffctrl;
	u32                gt_txprecursor;
	u32                gt_txpostcursor;
	u32                gt_rxlpmen;

	/* Status monitoring */
	struct timer_list  status_timer;
	unsigned           status_every;
	unsigned           status_prev;
	unsigned long      status_counts[10];

	/* Try to recover from errors */
	struct timer_list  reset_timer;

	/* Mapped pointer for the maintenance registers */
	void __iomem  *maint;

	/* Mapped pointer for the SYS_REG registers */
	struct sd_sys_reg __iomem  *sys_regs;

	/* Mapped pointer for the DRP registers */
	void __iomem  *drp_regs;

	/* Shadow FIFO may be included in design to allow sniffing parts of the message path
	 * (ie init_fifo -> SRIO core).  If it's not specified in the devtree then no driver
	 * will be started for it, but if the FIFO is included in the PL the software must be
	 * initialized so the FIFO is drained, or it may fill up and stall the stream it's
	 * attached to. */
	struct sd_fifo    *shadow_fifo;
};


#define REG_WRITE(addr, val) \
	do{ \
		/* printk("SET %p, %08x\n", addr, val); */ \
		iowrite32(val, addr); \
	}while(0)

#define REG_READ(addr) \
	({ \
		uint32_t reg = ioread32(addr); \
		/* printk("GET %p: %08x\n", addr, reg); */ \
		reg; \
	})

#define REG_RMW(addr, clr, set) \
	do{ \
		uint32_t reg = ioread32(addr); \
		/* printk("RMW %p: %08x & %08x = ", (addr), reg, ~(clr)); */ \
		reg &= ~(clr); \
		/* printk("%08x | %08x = ", reg, (set)); */ \
		reg |= (set); \
		/* printk("%08x\n", reg); */ \
		iowrite32(reg, (addr)); \
	}while(0)


// temporary, in sd_user
void hexdump_buff (const void *buf, int len);


#endif /* _INCLUDE_SRIO_DEV_H */
