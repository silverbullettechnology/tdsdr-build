/** \file      sd_desc.h
 *  \brief     Message descriptor / buffer
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
#ifndef _INCLUDE_SD_DESC_H_
#define _INCLUDE_SD_DESC_H_
#include <linux/kernel.h>
#include <linux/dmaengine.h>


/* Sizes in 32-bit words */
#define SD_HEAD_SIZE   3
#define SD_DATA_SIZE  64
#define SD_PAINT_SIZE  1
#define SD_FIFO_SIZE   (SD_HEAD_SIZE + SD_DATA_SIZE)
#define SD_DESC_SIZE   (SD_FIFO_SIZE + SD_PAINT_SIZE)

struct srio_dev;

struct sd_desc
{
	dma_addr_t       phys;  /* Physical address for data buffer */
	size_t           used;  /* Size of data in buffer */
	size_t           offs;  /* Cursor for chunked transfer */
	struct list_head list;  /* List pointers for queuing */
#ifdef CONFIG_USER_MODULES_SRIO_DEV_FIFO_DEST
	uint32_t         dest;  /* TDR/RDR register value */
#endif
	uint32_t         virt[SD_DESC_SIZE];  /* Actual data buffer */
};


struct sd_desc *sd_desc_alloc (struct srio_dev *sd, gfp_t flags);

void sd_desc_free (struct srio_dev *sd, struct sd_desc *desc);

int sd_desc_init (struct srio_dev *sd);

void sd_desc_exit (struct srio_dev *sd);


#endif 
