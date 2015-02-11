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
#include "sd_xparameters.h"
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

static u32 *sd_loop_addr_base = NULL;
static u32  sd_loop_init_reg;
static u32  sd_loop_targ_reg;

static struct sd_desc  sd_loop_init_rx_ring[RX_RING_SIZE];
static struct sd_desc  sd_loop_targ_rx_ring[RX_RING_SIZE];
static struct sd_desc  sd_loop_user_rx_ring[RX_RING_SIZE];



/******* Test code *******/

void sd_loop_set_src_dest_reg (u32 init_reg, u32 targ_reg)
{
	sd_loop_init_reg = init_reg;
	sd_loop_targ_reg = targ_reg;

}




static void sd_loop_rx_done (struct sd_fifo *fifo, struct sd_desc *desc)
{
	printk("%s: RX desc %p used %08x\n", __func__, desc, desc->used);

	// give buffer back to RX ring
	sd_fifo_rx_enqueue(fifo, desc);
}

void sd_loop_tx_done (struct sd_fifo *fifo, struct sd_desc *desc)
{
	printk("%s: TX desc %p done\n", __func__, desc);
}



/******* VFS glue & init/exit *******/


static int sd_loop_open (struct inode *i, struct file *f)
{
	return 0;
}

static int sd_loop_release (struct inode *i, struct file *f)
{
	return 0;
}

static struct file_operations sd_loop_fops = 
{
	open:    sd_loop_open,
	release: sd_loop_release,
};

static struct miscdevice sd_loop_mdev =
{
	MISC_DYNAMIC_MINOR,
	SD_DRIVER_NODE,
	&sd_loop_fops
};

struct device *sd_loop_init (void)
{
	int idx, ret;

	pr_debug("%s starts:\n", __func__);
	pr_debug("  Init FIFO: regs %08x data %08x irq %d\n", sd_loop_init_fifo_cfg.regs,
	         sd_loop_init_fifo_cfg.data, sd_loop_init_fifo_cfg.irq);
	pr_debug("  Targ FIFO: regs %08x data %08x irq %d\n", sd_loop_targ_fifo_cfg.regs,
	         sd_loop_targ_fifo_cfg.data, sd_loop_targ_fifo_cfg.irq);
	pr_debug("  User FIFO: regs %08x data %08x irq %d\n", sd_loop_user_fifo_cfg.regs,
	         sd_loop_user_fifo_cfg.data, sd_loop_user_fifo_cfg.irq);
	pr_debug("  SRIO Core: regs %08x addr %08x\n",
	         sd_srio_cfg.regs, sd_srio_cfg.addr);

	if ( (ret = sd_fifo_init(&sd_loop_init_fifo, &sd_loop_init_fifo_cfg)) )
	{
		pr_err("sd_fifo_init() for initiator failed: %d\n", ret);
		return NULL;
	}
	sd_fifo_init_dir(&sd_loop_init_fifo.tx, sd_loop_tx_done, HZ);
	sd_fifo_init_dir(&sd_loop_init_fifo.rx, sd_loop_rx_done, HZ);

	if ( (ret = sd_fifo_init(&sd_loop_targ_fifo, &sd_loop_targ_fifo_cfg)) )
	{
		pr_err("sd_fifo_init() for target failed: %d\n", ret);
		goto fifo_init;
	}
	sd_fifo_init_dir(&sd_loop_targ_fifo.tx, sd_loop_tx_done, HZ);
	sd_fifo_init_dir(&sd_loop_targ_fifo.rx, sd_loop_rx_done, HZ);

	if ( (ret = sd_fifo_init(&sd_loop_user_fifo, &sd_loop_user_fifo_cfg)) )
	{
		pr_err("sd_fifo_init() for user failed: %d\n", ret);
		goto fifo_targ;
	}
	sd_fifo_init_dir(&sd_loop_user_fifo.tx, sd_loop_tx_done, HZ);
	sd_fifo_init_dir(&sd_loop_user_fifo.rx, sd_loop_rx_done, HZ);

	if ( !(sd_loop_addr_base = sd_iomap(sd_srio_cfg.addr, PAGE_SIZE)) )
	{
		pr_err("sd_iomap() for addr regs failed\n");
		goto fifo_user;
	}

