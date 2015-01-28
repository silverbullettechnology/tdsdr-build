/** \file      dsxx/test.c
 *  \brief     Kernelspace test of data source/sink to FIFO
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
#include "dsxx/test.h"
#include "dsxx/proc.h"
#include "dsxx/regs.h"


/******* Exported vars *******/


//static spinlock_t      sd_dsxx_lock;
struct sd_fifo         sd_dsxx_fifo;
struct sd_fifo_config  sd_dsxx_fifo_cfg;

uint32_t  sd_dsxx_rx_length    = 4096;
uint32_t  sd_dsxx_rx_pause_on  = 0;
uint32_t  sd_dsxx_rx_pause_off = 0;
uint32_t  sd_dsxx_rx_reps      = 1;

uint32_t  sd_dsxx_tx_length    = 256;
uint32_t  sd_dsxx_tx_pause_on  = 0;
uint32_t  sd_dsxx_tx_pause_off = 0;
uint32_t  sd_dsxx_tx_burst     = 0;

struct sd_dsxx_stats  sd_dsxx_rx_stats;
struct sd_dsxx_stats  sd_dsxx_tx_stats;


/******* Static vars *******/


static int                sd_dsxx_users;
static struct device     *sd_dsxx_dev;

static struct sd_desc     sd_dsxx_rx_ring[RX_RING_SIZE];
static struct completion  sd_dsxx_rx_complete;
static int                sd_dsxx_rx_reps_left;
//static int                sd_dsxx_rx_slot;

static struct sd_desc     sd_dsxx_tx_desc[TX_RING_SIZE];
static struct completion  sd_dsxx_tx_complete;
//static int                sd_dsxx_tx_slot;


/******* Test code *******/


static int sd_dsxx_open (struct inode *i, struct file *f)
{
//	unsigned long  flags;
	int            ret = 0;

	printk("%s starts\n", __func__);
	//spin_lock_irqsave(&sd_dsxx_lock, flags);
	if ( sd_dsxx_users )
		ret = -EACCES;
	else
	{
		sd_dsxx_users   = 1;
		sd_dsrc_regs_ctrl(SD_DSXX_CTRL_RESET);
		sd_dsnk_regs_ctrl(SD_DSXX_CTRL_RESET);
	}
	//spin_unlock_irqrestore(&sd_dsxx_lock, flags);

	printk("%s: %d\n", __func__, ret);
	return ret;
}


/******* RX testing *******/


void sd_dsxx_rx_check (void *buf, size_t len)
{
	const uint32_t *ptr = buf;
	uint32_t        val = 0;
	size_t          idx = 0;
	uint32_t        cnt = 0;

	pr_debug("%s(buff %p, size %08zx):\n", __func__, ptr, len);
	len /= sizeof(uint32_t);

	while ( idx < len )
	{
		if ( *ptr != val )
		{
			if ( cnt < 10 )
				printk("Idx %08x: want %08x, got %08x instead\n", idx, val, *ptr);
			val = *ptr;
			cnt++;
		}
		ptr++;
		val++;
		idx++;
	}
	if ( cnt > 10 )
		printk("%u lines suppressed\n", cnt - 10);
	printk("%u deviations\n", cnt);
	if ( cnt )
		sd_dsxx_rx_stats.errors++;
	else
		sd_dsxx_rx_stats.valids++;
}

static void sd_dsxx_rx_done (struct sd_desc *desc)
{
	printk("%s: RX desc %p used %08x, left %d\n", __func__,
	       desc, desc->used, sd_dsxx_rx_reps_left);

	sd_dsxx_rx_check(desc->virt, desc->used);

	// give buffer back to RX ring
	sd_fifo_rx_enqueue(&sd_dsxx_fifo, desc);

	//sd_dsxx_rx_slot = slot;
	sd_dsxx_rx_reps_left--;
	if ( !sd_dsxx_rx_reps_left )
		complete(&sd_dsxx_rx_complete);
}

