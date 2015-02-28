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

#define SD_LOOP_GT_TXDIFFCTRL   8
#define SD_LOOP_GT_TXPRECURSOR  0
#define SD_LOOP_GT_TXPOSTCURSOR 0


extern struct sd_fifo          sd_loop_init_fifo;
extern struct sd_fifo          sd_loop_targ_fifo;
extern struct sd_fifo_config   sd_loop_init_fifo_cfg;
extern struct sd_fifo_config   sd_loop_targ_fifo_cfg;

extern struct sd_srio_config   sd_srio_cfg;

extern int sd_loop_tx_dest;

struct device *sd_loop_init (void);
void           sd_loop_exit (void);


void sd_loop_set_tuser (u32 tuser);
u32  sd_loop_get_tuser (void);


#endif // _INCLUDE_LOOP_TEST_H_

