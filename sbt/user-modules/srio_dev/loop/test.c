/** \file      loop/test.c
 *  \brief     Kernelspace test of SRIO loopback
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
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/dmaengine.h>
#include <linux/uaccess.h>

#include "srio_dev.h"
#include "loop/test.h"
#include "sd_regs.h"


/******* Exported vars *******/


struct sd_fifo         sd_loop_init_fifo;
struct sd_fifo         sd_loop_targ_fifo;
struct sd_fifo         sd_loop_user_fifo;
struct sd_fifo_config  sd_loop_init_fifo_cfg;
struct sd_fifo_config  sd_loop_targ_fifo_cfg;
struct sd_fifo_config  sd_loop_user_fifo_cfg;
struct sd_srio_config  sd_srio_cfg;



/******* Static vars *******/




/******* Test code *******/



struct device *sd_loop_init (void)
{
	pr_debug("%s starts:\n", __func__);
	pr_debug("  Init FIFO: regs %08x data %08x irq %d\n", sd_loop_init_fifo_cfg.regs,
	         sd_loop_init_fifo_cfg.data, sd_loop_init_fifo_cfg.irq);
	pr_debug("  Targ FIFO: regs %08x data %08x irq %d\n", sd_loop_targ_fifo_cfg.regs,
	         sd_loop_targ_fifo_cfg.data, sd_loop_targ_fifo_cfg.irq);
	pr_debug("  User FIFO: regs %08x data %08x irq %d\n", sd_loop_user_fifo_cfg.regs,
	         sd_loop_user_fifo_cfg.data, sd_loop_user_fifo_cfg.irq);

	pr_debug("  SRIO Core: regs %08x addr %08x\n",
	         sd_srio_cfg.regs, sd_srio_cfg.addr);

	return NULL;
}


void sd_loop_exit (void)
{
}

