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

#define pr_trace(...) do{ }while(0)
//#define pr_trace pr_debug



static void sd_mbox_reasm_timeout (unsigned long arg)
{
	struct sd_mbox_reasm *ra = (struct sd_mbox_reasm *)arg;

	pr_err("%s: reassembly timeout\n", __func__);
	sd_mesg_free(ra->mesg);
	memset(ra, 0, sizeof(*ra));
}

struct sd_mesg *sd_mbox_reasm (struct sd_fifo *fifo, struct sd_desc *desc, int mbox)
{
	struct sd_mbox_reasm *ra;
	struct sd_mesg       *ret;
	unsigned              ltr = desc->virt[1] & 3;
	unsigned              num = (desc->virt[2] >> 28) & 0x0F;
	unsigned              seg = (desc->virt[2] >> 24) & 0x0F;
	unsigned              len = (desc->virt[2] >>  4) & 0xFF;
	unsigned              bit = 1 << seg;
	void                 *dst;

	ra = &fifo->sd->mbox_reasm[mbox][ltr];
	pr_trace("%s: mbox %d, letter %u, seg %u / num %u\n", fifo->name, mbox, ltr, seg, num);
	num++;
	len++;

	// init reassembly
	if ( ! ra->mesg )
	{
		if ( !(ra->mesg = sd_mesg_alloc(11, num * len, GFP_ATOMIC)) )
		{
			pr_err("%s: failed to alloc %u for message\n", fifo->name, num * len);
			return NULL;
		}
		ra->bits = (1 << num) - 1;
		ra->num  = num;
		ra->len  = len;
		setup_timer(&ra->timer, sd_mbox_reasm_timeout, (unsigned long)ra);

		ra->mesg->dst_addr =  desc->virt[0] & 0xFFFF;
		ra->mesg->src_addr = (desc->virt[0] >> 16) & 0xFFFF;
		ra->mesg->hello[0] =  desc->virt[1];
		ra->mesg->hello[1] =  desc->virt[2];
		ra->mesg->mesg.mbox.mbox   = mbox;
		ra->mesg->mesg.mbox.letter = ltr;
	}

	// sanity checks
	if ( !(ra->bits & bit) )
		pr_debug("%s: seg %u duplicate?\n", fifo->name, seg);
	if ( ra->num != num )
		pr_debug("%s: num %u when expecting %u?\n", fifo->name, num, ra->num);
	if ( (seg + 1) < num && ra->len != len )
		pr_debug("%s: len %u when expecting %u?\n", fifo->name, len, ra->len);

	// payload fraction
	dst  = ra->mesg->mesg.mbox.data;
	dst += ra->len * seg;
	pr_trace("%s: buff %p, offs %03x, dst %p\n", fifo->name,
	         ra->mesg->mesg.mbox.data, ra->len * seg, dst);
	memcpy(dst, &desc->virt[SD_HEAD_SIZE], len);
	sd_desc_free(fifo->sd, desc);

	// clear fragment bit and check for done
	ra->bits &= ~bit;
	if ( ra->bits )
	{
		pr_trace("%s: seg %u -> bits %04x, wait\n", fifo->name, seg, ra->bits);
		mod_timer(&ra->timer, jiffies + 10); // 100ms for now
		return NULL;
	}

	// done: figure final size (last frag may be partial size), zero state, return
	pr_trace("%s: seg %u -> bits %04x, done (last seg %u)\n", fifo->name,
	         seg, ra->bits, len);
	ret = ra->mesg;
	ret->size = offsetof(struct sd_mesg,      mesg) + 
	            offsetof(struct sd_mesg_mbox, data) + 
	            (num - 1) * ra->len + len;
	pr_trace("Final size %u:\n", ret->size);
//	hexdump_buff(ret, ret->size);
	del_timer(&ra->timer);
	memset(ra, 0, sizeof(*ra));

	return ret;
}


#define DATA_SIZE (sizeof(uint32_t) * SD_DATA_SIZE)
#define HEAD_SIZE (sizeof(uint32_t) * SD_HEAD_SIZE)

