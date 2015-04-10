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

#include "sd_fifo.h"
#include "sd_desc.h"


struct srio_dev
{
	struct device     *dev;
	struct rio_mport   mport;
	struct rio_ops     ops;

	struct sd_fifo    *init_fifo;
	struct sd_desc    *init_ring;
	size_t             init_size;

	struct sd_fifo    *targ_fifo;
	struct sd_desc    *targ_ring;
	size_t             targ_size;

	uint32_t           devid;

	/* Mapped pointer for the maintenance registers */
	void __iomem  *maint;

	/* Mapped pointer for the SYS_REG registers */
	struct sd_sys_reg __iomem  *sys_regs;
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


#endif /* _INCLUDE_SRIO_DEV_H */
