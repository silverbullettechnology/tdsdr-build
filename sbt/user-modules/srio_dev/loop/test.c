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
#include <linux/ctype.h>

#include "srio_dev.h"
#include "sd_xparameters.h"
#include "loop/test.h"
#include "loop/gpio.h"
#include "loop/proc.h"
#include "sd_regs.h"


/******* Exported vars *******/


struct sd_fifo         sd_loop_init_fifo;
struct sd_fifo         sd_loop_targ_fifo;
struct sd_fifo         sd_loop_user_fifo;
struct sd_fifo_config  sd_loop_init_fifo_cfg;
struct sd_fifo_config  sd_loop_targ_fifo_cfg;
struct sd_fifo_config  sd_loop_user_fifo_cfg;
struct sd_srio_config  sd_srio_cfg;

int sd_loop_tx_dest;



/******* Static vars *******/

static struct device *sd_loop_dev;

static u32 *sd_loop_addr_base = NULL;
static u32  sd_loop_init_reg;
static u32  sd_loop_targ_reg;

static struct sd_desc  sd_loop_init_rx_ring[RX_RING_SIZE];
static struct sd_desc  sd_loop_targ_rx_ring[RX_RING_SIZE];
static struct sd_desc  sd_loop_user_rx_ring[RX_RING_SIZE];
static struct sd_desc  sd_loop_tx_desc;

static struct completion  sd_loop_tx_complete;



/******* Test code *******/

void sd_loop_set_dest_init_reg (u32 init_reg)
{
	sd_loop_init_reg = init_reg;
	REG_WRITE(&sd_loop_addr_base[0], init_reg);
}
u32 sd_loop_get_dest_init_reg (void)
{
	return REG_READ(&sd_loop_addr_base[0]);
}

void sd_loop_set_dest_targ_reg (u32 targ_reg)
{
	sd_loop_targ_reg = targ_reg;
	REG_WRITE(&sd_loop_addr_base[1], targ_reg);
}
u32 sd_loop_get_dest_targ_reg (void)
{
	return REG_READ(&sd_loop_addr_base[1]);
}


static void hexdump_line (const unsigned char *ptr, const unsigned char *org, int len)
{
	char  buff[80];
	char *p = buff;
	int   i;

	p += sprintf (p, "%04x: ", (unsigned)(ptr - org));

	for ( i = 0; i < len; i++ )
		p += sprintf (p, "%02x ", ptr[i]);

	for ( ; i < 16; i++ )
	{
		*p++ = ' ';
		*p++ = ' ';
		*p++ = ' ';
	}

	for ( i = 0; i < len; i++ )
		*p++ = isprint(ptr[i]) ? ptr[i] : '.';
	*p = '\0';

	printk("%s\n", buff);
}

static void hexdump_buff (const unsigned char *buf, int len)
{
	const unsigned char *ptr = buf;

	while ( len >= 16 )
	{
		hexdump_line(ptr, buf, 16);
		ptr += 16;
		len -= 16;
	}
	if ( len )
		hexdump_line(ptr, buf, len);
}

static void sd_loop_rx_done (struct sd_fifo *fifo, struct sd_desc *desc)
{
	printk("%s: RX desc %p used %08x:\n", __func__, desc, desc->used);
	hexdump_buff(desc->virt, desc->used);
	sd_fifo_rx_enqueue(fifo, desc);
}

void sd_loop_tx_done (struct sd_fifo *fifo, struct sd_desc *desc)
{
	printk("%s: TX desc %p done\n", __func__, desc);
	complete(&sd_loop_tx_complete);
}



/******* VFS glue & init/exit *******/


static int sd_loop_open (struct inode *i, struct file *f)
{
	return 0;
}

ssize_t sd_loop_write (struct file *f, const char __user *user, size_t size, loff_t *ofs)
{
	uint8_t *dest;

	printk("%s(f, user %p, size %zu, ofs %lld)\n", __func__, user, size, *ofs);

	// late alloc TX buffer
	if ( !sd_loop_tx_desc.virt )
	{
		u32 *hdr;
		if ( sd_desc_alloc(&sd_loop_tx_desc, BUFF_SIZE) )
		{
			pr_err("Memory alloc failed\n");
			return -ENOMEM;
		}

		// HELLO header (see PG007 Fig 3-1)
		hdr = (u32 *)sd_loop_tx_desc.virt;
		hdr[0] = 0xa0000600; // lsw
		hdr[1] = 0x02602ff9; // msw
		sd_loop_tx_desc.used = 8;

		printk("%s allocated sd_loop_tx_desc.virt %p\n", __func__, sd_loop_tx_desc.virt);
	}

	if ( (sd_loop_tx_desc.used + size) > BUFF_SIZE )
		return -EINVAL;

	dest = sd_loop_tx_desc.virt + sd_loop_tx_desc.used;
	if ( copy_from_user(dest, user, size) )
		return -EFAULT;

	sd_loop_tx_desc.used += size;
	printk("%s sd_loop_tx_desc.used %zu\n", __func__, sd_loop_tx_desc.used);
	return size;
}

