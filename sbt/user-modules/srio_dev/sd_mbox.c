/** \file      sd_mbox.h
 *  \brief     Rapidio mbox/type-11 implementation
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
#ifndef _INCLUDE_SD_MBOX_H
#define _INCLUDE_SD_MBOX_H
#include <linux/kernel.h>
#include <linux/rio.h>

#include "srio_dev.h"
#include "sd_mbox.h"
#include "sd_mesg.h"
#include "sd_fifo.h"
#include "sd_desc.h"





struct sd_mesg *sd_mbox_reasm (struct sd_fifo *fifo, struct sd_desc *desc)
{
	hexdump_buff(desc->virt, desc->used);
	sd_desc_free(fifo->sd, desc);
	return NULL;
}


int sd_mbox_frag (struct srio_dev *sd, struct sd_desc **desc, struct sd_mesg *mesg,
                  gfp_t flags)
{
	unsigned  size;
	unsigned  num;
	unsigned  idx;
	void     *src;

	size = mesg->size -
	       offsetof(struct sd_mesg, mesg) -
	       offsetof(struct sd_mesg_mbox, data);
	if ( size < 8 || size > RIO_MAX_MSG_SIZE || (size & 7) )
	{
		pr_err("%s: Bad message size %d (max %u, min 8, multiple of 8)\n", __func__,
		       size, RIO_MAX_MSG_SIZE);
		return -EINVAL;
	}
	// TODO: zero-pad to 8-byte length

	num = (size + (sizeof(uint32_t) * SD_DATA_SIZE) - 1) /
	      (sizeof(uint32_t) * SD_DATA_SIZE);
	if ( num >= 16 )
	{
		pr_err("%s: Message too big (%d frags, max 16)\n", __func__, num);
		return -EFBIG;
	}
	pr_debug("%s: mesg size %d -> %d frags\n", __func__, size, num);

	// setup HELLO header on first fragment.  if the message needs fragmentation, then the
	// size on all descriptors except the last is the chunk size, 256 in this driver, -1.
	// In the last/only descriptor a smaller chunk size is used
	desc[0]->virt[1]  = mesg->mesg.mbox.letter & 3;
	desc[0]->virt[1] |= (mesg->mesg.mbox.mbox & 0x3F) << 4;
	desc[0]->virt[2]  = 0x00B00000;
	desc[0]->virt[2] |= (num - 1) << 28;
	desc[0]->virt[2] |= (min(size, (sizeof(uint32_t) * SD_DATA_SIZE)) - 1) << 4;

	// Payload of first fragment
	src = mesg->mesg.mbox.data;
	memcpy(&desc[0]->virt[SD_HEAD_SIZE], src, min(size, (sizeof(uint32_t) * SD_DATA_SIZE)));
	src  += (sizeof(uint32_t) * SD_DATA_SIZE);
	size -= (sizeof(uint32_t) * SD_DATA_SIZE);

	desc[0]->used = min(size, (sizeof(uint32_t) * SD_DATA_SIZE)) +
	                sizeof(uint32_t) * SD_HEAD_SIZE;

	// alloc first: caller is responsible for allocating and setting up first desc, and
	// freeing any descs allocated in here
	for ( idx = 1; idx < num; idx++ )
	{
		if ( !(desc[idx] = sd_desc_alloc(sd, flags)) )
		{
			pr_err("%s: Failed to alloc fragment %d\n", __func__, idx);
			return -ENOMEM;
		}

		// copy addresses and header from frag 0 and modify
		memcpy(desc[idx]->virt, desc[0]->virt, sizeof(uint32_t) * SD_HEAD_SIZE);
		desc[idx]->virt[2] |= idx << 24;
		if ( size < (sizeof(uint32_t) * SD_DATA_SIZE) )
		{
			desc[idx]->virt[2] &= ~0x00000FF0;
			desc[idx]->virt[2] |= (size - 1) << 4;
		}

		// Payload of fragment
		memcpy(&desc[idx]->virt[SD_HEAD_SIZE], src,
		       min(size, (sizeof(uint32_t) * SD_DATA_SIZE)));

		desc[idx]->used = min(size, (sizeof(uint32_t) * SD_DATA_SIZE)) +
		                sizeof(uint32_t) * SD_HEAD_SIZE;
		src  += (sizeof(uint32_t) * SD_DATA_SIZE);
		size -= (sizeof(uint32_t) * SD_DATA_SIZE);
	}

	return num;
}


int sd_mbox_open_outb_mbox (struct rio_mport *mport, void *dev_id, int mbox,
                            int entries)
{
	printk("sd_mbox_open_outb_mbox(mport %p, dev_id %p, mbox %d, entries %d)\n",
	       mport, dev_id, mbox, entries);

	return -ENOSYS;
}


void sd_mbox_close_outb_mbox (struct rio_mport *mport, int mbox)
{
	printk("sd_mbox_close_outb_mbox(mport %p, mbox %d)\n", mport, mbox);
}


int sd_mbox_open_inb_mbox (struct rio_mport *mport, void *dev_id, int mbox, int entries)
{
	printk("sd_mbox_open_inb_mbox(mport %p, dev_id %p, mbox %d, entries %d)\n",
	       mport, dev_id, mbox, entries);

	return -ENOSYS;
}


void sd_mbox_close_inb_mbox (struct rio_mport *mport, int mbox)
{
	printk("sd_mbox_close_inb_mbox(mport %p, mbox %d)\n", mport, mbox);
}


int sd_mbox_add_outb_message (struct rio_mport *mport, struct rio_dev *rdev, int mbox,
                              void *buffer, size_t len)
{
	printk("sd_mbox_add_outb_message(mport %p, rdev %p, mbox %d, buffer %p, len %zu)\n",
	       mport, rdev, mbox, buffer, len);

	return -ENOSYS;
}


int sd_mbox_add_inb_buffer (struct rio_mport *mport, int mbox, void *buf)
{
	printk("sd_mbox_add_inb_buffer(mport %p, mbox %d, buf %p)\n", mport, mbox, buf);

	return -ENOSYS;
}


void *sd_mbox_get_inb_message (struct rio_mport *mport, int mbox)
{
	printk("sd_mbox_get_inb_message(mport %p, mbox %d)\n", mport, mbox);

	return NULL;
}


#endif /* _INCLUDE_SD_MBOX_H */
