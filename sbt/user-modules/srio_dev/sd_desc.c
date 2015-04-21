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

#include "srio_dev.h"
#include "sd_desc.h"


struct sd_desc *sd_desc_alloc (struct srio_dev *sd, gfp_t flags)
{
	struct sd_desc *desc = kmem_cache_alloc(sd->desc_pool, GFP_KERNEL);
	if ( !desc )
		return NULL;

	desc->phys = 0;
	desc->used = 0;
	desc->offs = 0;
	INIT_LIST_HEAD(&desc->list);

	memset(&desc->virt[SD_FIFO_SIZE], 0xFF, SD_PAINT_SIZE * sizeof(uint32_t));

	return desc;
}


void sd_desc_free (struct srio_dev *sd, struct sd_desc *desc)
{
	int i;

	for ( i = SD_FIFO_SIZE; i < SD_DESC_SIZE; i++ )
		if ( desc->virt[i] != 0xFFFFFFFF )
		{
			pr_err("%p: paint corruption at free, desc:\n", desc);
			hexdump_buff(desc, sizeof(*desc));
			break;
		}

	desc->phys = 0;
	desc->used = 0;
	desc->offs = 0;
	if ( !list_empty(&desc->list) )
		list_del_init(&desc->list);

	kmem_cache_free(sd->desc_pool, desc);
}


int sd_desc_init (struct srio_dev *sd)
{
	sd->desc_pool = kmem_cache_create("sd_desc_pool", sizeof(struct sd_desc),
	                                  sizeof(uint32_t), SLAB_CACHE_DMA, NULL);
	if ( !sd->desc_pool )
	{
		pr_err("kmem_cache_create failed\n");
		return -ENOMEM;
	}

	return 0;
}


void sd_desc_exit (struct srio_dev *sd)
{
	kmem_cache_destroy(sd->desc_pool);
}


