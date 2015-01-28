/** \file      sd_xparameters.c
 *  \brief     Definitions used in main module and sd_xparameters.c
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
#ifndef _INCLUDE_SD_XPARAMETERS_H_
#define _INCLUDE_SD_XPARAMETERS_H_
#include <linux/kernel.h>




void __iomem *sd_iomap (unsigned long base, size_t size);
void sd_unmap          (void __iomem *mapped, unsigned long base, size_t size);


int  sd_xparameters_init (void);
void sd_xparameters_exit (void);


#endif // _INCLUDE_SD_XPARAMETERS_H_
