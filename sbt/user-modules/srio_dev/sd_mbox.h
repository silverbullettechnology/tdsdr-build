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


#include "srio_dev.h"

#include "sd_mesg.h"
#include "sd_fifo.h"
#include "sd_desc.h"



struct sd_mesg *sd_mbox_reasm (struct sd_fifo *fifo, struct sd_desc *desc, int mbox);


int sd_mbox_frag (struct srio_dev *sd, struct sd_desc **desc, struct sd_mesg *mesg);


int sd_mbox_open_outb_mbox (struct rio_mport *mport, void *dev_id, int mbox,
                            int entries);

void sd_mbox_close_outb_mbox (struct rio_mport *mport, int mbox);

int sd_mbox_open_inb_mbox (struct rio_mport *mport, void *dev_id, int mbox, int entries);

void sd_mbox_close_inb_mbox (struct rio_mport *mport, int mbox);

int sd_mbox_add_outb_message (struct rio_mport *mport, struct rio_dev *rdev, int mbox,
                              void *buffer, size_t len);

int sd_mbox_add_inb_buffer (struct rio_mport *mport, int mbox, void *buf);

void *sd_mbox_get_inb_message (struct rio_mport *mport, int mbox);


#endif /* _INCLUDE_SD_MBOX_H */