void sd_dsxx_rx_trigger (void)
{
	unsigned long timeout;
	//sd_dsxx_rx_slot = -1;

	if ( sd_dsrc_pause )
	{
		// Reset, program, enable pause module
		if ( sd_dsxx_rx_pause_on || sd_dsxx_rx_pause_off )
		{
			printk("DSRC PAUSE set for %u on, %u off\n", sd_dsxx_rx_pause_on, sd_dsxx_rx_pause_off);
			sd_dsrc_pause_ctrl(SD_DSXX_PAUSE_CTRL_RESET);
			sd_dsrc_pause_on(sd_dsxx_rx_pause_on);
			sd_dsrc_pause_off(sd_dsxx_rx_pause_off);
			sd_dsrc_pause_ctrl(SD_DSXX_PAUSE_CTRL_ENABLE);
		}
		// Disable pause module
		else
		{
			printk("DSRC PAUSE disabled\n");
			sd_dsrc_pause_ctrl(SD_DSXX_PAUSE_CTRL_RESET);
			sd_dsrc_pause_ctrl(SD_DSXX_PAUSE_CTRL_DISABLE);
		}
	}

	// Reset, then setup packet generator
	printk("Reset DSRC, set for %08x bytes\n", sd_dsxx_rx_length);
	sd_dsrc_regs_ctrl(SD_DSXX_CTRL_RESET);
	sd_dsrc_regs_bytes(sd_dsxx_rx_length);
	sd_dsrc_regs_reps(sd_dsxx_rx_reps);
	sd_dsrc_regs_type(SD_DSRC_TYPE_INCREMENT);
	sd_dsxx_rx_reps_left = sd_dsxx_rx_reps;

	// enable packet generator
	printk("Start DSRC...\n");
	sd_dsxx_rx_stats.starts++;
	sd_dsrc_regs_ctrl(SD_DSXX_CTRL_ENABLE);

	// Wait for packet to complete or timeout
	if ( (timeout = wait_for_completion_timeout(&sd_dsxx_rx_complete, HZ)) )
		pr_debug("RX complete\n");
	else
	{
		pr_err("RX timed out\n");
		sd_dsxx_rx_stats.timeouts++;
	}

	// disable packet generator
	sd_dsrc_regs_ctrl(SD_DSXX_CTRL_DISABLE);
	sd_dsrc_regs_dump();
}


/******* TX testing *******/


static uint64_t sd_dsxx_tx_sum (uint64_t sum, const void *buff, size_t size)
{
	const uint32_t *walk = buff;
	int             cry;

	while ( size )
	{
		cry   = sum & 0x0000000000000001;
		sum >>= 1;
		sum  &= 0x7FFFFFFFFFFFFFFF;
		if ( cry )
			sum |= 0x8000000000000000;
		sum += *walk;
		size -= sizeof(uint32_t);
		walk++;
	}

	return sum;
}


void sd_dsxx_tx_done (struct sd_desc *desc)
{
//	sd_dsxx_tx_slot = slot;
	complete(&sd_dsxx_tx_complete);
}


ssize_t sd_dsxx_write (struct file *f, const char __user *user, size_t size, loff_t *ofs)
{
	uint8_t *dest;

	printk("%s(f, user %p, size %zu, ofs %lld)\n", __func__, user, size, *ofs);

	// late alloc TX buffer
	if ( !sd_dsxx_tx_desc[0].virt )
	{
		int i;

		if ( sd_desc_alloc(&sd_dsxx_tx_desc[0], BUFF_SIZE) )
		{
			pr_err("Memory alloc failed\n");
			return -ENOMEM;
		}

		for ( i = 1; i <= sd_dsxx_tx_burst; i++ )
			if ( sd_desc_alloc(&sd_dsxx_tx_desc[i], BUFF_SIZE) )
			{
				pr_err("Memory alloc failed\n");
				return -ENOMEM;
			}

		sd_dsxx_tx_desc[0].used = 0;
		printk("%s allocated sd_dsxx_tx_desc[0].virt %p\n", __func__, sd_dsxx_tx_desc[0].virt);
	}

	if ( (sd_dsxx_tx_desc[0].used + size) > BUFF_SIZE )
		return -EINVAL;

	dest = sd_dsxx_tx_desc[0].virt + sd_dsxx_tx_desc[0].used;
	if ( copy_from_user(dest, user, size) )
		return -EFAULT;

	sd_dsxx_tx_desc[0].used += size;
	printk("%s sd_dsxx_tx_desc[0].used %zu\n", __func__, sd_dsxx_tx_desc[0].used);
	return size;
}


