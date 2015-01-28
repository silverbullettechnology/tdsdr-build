/** \file      dsxx/test.h
 *  \brief     Definitions for data source/sink modules
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
#ifndef _INCLUDE_DSXX_TEST_H_
#define _INCLUDE_DSXX_TEST_H_
#include <linux/kernel.h>




extern struct sd_fifo          sd_dsxx_fifo;
extern struct sd_fifo_config   sd_dsxx_fifo_cfg;

extern uint32_t  sd_dsxx_rx_length;
extern uint32_t  sd_dsxx_rx_pause_on;
extern uint32_t  sd_dsxx_rx_pause_off;
extern uint32_t  sd_dsxx_rx_reps;

extern uint32_t  sd_dsxx_tx_length;
extern uint32_t  sd_dsxx_tx_pause_on;
extern uint32_t  sd_dsxx_tx_pause_off;
extern uint32_t  sd_dsxx_tx_burst;

struct sd_dsxx_stats
{
	unsigned long  starts;
	unsigned long  timeouts;
	unsigned long  valids;
	unsigned long  errors;
};
extern struct sd_dsxx_stats sd_dsxx_rx_stats;
extern struct sd_dsxx_stats sd_dsxx_tx_stats;

struct device *sd_dsxx_init (void);
void sd_dsxx_exit (void);

void sd_dsxx_rx_trigger (void);




#endif // _INCLUDE_DSXX_TEST_H_

