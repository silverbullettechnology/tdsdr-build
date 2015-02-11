/** \file      sd_desc.c
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
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/dmaengine.h>
#include <linux/dma-mapping.h>

#include "sd_desc.h"


int sd_desc_alloc (struct sd_desc *desc, size_t size)
{
	if ( !(desc->virt = kzalloc(size, GFP_KERNEL)) )
	{
		pr_err("%s: kzalloc %zu bytes failed\n", __func__, size);
		return -1;
	}

	if ( !desc->virt )
	{
		pr_err("%s: kzalloc %zu bytes failed\n", __func__, size);
		return -1;
	}

	desc->phys = 0;
	desc->size = size;
	desc->used = 0;
	desc->offs = 0;

	INIT_LIST_HEAD(&desc->list);
	return 0;
}

int sd_desc_map (struct sd_desc *desc, struct device *dev, enum dma_transfer_direction dir)
{
	if ( !(desc->phys = dma_map_single(dev, desc->virt, desc->size, dir)) )
	{
		pr_err("%s: dma_map_single failed\n", __func__);
		return -1;
	}
	else
		pr_debug("%s: DMA mapped %p -> %08x\n", __func__, desc->virt, desc->phys);

	return 0;
}

void sd_desc_unmap (struct sd_desc *desc, struct device *dev,
                    enum dma_transfer_direction dir)
{
	dma_unmap_single(dev, desc->phys, desc->size, dir);
}

void sd_desc_free (struct sd_desc *desc)
{
	kfree(desc->virt);
	
	desc->virt = NULL;
	desc->phys = 0;
	desc->size = 0;
	desc->used = 0;
	desc->offs = 0;

	if ( !list_empty(&desc->list) )
		list_del_init(&desc->list);
}


void sd_desc_clean_ring (struct sd_desc *ring, int len, struct device *dev, int rx_dma)
{
	int idx;

	for ( idx = 0; idx < len; idx++ )
	{
		if ( rx_dma )
			sd_desc_unmap(&ring[idx], dev, DMA_DEV_TO_MEM);
		sd_desc_free(&ring[idx]);
	}
}

int sd_desc_setup_ring (struct sd_desc *ring, int len, size_t size, struct device *dev, int rx_dma)
{
	int  idx, ret;

	for ( idx = 0; idx < len; idx++ )
		if ( (ret = sd_desc_alloc(&ring[idx], size)) )
		{
			pr_err("RX slot %d sd_desc_alloc() failed, stop\n", idx);
			sd_desc_clean_ring(ring, idx - 1, dev, rx_dma);
			return ret;
		}
		else if ( rx_dma && (ret = sd_desc_map(&ring[idx], dev, DMA_DEV_TO_MEM)) )
		{
			pr_err("RX slot %d sd_desc_map() failed, stop\n", idx);
			sd_desc_clean_ring(ring, idx - 1, dev, rx_dma);
			return ret;
		}

	return 0;
}