static int sd_loop_release (struct inode *i, struct file *f)
{
	unsigned long  timeout;
	int            ret = 0;

	printk("%s: sd_loop_tx_desc.virt %p, used %zu\n", __func__,
	       sd_loop_tx_desc.virt, sd_loop_tx_desc.used);
	if ( sd_loop_tx_desc.virt )
	{
		printk("%s: had %d bytes in buff at release:\n", __func__, sd_loop_tx_desc.used);
		hexdump_buff(sd_loop_tx_desc.virt, sd_loop_tx_desc.used);

		if ( sd_loop_tx_desc.used & 0x0000003 )
		{
			sd_loop_tx_desc.used += 3;
			sd_loop_tx_desc.used &= 0xFFFFFFC;
			printk("  used rounded up to %d bytes\n", sd_loop_tx_desc.used);
		}

		if ( (sd_loop_init_fifo_cfg.dma & (1 << DMA_MEM_TO_DEV)) &&
		     sd_desc_map(&sd_loop_tx_desc, sd_loop_dev, DMA_MEM_TO_DEV) )
		{
			pr_err("DMA map failed\n");
			sd_desc_free(&sd_loop_tx_desc);
			return -ENOMEM;
		}

		// transmit packet depending on switch setting
		switch ( sd_loop_tx_dest )
		{
			case 0:
				pr_debug("Enqueu in Initiator fifo\n");
				sd_fifo_tx_enqueue(&sd_loop_init_fifo, &sd_loop_tx_desc);
				break;

			case 1:
				pr_debug("Enqueu in Target fifo\n");
				sd_fifo_tx_enqueue(&sd_loop_user_fifo, &sd_loop_tx_desc);
				break;

			case 2:
				pr_debug("Enqueu in User fifo\n");
				sd_fifo_tx_enqueue(&sd_loop_targ_fifo, &sd_loop_tx_desc);
				break;
		}

		// Wait for packet to complete or timeout
		if ( (timeout = wait_for_completion_timeout(&sd_loop_tx_complete, HZ)) )
			pr_debug("TX complete\n");
		else
			pr_err("TX timed out\n");

		if ( sd_loop_init_fifo_cfg.dma & (1 << DMA_MEM_TO_DEV) )
			sd_desc_unmap(&sd_loop_tx_desc, sd_loop_dev, DMA_MEM_TO_DEV);

		sd_desc_free(&sd_loop_tx_desc);
	}
	
	return ret;
}

static struct file_operations sd_loop_fops = 
{
	open:    sd_loop_open,
	write:   sd_loop_write,
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

	if ( (ret = sd_loop_gpio_init()) )
	{
		pr_err("sd_loop_gpio_init(): %d\n", ret);
		return NULL;
	}

	if ( (ret = sd_loop_proc_init()) )
	{
		pr_err("sd_loop_proc_init(): %d\n", ret);
		goto gpio;
	}

	if ( (ret = sd_fifo_init(&sd_loop_init_fifo, &sd_loop_init_fifo_cfg)) )
	{
		pr_err("sd_fifo_init() for initiator failed: %d\n", ret);
		goto proc;
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
	sd_loop_set_dest_init_reg(0x00ab0012);
	sd_loop_set_dest_targ_reg(0x003f0081);

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

	init_completion(&sd_loop_tx_complete);

	// reset core
	sd_loop_gpio_set_gt_loopback(0);
	sd_loop_gpio_srio_reset();

	// success
	sd_loop_dev = sd_loop_mdev.this_device;
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

proc:
	sd_loop_proc_exit();

gpio:
	sd_loop_gpio_exit();

	printk("LOOP test init done\n");
	return NULL;
}


void sd_loop_exit (void)
{
	printk("LOOP test exit start\n");
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
	sd_loop_proc_exit();
	sd_loop_gpio_exit();
	printk("LOOP test exit done\n");
}

