/** \file      sd_recv.c
 *  \brief     Linux Rapidio stack operations
 *
 *  \copyright Copyright 2015 Silver Bullet Technologies
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
#include <linux/ctype.h>

#include "srio_dev.h"
#include "sd_fifo.h"
#include "sd_desc.h"
#include "sd_recv.h"


void sd_recv_init (struct sd_fifo *fifo, struct sd_desc *desc)
{
	printk("%s: RX desc %p used %08x:\n", __func__, desc, desc->used);
	hexdump_buff(desc->virt, desc->used);
	sd_desc_free(fifo->sd, desc);
}


void sd_recv_targ (struct sd_fifo *fifo, struct sd_desc *desc)
{
	printk("%s: RX desc %p used %08x:\n", __func__, desc, desc->used);
	hexdump_buff(desc->virt, desc->used);
	sd_desc_free(fifo->sd, desc);
}