int sd_mbox_frag (struct srio_dev *sd, struct sd_desc **desc, struct sd_mesg *mesg,
                  gfp_t flags)
{
	unsigned  left;
	unsigned  len;
	unsigned  num;
	unsigned  idx;
	unsigned  ltr;
	void     *ptr;

	left = mesg->size -
	       offsetof(struct sd_mesg, mesg) -
	       offsetof(struct sd_mesg_mbox, data);
	if ( left > RIO_MAX_MSG_SIZE )
	{
		pr_err("%s: Bad message size %d (max %u)\n", __func__, left, RIO_MAX_MSG_SIZE);
		return -EINVAL;
	}

	if ( (num = (left + DATA_SIZE - 1) / DATA_SIZE) >= 16 )
	{
		pr_err("%s: Message too big (%u frags, max 16)\n", __func__, num);
		return -EFBIG;
	}
	pr_trace("%s: mesg size %d -> %d frags\n", __func__, left, num);

	// Letter number 0..3 is normally a rolling counter per mbox, but allow the user the
	// flexibility of trading off smaller messages and more mailboxes
	if ( mesg->mesg.mbox.mbox > 3 )
		ltr = mesg->mesg.mbox.letter;
	else
		ltr = sd->mbox_letter[mesg->mesg.mbox.mbox]++;

	// setup HELLO header on first fragment.  if the message needs fragmentation, then the
	// size on all descriptors except the last is the chunk size, 256 in this driver, -1.
	// In the last/only descriptor a smaller chunk size is used
	desc[0]->virt[1]  = ltr & 3;
	desc[0]->virt[1] |= (mesg->mesg.mbox.mbox & 0x3F) << 4;
	desc[0]->virt[2]  = 0x00B00000;
	desc[0]->virt[2] |= (num - 1) << 28;
	desc[0]->virt[2] |= (min(left, DATA_SIZE) - 1) << 4;

	// Payload of first fragment
	ptr = mesg->mesg.mbox.data;
	memcpy(&desc[0]->virt[SD_HEAD_SIZE], ptr, min(left, DATA_SIZE));
	desc[0]->used = min(left, DATA_SIZE) + HEAD_SIZE;

	// alloc first: caller is responsible for allocating and setting up first desc, and
	// freeing any descs allocated in here
	for ( idx = 1; idx < num; idx++ )
	{
		pr_debug("%s: frag %u: %08x:%08x.%08x used %04x\n", __func__, idx - 1,
		         desc[idx - 1]->virt[0], desc[idx - 1]->virt[2],
		         desc[idx - 1]->virt[1], desc[idx - 1]->used);

		ptr  += DATA_SIZE;
		left -= DATA_SIZE;

		if ( !(desc[idx] = sd_desc_alloc(sd, flags)) )
		{
			pr_err("%s: Failed to alloc fragment %d\n", __func__, idx);
			return -ENOMEM;
		}

		// copy addresses and header from frag 0 and modify
		memcpy(desc[idx]->virt, desc[0]->virt, HEAD_SIZE);
		desc[idx]->virt[2] |= idx << 24;

		// Payload of fragment
		memcpy(&desc[idx]->virt[SD_HEAD_SIZE], ptr, min(left, DATA_SIZE));
		desc[idx]->used = min(left, DATA_SIZE) + HEAD_SIZE;

	}

	// padding: type 11 messages are limited to 8/16/32/64/128/256 byte payload lengths,
	// which applies to last/only fragment since we fragment at 256 bytes.  find the
	// smallest valid length, zero-pad, and adjust the HELLO word in the descriptor
	if ( left < 256 )
		for ( len = 8; len < 256; len <<= 1 )
			if ( left < len )
			{
				ptr = &desc[num - 1]->virt[SD_HEAD_SIZE];
				memset(ptr + left, 0, (len - left));

				desc[num - 1]->virt[2] &= ~0x00000FF0;
				desc[num - 1]->virt[2] |= (len - 1) << 4;
				desc[num - 1]->used = len + HEAD_SIZE;

				pr_debug("%s: Pad last frag %u -> %u\n", __func__, left, len);
				break;
			}

	pr_debug("%s: frag %u: %08x:%08x.%08x used %04x (last)\n", __func__, num - 1,
	         desc[num - 1]->virt[0], desc[num - 1]->virt[2],
	         desc[num - 1]->virt[1], desc[num - 1]->used);

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
