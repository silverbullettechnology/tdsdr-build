/** \file      dsm_xfer.c
 *  \brief     Transfer setup / cleanup for DSM
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
#include <linux/version.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/mm.h>
#include <linux/ioport.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/sched.h>
#include <linux/uaccess.h>
#include <linux/pagemap.h>
#include <linux/kthread.h>
#include <linux/scatterlist.h>
#include <linux/dmaengine.h>
#include <linux/amba/xilinx_dma.h>

#include "dma_streamer_mod.h"
#include "dsm_private.h"


/** Clean up transfer - currently assumes zero-copy userspace page
 *
 *  \param  chan  DSM channel state structure pointer
 *  \param  xfer  DSM transfer state structure pointer
 */
void dsm_xfer_cleanup (struct dsm_chan *chan, struct dsm_xfer *xfer)
{
	struct scatterlist *sg;
	struct page        *pg;
	int                 sc;
	int                 pc;

	dma_unmap_sg(dsm_dev, xfer->chain[0], xfer->pages, chan->dma_dir);

	pc = xfer->pages;
	sg = xfer->chain[0];
	sc = pc / (SG_MAX_SINGLE_ALLOC - 1);
	if ( pc % (SG_MAX_SINGLE_ALLOC - 1) )
		sc++;

pr_debug("%s(): pc %d, sg %p, sc %d\n", __func__, pc, sg, sc);

	pr_trace("put_page() on %d user pages:\n", pc);
	while ( pc > 0 )
	{
		pg = sg_page(sg);
		pr_trace("  sg %p -> pg %p\n", sg, pg);
		if ( chan->dma_dir == DMA_DEV_TO_MEM && !PageReserved(pg) )
			set_page_dirty(pg);

		// equivalent to page_cache_release()
		put_page(pg);

		sg = sg_next(sg);
		pc--;
	}

	pr_trace("free_page() on %d sg pages:\n", sc);
	for ( pc = 0; pc < sc; pc++ )
		if ( xfer->chain[pc] )
		{
			pr_trace("  sg %p\n", xfer->chain[pc]);
			free_page((unsigned long)xfer->chain[pc]);
		}
}


/** Setup transfer - currently assumes zero-copy userspace page
 *
 *  \param  chan  DSM channel state structure pointer
 *  \param  desc  DMA channel description pointer
 *  \param  buff  Userspace buffer specfication
 *
 *  \return  DSM transfer state structure
 */