	if ( (ret = misc_register(&sd_loop_mdev)) < 0 )
	{
		pr_err("misc_register() failed\n");
		goto addr_base;
	}

	ret = sd_desc_setup_ring(sd_loop_targ_rx_ring, RX_RING_SIZE, BUFF_SIZE,
	                         sd_loop_mdev.this_device,
	                         sd_loop_targ_fifo_cfg.dma & (1 << DMA_DEV_TO_MEM));
	if ( ret ) 
	{
		pr_err("sd_desc_setup_ring() for target failed: %d\n", ret);
		goto misc_dev;
	}

	ret = sd_desc_setup_ring(sd_loop_init_rx_ring, RX_RING_SIZE, BUFF_SIZE,
	                         sd_loop_mdev.this_device,
	                         sd_loop_init_fifo_cfg.dma & (1 << DMA_DEV_TO_MEM));
	if ( ret ) 
	{
		pr_err("sd_desc_setup_ring() for initator failed: %d\n", ret);
		goto targ_ring;
	}

	ret = sd_desc_setup_ring(sd_loop_user_rx_ring, RX_RING_SIZE, BUFF_SIZE,
	                         sd_loop_mdev.this_device,
	                         sd_loop_user_fifo_cfg.dma & (1 << DMA_DEV_TO_MEM));
	if ( ret ) 
	{
		pr_err("sd_desc_setup_ring() for user failed: %d\n", ret);
		goto init_ring;
	}

	for ( idx = 0; idx < RX_RING_SIZE; idx++ )
	{
		sd_loop_init_rx_ring[idx].info = idx;
		sd_fifo_rx_enqueue(&sd_loop_init_fifo, &sd_loop_init_rx_ring[idx]);

		sd_loop_targ_rx_ring[idx].info = idx;
		sd_fifo_rx_enqueue(&sd_loop_targ_fifo, &sd_loop_targ_rx_ring[idx]);

		sd_loop_user_rx_ring[idx].info = idx;
		sd_fifo_rx_enqueue(&sd_loop_user_fifo, &sd_loop_user_rx_ring[idx]);
	}


	// success
	return sd_loop_mdev.this_device;


init_ring:
	sd_desc_clean_ring(sd_loop_init_rx_ring, RX_RING_SIZE, sd_loop_mdev.this_device,
	                   sd_loop_init_fifo_cfg.dma & (1 << DMA_DEV_TO_MEM));

targ_ring:
	sd_desc_clean_ring(sd_loop_targ_rx_ring, RX_RING_SIZE, sd_loop_mdev.this_device,
	                   sd_loop_targ_fifo_cfg.dma & (1 << DMA_DEV_TO_MEM));

misc_dev:
	misc_deregister(&sd_loop_mdev);

addr_base:
	sd_unmap(sd_loop_addr_base, sd_srio_cfg.addr, PAGE_SIZE);

fifo_user:
	sd_fifo_exit(&sd_loop_user_fifo);

fifo_targ:
	sd_fifo_exit(&sd_loop_targ_fifo);

fifo_init:
	sd_fifo_exit(&sd_loop_init_fifo);

	return NULL;
}


void sd_loop_exit (void)
{
	sd_desc_clean_ring(sd_loop_user_rx_ring, RX_RING_SIZE, sd_loop_mdev.this_device,
	                   sd_loop_user_fifo_cfg.dma & (1 << DMA_DEV_TO_MEM));
	sd_desc_clean_ring(sd_loop_init_rx_ring, RX_RING_SIZE, sd_loop_mdev.this_device,
	                   sd_loop_init_fifo_cfg.dma & (1 << DMA_DEV_TO_MEM));
	sd_desc_clean_ring(sd_loop_targ_rx_ring, RX_RING_SIZE, sd_loop_mdev.this_device,
	                   sd_loop_targ_fifo_cfg.dma & (1 << DMA_DEV_TO_MEM));
	misc_deregister(&sd_loop_mdev);
	sd_unmap(sd_loop_addr_base, sd_srio_cfg.addr, PAGE_SIZE);
	sd_fifo_exit(&sd_loop_user_fifo);
	sd_fifo_exit(&sd_loop_targ_fifo);
	sd_fifo_exit(&sd_loop_init_fifo);
}

