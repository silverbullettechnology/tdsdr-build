/** \file      sd_recv.h
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
#ifndef _INCLUDE_RECV_H
#define _INCLUDE_RECV_H
#include <linux/kernel.h>

#include "srio_dev.h"
#include "sd_fifo.h"
#include "sd_desc.h"
#include "sd_recv.h"


void sd_recv_init (struct sd_fifo *fifo, struct sd_desc *desc);
void sd_recv_targ (struct sd_fifo *fifo, struct sd_desc *desc);


#endif /* _INCLUDE_RECV_H */
