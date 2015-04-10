/** \file      sd_mbox.h
 *  \brief     Stateless Rapidio stack operations
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