struct dsm_xfer *dsm_xfer_map_user (struct dsm_chan            *chan,
                                    struct dsm_chan_desc       *desc,
                                    const struct dsm_user_xfer *buff)
{
	struct dsm_xfer     *xfer;

	unsigned long        us_bytes;
	unsigned long        us_words;
	unsigned long        us_addr;
	struct page        **us_pages;
	int                  us_count;

	struct scatterlist  *sg_walk;
	int                  sg_count;
	int                  sg_index;

	int                  idx;
	int                  ret;

	pr_debug("%s(chan %p, desc %s, buff.addr %08lx, .size %08lx )\n", __func__,
	         chan, desc->device, buff->addr, buff->size);

	// address alignment check
	if ( buff->addr & ~PAGE_MASK )
	{
		pr_err("bad page alignment: addr %08lx\n", buff->addr);
		return NULL;
	}
	us_addr = buff->addr;

	// size / granularity check unnecessary now that size is specified in words
	us_bytes = buff->size << desc->width;
	us_words = buff->size;
	pr_debug("%lu bytes / %lu words per DMA\n", us_bytes, us_words);

	// pagelist for looped get_user_pages() to fill
	if ( !(us_pages = (struct page **)__get_free_page(GFP_KERNEL)) )
	{
		pr_err("out of memory getting userspace page list\n");
		return NULL;
	}

	// count userspace pages, the last may be partial
	us_count = us_bytes >> PAGE_SHIFT;
	if ( us_bytes & ~PAGE_MASK )
		us_count++;
	pr_debug("%d user pages\n", us_count);

	// count of scatterlist pages, allowing an extra entry per page for chaining
	sg_count = us_count / (SG_MAX_SINGLE_ALLOC - 1);
	if ( us_count % (SG_MAX_SINGLE_ALLOC - 1) )
		sg_count++;

	// single struct with variable-sized chain[] array following fixed members; each array
	// element is a single page-sized scatterlist
	xfer = kzalloc(offsetof(struct dsm_xfer, chain) +
	                sizeof(struct scatterlist *) * sg_count,
	                GFP_KERNEL);
	if ( !xfer )
	{
		pr_err("failed to kmalloc dsm_xfer\n");
		goto free;
	}
	xfer->pages = us_count;
	xfer->bytes = us_bytes;
	xfer->words = us_words;

	// setup for burst/repeat transfer
	xfer->chunk = buff->words;
	pr_debug("%lu words per DMA, %lu per rep = %lu + %lu reps\n",
	         xfer->words, xfer->chunk,
	         xfer->chunk / xfer->words,
	         xfer->chunk % xfer->words);

	// allocate scatterlists and init
	pr_debug("%d sg pages\n", sg_count);
	for ( idx = 0; idx < sg_count; idx++ )
		if ( !(xfer->chain[idx] = (struct scatterlist *)__get_free_page(GFP_KERNEL)) )
		{
			pr_err("failed to get page for chain[%d]\n", idx);
			goto free;
		}
		else
			sg_init_table(xfer->chain[idx], SG_MAX_SINGLE_ALLOC);

	// chain together before adding pages
	for ( idx = 1; idx < sg_count; idx++ )
		sg_chain(xfer->chain[idx - 1], SG_MAX_SINGLE_ALLOC, xfer->chain[idx]);

	// get_user_pages in blocks and convert to chained scatterlists
	sg_index = 0;;
	sg_walk  = xfer->chain[0];
	while ( us_count )
	{
		int want = min_t(int, us_count, SG_MAX_SINGLE_ALLOC - 1);

		down_read(&current->mm->mmap_sem);
		ret = get_user_pages(current, current->mm, us_addr, want, 
		                     (chan->dma_dir == DMA_DEV_TO_MEM),
		                     0, us_pages, NULL);
		up_read(&current->mm->mmap_sem);
		pr_trace("get_user_pages(..., us_addr %08lx, want %04x, ...) returned %d\n",
		         us_addr, want, ret);

		if ( ret < want )
		{
			pr_err("get_user_pages(): want %d, got %d, stop\n", want, ret);
			goto free;
		}

		for ( idx = 0; idx < ret; idx++ )
		{
			unsigned int len = min_t(unsigned int, us_bytes, PAGE_SIZE);
			sg_set_page(sg_walk, us_pages[idx], len, 0);
			us_bytes -= len;
			if ( us_bytes )
				sg_walk = sg_next(sg_walk);
			else
				sg_mark_end(sg_walk);
		}

		us_addr  += ret << PAGE_SHIFT;
		us_count -= ret;
	}

	// assumes that map_sg uses chain-aware iteration (sg_next/for_each_sg)
	dma_map_sg(dsm_dev, xfer->chain[0], xfer->pages, chan->dma_dir);

	pr_trace("Mapped scatterlist chain:\n");
	for_each_sg(xfer->chain[0], sg_walk, xfer->pages, idx)
		pr_trace("  %d: sg %p -> phys %08lx\n", idx, sg_walk,
		         (unsigned long)sg_dma_address(sg_walk));

	pr_debug("done\n");
	free_page((unsigned long)us_pages);
	return xfer;

free:
	free_page((unsigned long)us_pages);
	for ( idx = 0; idx < sg_count; idx++ )
		if ( xfer->chain[idx] )
			free_page((unsigned long)xfer->chain[idx]);
	kfree(xfer);
	return NULL;
}


