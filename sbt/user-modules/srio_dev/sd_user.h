/** \file      sd_user.h
 *  \brief     /proc/ entries for Loopback test
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
#ifndef _INCLUDE_SD_USER_H_
#define _INCLUDE_SD_USER_H_
#include <linux/kernel.h>


// TODO: IOCTLs for config and userspace tools


int  sd_user_init (struct srio_dev *sd);
void sd_user_exit (void);


#endif /* _INCLUDE_SD_USER_H_ */
