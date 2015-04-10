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


struct sd_desc
{
	void            *virt;  /* Virtual address for data buffer */
	dma_addr_t       phys;  /* Physical address for data buffer */
	size_t           size;  /* Size of data buffer for range checks */
	size_t           used;  /* Size of data in buffer */
	size_t           offs;  /* Cursor for chunked transfer */
	unsigned long    info;  /* Identifier for upper layer, not used by the FIFO code */
	struct list_head list;  /* List pointers for queuing */
#ifdef CONFIG_USER_MODULES_SRIO_DEV_FIFO_DEST
	uint32_t         dest;  /* TDR/RDR register value */
#endif
};


int sd_desc_alloc (struct sd_desc *desc, size_t size, gfp_t flags);

void sd_desc_free (struct sd_desc *desc);

int sd_desc_map (struct sd_desc *desc, struct device *dev,
                 enum dma_transfer_direction dir);

void sd_desc_unmap (struct sd_desc *desc, struct device *dev,
                    enum dma_transfer_direction dir);

int sd_desc_setup_ring (struct sd_desc *ring, int len, size_t size, gfp_t flags,
                        struct device *dev, int rx_dma);

void sd_desc_clean_ring (struct sd_desc *ring, int len, struct device *dev);

#endif 
