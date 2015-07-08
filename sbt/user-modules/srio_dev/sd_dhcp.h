/** \file      sd_dhcp.h
 *  \brief     Dynamic device-ID probe using loopback
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
#ifndef _INCLUDE_SD_DHCP_H_
#define _INCLUDE_SD_DHCP_H_
#include <linux/kernel.h>


struct srio_dev;

struct sd_dhcp
{
	uint16_t           min;
	uint16_t           max;
	uint16_t           cur;
	uint16_t           rep;
	struct timer_list  timer;
	uint64_t           rand[2];
};


void sd_dhcp_start  (struct srio_dev *sd, uint16_t min, uint16_t max, uint16_t rep);
void sd_dhcp_pause  (struct srio_dev *sd);
void sd_dhcp_resume (struct srio_dev *sd);


#endif // _INCLUDE_SD_DHCP_H_