static int sd_dsxx_release (struct inode *i, struct file *f)
{
	unsigned long  timeout;
	int            ret = 0;

	printk("%s starts\n", __func__);
	//spin_lock_irqsave(&sd_dsxx_lock, flags);
	if ( !sd_dsxx_users )
		ret = -EBADF;

	printk("%s: sd_dsxx_tx_desc[0].virt %p, used %zu\n", __func__,
	       sd_dsxx_tx_desc[0].virt, sd_dsxx_tx_desc[0].used);
	if ( sd_dsxx_tx_desc[0].virt )
	{
		uint64_t  want,  have;
		int       i;

		printk("%s: had %d bytes in buff at release\n", __func__, sd_dsxx_tx_desc[0].used);

		if ( sd_dsxx_tx_desc[0].used & 0x0000003 )
		{
			sd_dsxx_tx_desc[0].used += 3;
			sd_dsxx_tx_desc[0].used &= 0xFFFFFFC;
			printk("  used rounded up to %d bytes\n", sd_dsxx_tx_desc[0].used);
		}

		if ( (sd_dsxx_fifo_cfg.dma & (1 << DMA_MEM_TO_DEV)) &&
		     sd_desc_map(&sd_dsxx_tx_desc[0], sd_dsxx_dev, DMA_MEM_TO_DEV) )
		{
			pr_err("DMA map failed\n");
			sd_desc_free(&sd_dsxx_tx_desc[0]);
			return -ENOMEM;
		}

		for ( i = 1; i <= sd_dsxx_tx_burst; i++ )
			if ( sd_dsxx_tx_desc[i].virt )
			{
				memcpy(sd_dsxx_tx_desc[i].virt, sd_dsxx_tx_desc[0].virt, sd_dsxx_tx_desc[0].used);
				sd_dsxx_tx_desc[i].used = sd_dsxx_tx_desc[0].used;
				pr_debug("Copied %d bytes from desc[0] to desc[%d]\n",
				         sd_dsxx_tx_desc[0].used, i);

				if ( (sd_dsxx_fifo_cfg.dma & (1 << DMA_MEM_TO_DEV)) &&
				     sd_desc_map(&sd_dsxx_tx_desc[i], sd_dsxx_dev, DMA_MEM_TO_DEV) )
				{
					pr_err("DMA map failed\n");
					return -ENOMEM;
				}
				pr_debug("Mapped desc[%d] to %08x\n", i, sd_dsxx_tx_desc[0].phys);
			}

		want = 0;
		for ( i = 0; i < TX_RING_SIZE; i++ )
			if ( sd_dsxx_tx_desc[i].virt )
				want = sd_dsxx_tx_sum(want, sd_dsxx_tx_desc[i].virt, sd_dsxx_tx_desc[i].used);

		if ( sd_dsnk_pause )
		{
			// Reset, program, enable pause module
			if ( sd_dsxx_tx_pause_on || sd_dsxx_tx_pause_off )
			{
				printk("DSNK PAUSE set for %u on, %u off\n", sd_dsxx_tx_pause_on, sd_dsxx_tx_pause_off);
				sd_dsnk_pause_ctrl(SD_DSXX_PAUSE_CTRL_RESET);
				sd_dsnk_pause_on(sd_dsxx_tx_pause_on);
				sd_dsnk_pause_off(sd_dsxx_tx_pause_off);
				sd_dsnk_pause_ctrl(SD_DSXX_PAUSE_CTRL_ENABLE);
			}
			// Disable pause module
			else
			{
				printk("DSNK PAUSE disabled\n");
				sd_dsnk_pause_ctrl(SD_DSXX_PAUSE_CTRL_RESET);
				sd_dsnk_pause_ctrl(SD_DSXX_PAUSE_CTRL_DISABLE);
			}
		}

		// Reset, then enable data sink
		sd_dsnk_regs_ctrl(SD_DSXX_CTRL_RESET);
		sd_dsnk_regs_ctrl(SD_DSXX_CTRL_ENABLE);

		// transmit packet
		sd_dsxx_tx_stats.starts++;
		for ( i = 0; i < TX_RING_SIZE; i++ )
			if ( sd_dsxx_tx_desc[i].virt )
			{
				pr_debug("Enqueue desc[%d]\n", i);
				sd_fifo_tx_enqueue(&sd_dsxx_fifo, &sd_dsxx_tx_desc[i]);
			}

		// Wait for packet to complete or timeout
		if ( (timeout = wait_for_completion_timeout(&sd_dsxx_tx_complete, HZ)) )
			pr_debug("TX complete\n");
		else
		{
			pr_err("TX timed out\n");
			sd_dsxx_tx_stats.timeouts++;
		}

		// disable data sink
		sd_dsnk_regs_ctrl(SD_DSXX_CTRL_DISABLE);

		if ( sd_dsxx_fifo_cfg.dma & (1 << DMA_MEM_TO_DEV) )
			for ( i = 0; i < TX_RING_SIZE; i++ )
				if ( sd_dsxx_tx_desc[i].phys )
				{
					sd_desc_unmap(&sd_dsxx_tx_desc[i], sd_dsxx_dev, DMA_MEM_TO_DEV);
					pr_debug("Unmapped desc[%d]\n", i);
				}

		sd_dsrc_regs_dump();
		have = sd_dsnk_regs_sum64();

		printk("have: %016llx\n", have);
		printk("want: %016llx\n", want);
		if ( timeout )
		{
			if ( have == want )
				sd_dsxx_tx_stats.valids++;
			else
				sd_dsxx_tx_stats.errors++;
		}

		for ( i = 0; i < TX_RING_SIZE; i++ )
			if ( sd_dsxx_tx_desc[i].virt )
			{
				sd_desc_free(&sd_dsxx_tx_desc[i]);
				pr_debug("Freed desc[%d]\n", i);
			}
	}
	
	sd_dsxx_users = 0;
	//spin_unlock_irqrestore(&sd_dsxx_lock, flags);

	return ret;
}

