/** \file      loop/test.h
 *  \brief     SRIO Loopback test
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
#ifndef _INCLUDE_LOOP_TEST_H_
#define _INCLUDE_LOOP_TEST_H_
#include <linux/kernel.h>

#include "srio_dev.h"

extern int sd_loop_tx_dest;
extern struct srio_dev *sd_dev;

struct device *sd_loop_init (struct srio_dev *sd);
void           sd_loop_exit (void);


void sd_loop_set_tuser (u32 tuser);
u32  sd_loop_get_tuser (void);


#endif // _INCLUDE_LOOP_TEST_H_

