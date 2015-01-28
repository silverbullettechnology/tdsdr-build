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


extern struct sd_fifo          sd_loop_init_fifo;
extern struct sd_fifo          sd_loop_targ_fifo;
extern struct sd_fifo          sd_loop_user_fifo;
extern struct sd_fifo_config   sd_loop_init_fifo_cfg;
extern struct sd_fifo_config   sd_loop_targ_fifo_cfg;
extern struct sd_fifo_config   sd_loop_user_fifo_cfg;

extern struct sd_srio_config   sd_srio_cfg;

struct device *sd_loop_init (void);
void           sd_loop_exit (void);


#endif // _INCLUDE_LOOP_TEST_H_