static struct file_operations sd_dsxx_fops = 
{
	open:    sd_dsxx_open,
	write:   sd_dsxx_write,
	release: sd_dsxx_release,
};

static struct miscdevice sd_dsxx_mdev =
{
	MISC_DYNAMIC_MINOR,
	SD_DRIVER_NODE,
	&sd_dsxx_fops
};

struct device *sd_dsxx_init (void)
{
	int  idx;
	int  ret = 0;

	if ( (ret = misc_register(&sd_dsxx_mdev)) < 0 )
	{
		pr_err("misc_register() failed\n");
		return NULL;
	}

	if ( (ret = sd_fifo_init(&sd_dsxx_fifo, &sd_dsxx_fifo_cfg)) )
	{
		pr_err("sd_fifo_init() failed, stop\n");
		goto unreg;
	}
	sd_fifo_init_dir(&sd_dsxx_fifo.tx, sd_dsxx_tx_done, HZ);
	sd_fifo_init_dir(&sd_dsxx_fifo.rx, sd_dsxx_rx_done, HZ);

	for ( idx = 0; idx < RX_RING_SIZE; idx++ )
		if ( sd_desc_alloc(&sd_dsxx_rx_ring[idx], BUFF_SIZE) )
		{
			pr_err("RX slot %d sd_desc_alloc() failed, stop\n", idx);
			goto unfifo;
		}
		else
		{
			if ( (sd_dsxx_fifo_cfg.dma & (1 << DMA_DEV_TO_MEM)) &&
			     sd_desc_map(&sd_dsxx_rx_ring[idx], sd_dsxx_dev, DMA_DEV_TO_MEM) )
			{
				pr_err("RX slot %d sd_desc_map() failed, stop\n", idx);
				goto unfifo;
			}

			sd_dsxx_rx_ring[idx].info = idx;
			sd_fifo_rx_enqueue(&sd_dsxx_fifo, &sd_dsxx_rx_ring[idx]);
		}

	if ( (ret = sd_dsxx_proc_init()) )
	{
		pr_err("Failed to init /proc, stop\n");
		goto unfifo;
	}

//	if ( want > 0 )
//		sd_dsxx_rx_length = want;
	if ( sd_dsxx_rx_length & 0x03 )
	{
		sd_dsxx_rx_length += 3;
		sd_dsxx_rx_length &= 0xFFFFFFFC;
	}
	if ( sd_dsxx_rx_length > BUFF_SIZE )
		sd_dsxx_rx_length = BUFF_SIZE;
	pr_info("RX want set to %d\n", sd_dsxx_rx_length);

	init_completion(&sd_dsxx_rx_complete);
	init_completion(&sd_dsxx_tx_complete);

	memset(&sd_dsxx_tx_stats, 0, sizeof(struct sd_dsxx_stats));
	memset(&sd_dsxx_tx_stats, 0, sizeof(struct sd_dsxx_stats));

	sd_dsxx_dev = sd_dsxx_mdev.this_device;
	return sd_dsxx_mdev.this_device;

unfifo:
	for ( idx = 0; idx < RX_RING_SIZE; idx++ )
	{
		if ( sd_dsxx_fifo_cfg.dma & (1 << DMA_DEV_TO_MEM) )
			sd_desc_unmap(&sd_dsxx_rx_ring[idx], sd_dsxx_dev, DMA_DEV_TO_MEM);
		sd_desc_free(&sd_dsxx_rx_ring[idx]);
	}
	sd_fifo_exit(&sd_dsxx_fifo);

unreg:
	misc_deregister(&sd_dsxx_mdev);

	return NULL;
}


void sd_dsxx_exit (void)
{
	int idx;

	sd_dsxx_proc_exit();

	for ( idx = 0; idx < RX_RING_SIZE; idx++ )
	{
		if ( sd_dsxx_fifo_cfg.dma & (1 << DMA_DEV_TO_MEM) )
			sd_desc_unmap(&sd_dsxx_rx_ring[idx], sd_dsxx_dev, DMA_DEV_TO_MEM);
		sd_desc_free(&sd_dsxx_rx_ring[idx]);
	}

	sd_fifo_exit(&sd_dsxx_fifo);

	misc_deregister(&sd_dsxx_mdev);
}


