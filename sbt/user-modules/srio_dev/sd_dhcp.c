/** \file      sd_dhcp.c
 *  \brief     Dynamic device-ID probe using loopback
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
#include <linux/random.h>

#include "srio_dev.h"
#include "sd_user.h"
#include "sd_regs.h"
#include "sd_dhcp.h"


static void sd_dhcp_tick (unsigned long param)
{
	struct srio_dev *sd = (struct srio_dev *)param;
	struct sd_desc  *desc;

	// if descriptor alloc fails re-arm for long delay
	if ( !(desc = sd_desc_alloc(sd, GFP_ATOMIC|GFP_DMA)) )
	{
		pr_err("%s: desc alloc failed, retry later\n", dev_name(sd->dev));
		mod_timer(&sd->dhcp.timer, jiffies + (HZ * 5)); // 5sec delay
		return;
	}

	// format and send message
	// TODO: can we use short-message with a larger mbox number?
	pr_debug("format ping for %u\n", sd->dhcp.cur);
	desc->virt[0]  = sd->dhcp.cur << 16;
	desc->virt[0] |= sd->dhcp.cur;
	desc->virt[1]  = sd->dhcp.rep & 3;
	desc->virt[1] |= 0x3F << 4;
	desc->virt[2]  = 0x00B00000;
	desc->virt[2] |= sizeof(sd->dhcp.rand) << 4;
	memcpy(&desc->virt[SD_HEAD_SIZE], sd->dhcp.rand, sizeof(sd->dhcp.rand));
	desc->used = sizeof(sd->dhcp.rand) + 12;
	pr_debug("%s: send message:\n", dev_name(sd->dev));
	hexdump_buff(desc->virt, desc->used);
	sd_fifo_tx_enqueue(sd->init_fifo, desc);

	// increment counters and re-arm timer based on in-cycle or per-cycle time
	if ( sd->dhcp.cur < sd->dhcp.max )
	{
		sd->dhcp.cur++;
		mod_timer(&sd->dhcp.timer, jiffies + (HZ / 5)); // 200ms delay
		pr_debug("short timer rearm for cur %u\n", sd->dhcp.cur);
		return;
	}
	else if ( sd->dhcp.rep )
	{
		sd->dhcp.cur = sd->dhcp.min;
		sd->dhcp.rep--;
		mod_timer(&sd->dhcp.timer, jiffies + (HZ * 5)); // 5sec delay
		pr_debug("long timer rearm for rep %u\n", sd->dhcp.rep);
		return;
	}

	pr_err("%s: out of retries, please set device-ID manually\n", dev_name(sd->dev));
}


void sd_dhcp_recv (struct sd_fifo *fifo, struct sd_desc *desc)
{
	struct srio_dev *sd = fifo->sd;

	pr_debug("%s: recv message: %s: %08x:%08x.%08x\n", fifo->name, __func__,
	         desc->virt[0], desc->virt[2], desc->virt[1]);
	if ( desc->used > 12 )
		hexdump_buff(&desc->virt[SD_HEAD_SIZE], desc->used - 12);

	if ( sd->devid != 0xFFFF )
	{
		pr_debug("shouldn't be here, devid is set\n");
		goto user;
	}

	if ( (desc->virt[0] & 0xFFFF) != (desc->virt[0] >> 16) )
	{
		pr_debug("ignored: not loopback\n");
		goto free;
	}

	// TODO: can we use short-message with a larger mbox number?
	if ( ((desc->virt[2] >> 20) & 0x0F) != 11 || ((desc->virt[1] >> 4) & 0x3F) != 0x3F )
	{
		pr_debug("ignored: not MESSAGE to mbox 3 (%u / %u)\n",
		         ((desc->virt[2] >> 20) & 0x0F),
		         ((desc->virt[1] >>  4) & 0x3F));
		goto free;
	}

	if ( desc->used != sizeof(sd->dhcp.rand) + 12 || 
	     memcmp(&desc->virt[SD_HEAD_SIZE], sd->dhcp.rand, sizeof(sd->dhcp.rand)) )
	{
		pr_debug("ignored: cookie mismatch:\n");
		hexdump_buff(&desc->virt[SD_HEAD_SIZE], sizeof(sd->dhcp.rand));
		hexdump_buff(sd->dhcp.rand,             sizeof(sd->dhcp.rand));
		goto free;
	}

	// finished, sd_regs_set_devid() also sets sd->devid
	del_timer(&sd->dhcp.timer);
	sd_regs_set_devid(sd, desc->virt[0] & 0xFFFF);
	pr_info("%s: established address 0x%04x, normal service starts\n",
	        dev_name(sd->dev), sd->devid);

user:
	sd_user_attach(sd);

free:
	sd_desc_free(sd, desc);
}


void sd_dhcp_start (struct srio_dev *sd, uint16_t min, uint16_t max, uint16_t rep)
{
	sd->dhcp.min = min;
	sd->dhcp.max = max;
	sd->dhcp.cur = min;
	sd->dhcp.rep = rep;

	get_random_bytes(sd->dhcp.rand, sizeof(sd->dhcp.rand));

	sd_fifo_init_rx(sd->init_fifo, NULL,         HZ);
	sd_fifo_init_rx(sd->targ_fifo, sd_dhcp_recv, HZ);
	sd_fifo_init_tx(sd->init_fifo, NULL,         HZ);
	sd_fifo_init_tx(sd->targ_fifo, NULL,         HZ);

	setup_timer(&sd->dhcp.timer, sd_dhcp_tick, (unsigned long)sd);
	mod_timer(&sd->dhcp.timer, jiffies + (HZ * 5));

	pr_debug("%s: start address detect...\n", dev_name(sd->dev));
}


void sd_dhcp_pause (struct srio_dev *sd)
{
	del_timer(&sd->dhcp.timer);
}

void sd_dhcp_resume (struct srio_dev *sd)
{
	if ( sd->devid == 0xFFFF )
		mod_timer(&sd->dhcp.timer, jiffies + HZ);
}

