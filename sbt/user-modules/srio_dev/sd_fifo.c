/** \file      sd_fifo.c
 *  \brief     
 *             
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
#include <linux/device.h>
#include <linux/of.h>
#include <linux/io.h>
#include <linux/ioport.h>
#include <linux/skbuff.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>

#include "srio_dev.h"
#include "sd_fifo.h"
#include "sd_desc.h"


#ifdef DEBUG
#define sf_trace(sf, fmt, ...) do{ \
	if ( sf->flags & SD_FL_TRACE ) \
		pr_debug("%s: %s[%d]: " fmt, sf->name, __func__, __LINE__, ##__VA_ARGS__); \
	}while(0)
#define sf_debug(sf, fmt, ...) do{ \
	if ( sf->flags & SD_FL_DEBUG ) \
		pr_debug("%s: %s[%d]: " fmt, sf->name, __func__, __LINE__, ##__VA_ARGS__); \
	}while(0)
#else
#define sf_trace(...) do{ }while(0)
#define sf_debug(...) do{ }while(0)
#endif

#define sf_info(sf, fmt, ...) do{ \
	if ( sf->flags & SD_FL_INFO ) \
		pr_info("%s: %s[%d]: " fmt, sf->name, __func__, __LINE__, ##__VA_ARGS__); \
	}while(0)
#define sf_warn(sf, fmt, ...) do{ \
	if ( sf->flags & SD_FL_WARN ) \
		pr_warn("%s: %s[%d]: " fmt, sf->name, __func__, __LINE__, ##__VA_ARGS__); \
	}while(0)
#define sf_err(sf, fmt, ...) do{ \
	if ( sf->flags & SD_FL_ERROR ) \
		pr_err("%s: %s[%d]: " fmt, sf->name, __func__, __LINE__, ##__VA_ARGS__); \
	}while(0)



#define RT_TIMER_DELAY 10 // 100ms


/* Bits for IER / ISR */
#define RFPE    (1 << 19)
#define RFPF    (1 << 20)
#define TFPE    (1 << 21)
#define TFPF    (1 << 22)
#define RRC     (1 << 23)
#define TRC     (1 << 24)
#define TSE     (1 << 25)
#define RC      (1 << 26)
#define TC      (1 << 27)
#define TPOE    (1 << 28)
#define RPUE    (1 << 29)
#define RPORE   (1 << 30)
#define RPURE   (1 << 31)

#define RESET_VAL   0x000000A5


/** Print interrupt bits in IER/ISR using pr_debug */
#ifdef DEBUG
static void sd_dump_ixr (struct sd_fifo *sf, const char *name, uint32_t reg)
{
	if ( !(sf->flags & SD_FL_DEBUG) )
		return;

	pr_debug("%s: %s: %08x: { %s%s%s%s%s%s%s%s%s%s%s%s%s}\n",
	         sf->name, name, reg,
	         reg & RFPE  ? "RFPE "  : "",
	         reg & RFPF  ? "RFPF "  : "",
	         reg & TFPE  ? "TFPE "  : "",
	         reg & TFPF  ? "TFPF "  : "",
	         reg & RRC   ? "RRC "   : "",
	         reg & TRC   ? "TRC "   : "",
	         reg & TSE   ? "TSE "   : "",
	         reg & RC    ? "RC "    : "",
	         reg & TC    ? "TC "    : "",
	         reg & TPOE  ? "TPOE "  : "",
	         reg & RPUE  ? "RPUE "  : "",
	         reg & RPORE ? "RPORE " : "",
	         reg & RPURE ? "RPURE " : "");
}
#else
#define sd_dump_ixr(...) do{ }while(0)
#endif


/** Print meaningful registers using pr_debug */
#ifdef DEBUG
void sd_dump_regs (struct sd_fifo *sf)
{
	if ( !(sf->flags & SD_FL_DEBUG) )
		return;

	sd_dump_ixr(sf, "ISR", REG_READ(&sf->regs->isr));
	sd_dump_ixr(sf, "IER", REG_READ(&sf->regs->ier));
	pr_debug("%s: tdfr   : %08x\n", sf->name, REG_READ(&sf->regs->tdfr   ));
	pr_debug("%s: tdfd   : %08x\n", sf->name, REG_READ(&sf->regs->tdfd   ));
	pr_debug("%s: tlr    : %08x\n", sf->name, REG_READ(&sf->regs->tlr    ));
	pr_debug("%s: rdfo   : %08x\n", sf->name, REG_READ(&sf->regs->rdfo   ));
	pr_debug("%s: rdfd   : %08x\n", sf->name, REG_READ(&sf->regs->rdfd   ));
	pr_debug("%s: rlr    : %08x\n", sf->name, REG_READ(&sf->regs->rlr    ));
	pr_debug("%s: srr    : %08x\n", sf->name, REG_READ(&sf->regs->srr    ));
	pr_debug("%s: tdr    : %08x\n", sf->name, REG_READ(&sf->regs->tdr    ));
	pr_debug("%s: rdr    : %08x\n", sf->name, REG_READ(&sf->regs->rdr    ));
#if 0
	pr_debug("%s: tx_id  : %08x\n", sf->name, REG_READ(&sf->regs->tx_id  ));
	pr_debug("%s: tx_user: %08x\n", sf->name, REG_READ(&sf->regs->tx_user));
	pr_debug("%s: rx_id  : %08x\n", sf->name, REG_READ(&sf->regs->rx_id  ));
	pr_debug("%s: rx_user: %08x\n", sf->name, REG_READ(&sf->regs->rx_user));
#endif
}
#else
#define sd_dump_regs(...) do{ }while(0)
#endif


static void sd_fifo_unmap (struct sd_fifo *sf, struct sd_desc *desc, 
                           enum dma_transfer_direction dir)
{
	if ( desc->phys )
		dma_unmap_single(sf->dev, desc->phys, desc->used, dir);
	desc->phys = 0;
}


#define SD_MESG_TTL 3

static uint32_t sd_fifo_response (const uint32_t *hello)
{
	uint32_t ret = 0x00D00000;

	// priority is 0..3, 0..2 for request packets, request + 1 for response
	BUG_ON( (hello[1] & 0x00006000) == 0x00006000 );
	ret += 0x00002000;

	switch ( hello[1] & 0x00F00000 )
	{
		case 0x00A00000: // DBELL - need TID but no respoinse
			ret |= SD_MESG_TTL; // TTL stored in bottom 8 bits
			ret |= hello[1] & 0xFF000000; // target TID
			break;

		case 0x00B00000: // MESSAGE - need mbox, letter, & seg
			ret |= 0x00010000 | SD_MESG_TTL; // transaction type +  TTL 
			ret |= hello[1] & 0x0F000000; // segment number
			ret |= (hello[0] & 0x00000030) << 24; // mbox
			ret |= (hello[0] & 0x00000003) << 30; // letter
			break;

		default:
			return 0;
	}

	return ret;
}


/** Reset FIFO's TX, RX, and/or AXI-Stream 
 *
 *  \note  Caller should hold &sf->rx.lock if SD_FR_RX is set in mask
 *  \note  SD_FR_TX and SD_FR_RX will cause TRC and RRC IRQs respectively when reset is complete
 *
 *  \param  sf    Pointer to sd_fifo struct
 *  \param  mask  Bitmask of SD_FR_* bits
 */
void sd_fifo_reset (struct sd_fifo *sf, int mask)
{
	if ( mask & SD_FR_TX )
	{
		pr_info("%s: %s: start TX reset\n", sf->name, __func__);
		REG_RMW(&sf->regs->ier, TFPE|TFPF|TSE|TC|TPOE, TRC);
		del_timer(&sf->rt_timer);
		sd_fifo_tx_discard(sf);
		sf->tx.stats.resets++;
	}
	if ( mask & SD_FR_RX )
	{
		pr_info("%s: %s: start RX reset\n", sf->name, __func__);
		REG_RMW(&sf->regs->ier, RFPE|RFPF|RRC|RC|TC|RPUE|RPORE|RPURE, RRC);
		sf->rx.stats.resets++;

		if ( sf->rx_current )
		{
			sd_fifo_unmap(sf, sf->rx_current, DMA_DEV_TO_MEM);
			sd_desc_free(sf->sd, sf->rx_current);
			sf->rx_current = NULL;
		}
	}

	if ( mask & SD_FR_TX  ) REG_WRITE(&sf->regs->tdfr, RESET_VAL);
	if ( mask & SD_FR_RX  ) REG_WRITE(&sf->regs->rdfr, RESET_VAL);
	if ( mask & SD_FR_SRR ) REG_WRITE(&sf->regs->srr,  RESET_VAL);
}




/******** TX handling *******/


/** Flush all TX descriptors
 *
 *  \param  sf  Pointer to sd_fifo struct
 */
void sd_fifo_tx_discard (struct sd_fifo *sf)
{
	struct list_head *temp, *walk;
	struct sd_desc   *desc;

	list_for_each_safe(walk, temp, &sf->tx_queue)
    {
		desc = container_of(walk, struct sd_desc, list);
		list_del_init(&desc->list);
		sd_fifo_unmap(sf, desc, DMA_MEM_TO_DEV);
		sd_desc_free(sf->sd, desc);
		pr_info("%s: %s: TX queue: desc %p freed\n", sf->name, __func__, desc);
	}
	list_for_each_safe(walk, temp, &sf->tx_retry)
    {
		desc = container_of(walk, struct sd_desc, list);
		list_del_init(&desc->list);
		sd_fifo_unmap(sf, desc, DMA_MEM_TO_DEV);
		sd_desc_free(sf->sd, desc);
		pr_info("%s: %s: TX retry: desc %p freed\n", sf->name, __func__, desc);
	}
}


/** Finish a TX, writing the TLR register with the size in bytes
 *
 *  \note  Caller should hold &sf->tx.lock
 *
 *  \param  sf  Pointer to sd_fifo struct
 */
static void sd_fifo_tx_finish (struct sd_fifo *sf)
{
	struct sd_desc *desc;

	if ( list_empty(&sf->tx_queue) )
	{
		pr_err("%s: %s: TX queue empty?  Reset... TX\n", sf->name, __func__);
		sd_fifo_reset(sf, SD_FR_TX);
		return;
	}
	desc = container_of(sf->tx_queue.next, struct sd_desc, list);

	sf->tx.stats.completes++;
	REG_WRITE(&sf->regs->tlr, desc->used);
	REG_RMW(&sf->regs->ier, TFPE, TC|TSE|TPOE);

	sf_debug(sf, "desc %p: wrote TLR %08x\n", desc, desc->used);
}


/** TX DMA completion callback */
static void sd_fifo_tx_dma_complete (void *param)
{
	struct sd_fifo *sf = param;
	struct sd_desc *desc;
	unsigned long   flags;

	if ( list_empty(&sf->tx_queue) )
	{
		pr_err("%s: %s: TX queue empty?  Reset... TX\n", sf->name, __func__);
		sd_fifo_reset(sf, SD_FR_TX);
		return;
	}
	desc = container_of(sf->tx_queue.next, struct sd_desc, list);
	sd_fifo_unmap(sf, desc, DMA_MEM_TO_DEV);

	spin_lock_irqsave(&sf->tx.lock, flags);
	sd_fifo_tx_finish(sf);
	del_timer(&sf->tx.timer);
	sf_debug(sf, "status %d\n",
	         dma_async_is_tx_complete(sf->tx.chan, sf->tx.cookie, NULL, NULL));
	spin_unlock_irqrestore(&sf->tx.lock, flags);
}


/** TX DMA timeout callback */
static void sd_fifo_tx_dma_timeout (unsigned long param)
{
	struct sd_fifo *sf = (struct sd_fifo *)param;

	pr_err("%s: %s: reset TX path\n", sf->name, __func__);
	sf->tx.stats.timeouts++;
	sd_fifo_reset(sf, SD_FR_TX);
}


/** Start TX DMA for all of the current TX descriptor's bulk data
 *
 *  \note  Caller should hold &sf->tx.lock
 *
 *  \param  sf  Pointer to sd_fifo struct
 */
static void sd_fifo_tx_dma (struct sd_fifo *sf)
{
	struct dma_async_tx_descriptor *xfer;
	struct sd_desc                 *desc;

	if ( list_empty(&sf->tx_queue) )
	{
		pr_err("%s: %s: TX queue empty?  Reset... TX\n", sf->name, __func__);
		sd_fifo_reset(sf, SD_FR_TX);
		return;
	}
	desc = container_of(sf->tx_queue.next, struct sd_desc, list);

	sf_debug(sf, "set up DMA, %08x bytes from %08x to %08x...\n",
	         desc->used, desc->phys, sf->tx.phys);
	xfer = sf->tx.chan->device->device_prep_dma_memcpy(sf->tx.chan, sf->tx.phys, 
	                                                   desc->phys, desc->used,
	                                                   DMA_CTRL_ACK);
	if ( !xfer )
	{
		pr_err("device_prep_dma_memcpy() failed\n");
		sf->tx.stats.errors++;
		sd_fifo_reset(sf, SD_FR_TX);
		return;
	}
	sf_debug(sf, "device_prep_dma_memcpy(): %p\n", xfer);

	/* setup completion callback */
	xfer->callback       = sd_fifo_tx_dma_complete;
	xfer->callback_param = sf;

	/* submit transaction */
	sf->tx.cookie = xfer->tx_submit(xfer);
	if ( dma_submit_error(sf->tx.cookie) )
	{
		pr_err("tx_submit() failed, stop\n");
		sf->tx.stats.errors++;
		return;
	}
	sf_debug(sf, "tx_submit() ok, cookie %lx\n", (unsigned long)sf->tx.cookie);

	/* failure timer - 1 sec */
	mod_timer(&sf->tx.timer, jiffies + sf->tx.timeout);

	/* start transaction and return */
	sf_debug(sf, "start TX and DMA\n");
	dma_async_issue_pending(sf->tx.chan);
	sf->tx.stats.chunks++;
}


/** Run TX PIO for a chunk of the current TX descriptor's bulk data
 *
 *  \note  Caller should hold &sf->tx.lock
 *  \note  Number of bytes to send will be limited by FIFO vacancy (TDFV register)
 *
 *  \param  sf  Pointer to sd_fifo struct
 */
static void sd_fifo_tx_pio (struct sd_fifo *sf)
{
	struct sd_desc *desc;
	const uint32_t *walk;
	size_t          left;
	uint32_t        tdfv;

	/* no reason to ever be here with an empty TX queue */
	if ( list_empty(&sf->tx_queue) )
	{
		pr_err("%s: %s: TX queue empty?  Reset... TX\n", sf->name, __func__);
		sd_fifo_reset(sf, SD_FR_TX);
		return;
	}
	desc = container_of(sf->tx_queue.next, struct sd_desc, list);

	tdfv = REG_READ(&sf->regs->tdfv) * sizeof(uint32_t);
	if ( !tdfv )
	{
		pr_warn("%s: %s: TDFV 0, arm TFPE and wait\n", sf->name, __func__);
		REG_RMW(&sf->regs->ier, 0, TFPE|TSE|TPOE);
		return;
	}

	walk = (desc->virt + desc->offs);
	left = (desc->used - desc->offs);
	sf_debug(sf, "used %zu - offs %zu = left %zu, cap TDFV %u\n",
	         desc->used, desc->offs, left, tdfv);

	if ( left > tdfv )
		left = tdfv;
	sf_debug(sf, "write %zu bytes:\n", left);

	REG_RMW(&sf->regs->ier, TFPE|TC|TSE|TPOE, 0);
	while ( left )
	{
		REG_WRITE(&sf->data->tdfd, *walk);
		walk++;
		left -= sizeof(uint32_t);
		desc->offs += sizeof(uint32_t);
	}
	sf->tx.stats.chunks++;
	sf_debug(sf, "TDFV now %08x\n", REG_READ(&sf->regs->tdfv));

	/* arm error IRQs plus TC or TFPE, depending whether this is the last chunk */
	if ( desc->used == desc->offs )
		sd_fifo_tx_finish(sf);
	else
		REG_RMW(&sf->regs->ier, 0, TFPE|TSE|TPOE);
}


/** Start TX on the descriptor at the head of the TX queue
 *
 *  \note  Caller should hold &sf->tx.lock
 *
 *  \param  sf  Pointer to sd_fifo struct
 */
static void sd_fifo_tx_start (struct sd_fifo *sf)
{
	struct sd_desc *desc;

	if ( list_empty(&sf->tx_queue) )
	{
		pr_err("%s: %s: TX queue empty?  Reset... TX\n", sf->name, __func__);
		sd_fifo_reset(sf, SD_FR_TX);
		return;
	}
	desc = container_of(sf->tx_queue.next, struct sd_desc, list);
	sf_debug(sf, "%08x:%08x.%08x, size %zu\n", 
	         desc->virt[0], desc->virt[2], desc->virt[1], desc->used);

#ifdef CONFIG_USER_MODULES_SRIO_DEV_FIFO_DEST
	REG_WRITE(&sf->regs->tdr, desc->dest);
	sf_debug(sf, "desc %p: wrote TDR %08x\n", desc, desc->dest);
#endif

	/* select DMA or PIO for the bulk transfer: need DMA channel and desc mapped */
	if ( sf->tx.chan && desc->phys )
	{
		sd_fifo_tx_dma(sf);
		REG_RMW(&sf->regs->ier, TFPE|TFPF, TC|TSE|TPOE);
	}
	else
		sd_fifo_tx_pio(sf);

	sf->tx.stats.starts++;
}


/** Enqueue a burst of TX descriptors to the tail of the TX queue, and start TX if the
 *  queue was empty
 *
 *  \note  Takes &sf->tx.lock, caller should not hold the lock
 *
 *  \note  For bursts with num > 1, the cookie value returned will be assigned to the
 *         first descriptor and passed to the tx_func if registered.  The cookie value
 *         will increment for each subsequent descriptor.  Thus a burst of 3 descriptors
 *         may return a cookie value of 17, and tx_func will be called three times with
 *         cookie values 17, 18, and 19 as the TX burst completes.
 *
 *  \param  sf    Pointer to sd_fifo struct
 *  \param  desc  Array of sd_desc struct pointers
 *  \param  num   Length of descriptor pointer array
 *
 *  \return  An opaque cookie for the TX transaction, which will be passed to tx_func
 */
unsigned sd_fifo_tx_burst (struct sd_fifo *sf, struct sd_desc **desc, int num)
{
	unsigned long  flags;
	unsigned       ret;
	uint32_t       devid;
	int            empty;
	int            idx;

	if ( sf->flags & SD_FL_NO_TX )
	{
		sf_debug(sf, "SD_FL_NO_TX: drop %d descs\n", num);
		for ( idx = 0; idx < num; idx++ )
		{
			sf_trace(sf, "SD_FL_NO_TX: drop desc %p\n", desc[idx]);
			sd_desc_free(sf->sd, desc[idx]);
		}
		ret = sf->tx_cookie;
		sf->tx_cookie += num;
		return ret;
	}

	devid = sf->sd->devid;
	devid <<= 16;

	/* alignment check - maybe pad later instead? */
	for ( idx = 0; idx < num; idx++ )
	{
		BUG_ON(desc[idx]->used & 0x03);
		BUG_ON(!desc[idx]->virt);
		BUG_ON(!desc[idx]->used);

		/* Set correct return address before DMA map, if devid is valid */
		if ( devid != 0xFFFF0000 )
		{
			desc[idx]->virt[0] &= 0x0000FFFF;
			desc[idx]->virt[0] |= devid;
		}

		if ( sf->tx.chan && !desc[idx]->phys )
		{
			desc[idx]->phys = dma_map_single(sf->dev, desc[idx]->virt, desc[idx]->used,
			                                 DMA_MEM_TO_DEV);
			BUG_ON(!desc[idx]->phys);
		}
		sf_trace(sf, "desc %p TX DMA mapped %p -> %08x\n",
		         desc[idx], desc[idx]->virt, desc[idx]->phys);

		desc[idx]->offs = 0;

		sf_debug(sf, "%08x:%08x.%08x, %zu bytes, TX queued:\n", 
		         desc[idx]->virt[0], desc[idx]->virt[2], desc[idx]->virt[1], desc[idx]->used - 12);
		if ( sf->flags & SD_FL_TRACE )
			hexdump_buff(&desc[idx]->virt[SD_HEAD_SIZE], desc[idx]->used - 12);

		// pre-calculate expected response packet descriptor based on this packet's HELLO
		// header, including TTL in bottom 8 bits.
		if ( devid != 0xFFFF0000 && !(sf->flags & SD_FL_NO_RSP) )
		{
			desc[idx]->resp = sd_fifo_response(desc[idx]->virt + 1);
			sf_debug(sf, "req %08x.%08x -> resp %08x\n",
			         desc[idx]->virt[2], desc[idx]->virt[1], desc[idx]->resp);
		}
		else
			desc[idx]->resp = 0;
	}

	sf_trace(sf, "lock...\n");
	spin_lock_irqsave(&sf->tx.lock, flags);
	empty = list_empty(&sf->tx_queue);
	sf_debug(sf, "append %d descs to %s list\n", num, empty ? "empty" : "non-empty");
	ret = sf->tx_cookie;
	for ( idx = 0; idx < num; idx++ )
	{
		sf_trace(sf, "desc %p...\n", desc[idx]);
		desc[idx]->info = sf->tx_cookie++;
		list_add_tail(&desc[idx]->list, &sf->tx_queue);
	}
	sf_trace(sf, "enqueued...\n");

	if ( empty )
	{
		sf_trace(sf, "start...\n");
		sd_fifo_tx_start(sf);
		sf_trace(sf, "started...\n");
	}

	spin_unlock_irqrestore(&sf->tx.lock, flags);
	sf_trace(sf, "unlock, return %u\n", ret);
	return ret;
}


static void sd_fifo_tx_retry (unsigned long param)
{
	struct list_head *temp, *walk;
	struct sd_fifo   *sf = (struct sd_fifo *)param;
	struct sd_desc   *desc;
	unsigned long     flags;
	int               empty;
	int               retry = 0;

	sf_debug(sf, "retry timer running\n");

	spin_lock_irqsave(&sf->tx.lock, flags);
	empty = list_empty(&sf->tx_queue);
	if ( !list_empty(&sf->tx_retry) )
	{
		list_for_each_safe(walk, temp, &sf->tx_retry)
		{
			desc = container_of(walk, struct sd_desc, list);
			list_del_init(&desc->list);
			if ( desc->resp-- & 0x000000FF )
			{
				sf_debug(sf, "desc %p retry, resp %08x\n", desc, desc->resp);
				if ( sf->tx.chan && !desc->phys )
				{
					desc->phys = dma_map_single(sf->dev, desc->virt, desc->used,
					                            DMA_MEM_TO_DEV);
					BUG_ON(!desc->phys);
				}
				desc->offs = 0;
				list_add_tail(&desc->list, &sf->tx_queue);
				retry++;
			}
			else
			{
				pr_info("%s: %s: desc %p dropped, resp %08x\n",
				        sf->name, __func__, desc, desc->resp);
				if ( sf->tx_func )
					sf->tx_func(sf, desc->info, 1);
				sd_desc_free(sf->sd, desc);
			}
		}
	}
	if ( empty && retry )
	{
		sf_debug(sf, "retry %d, empty %d, tx_start\n", retry, empty);
		sd_fifo_tx_start(sf);
	}

	spin_unlock_irqrestore(&sf->tx.lock, flags);
}


/******** RX handling *******/


/** Flush the RX FIFO, discarding the data there
 *
 *  \param  sf    Pointer to sd_fifo struct
 *  \param  size  Number of bytes to discard
 */
static void sd_fifo_rx_flush (struct sd_fifo *sf, uint32_t size)
{
	size += 3;
	size &= 3;

	while ( size )
	{
		(void)REG_READ(&sf->data->rdfd);
		size -= sizeof(uint32_t);
	}
}


/** Finish an RX
 *
 *  - 
 *
 *  \param  sf    Pointer to sd_fifo struct
 */
static void sd_fifo_rx_finish (struct sd_fifo *sf)
{
	struct list_head *temp, *walk;
	struct sd_desc   *desc = sf->rx_current;
	uint32_t          resp, tuser, type;

	sf->rx.stats.completes++;
	REG_RMW(&sf->regs->ier, 0, RFPF|RC);
	sf->rx_current = NULL;

	/* Unmap to flush cache */
	sd_fifo_unmap(sf, desc, DMA_DEV_TO_MEM);

	if ( sf->flags & SD_FL_NO_RX )
	{
		sf_debug(sf, "SD_FL_NO_RX: drop desc %p\n", desc);
		sd_desc_free(sf->sd, desc);
		return;
	}

	/* If devid is set, filter for it */
	if ( sf->sd->devid != 0xFFFF && (desc->virt[0] & 0xFFFF) != sf->sd->devid )
	{
		sf_debug(sf, "%08x:%08x.%08x, dropped\n",
		         desc->virt[0], desc->virt[2], desc->virt[1]);
		sd_desc_free(sf->sd, desc);
		//TODO: stats
		return;
	}

	/* handle response packets */
	type = (desc->virt[2] & 0x00F00000) >> 20;
	if ( type == 13 )
	{
		/* everything needed is in the MSW of the HELLO header */
		resp = desc->virt[2] & 0xFFF00000;
		sd_desc_free(sf->sd, desc);
		
		sf_debug(sf, "response packet %08x.%08x\n", desc->virt[2], desc->virt[1]);
		list_for_each_safe(walk, temp, &sf->tx_retry)
		{
			desc = container_of(walk, struct sd_desc, list);
			sf_debug(sf, "  desc %p resp %08x\n", desc, desc->resp);
			if ( (desc->resp & 0xFFF00000) == resp )
			{
				sf_debug(sf, "  match TTL %u, free\n\n", desc->resp & 0xFF);
				list_del_init(&desc->list);
				if ( sf->tx_func )
					sf->tx_func(sf, desc->info, 0);
				sd_desc_free(sf->sd, desc);
				break;
			}
		}
		if ( list_empty(&sf->tx_retry) )
		{
			sf_debug(sf, "retry list empty, stop timer\n\n");
			del_timer(&sf->rt_timer);
		}
		return;
	}

	/* Non-response packet: calculate response if necessary */
	tuser = desc->virt[0];
	if ( (resp = sd_fifo_response(desc->virt + 1)) )
		sf_debug(sf, "req %08x.%08x -> resp %08x\n",
		         desc->virt[2], desc->virt[1], resp);

	/* Dispatch to listener, or dump and free for debug */
	sf_debug(sf, "%08x:%08x.%08x, %zu bytes, RX complete:\n", 
	         desc->virt[0], desc->virt[2], desc->virt[1], desc->used - 12);
	if ( sf->flags & SD_FL_TRACE )
		hexdump_buff(&desc->virt[SD_HEAD_SIZE], desc->used - 12);

	/* ping response: type 10 (dbell) packet with info == ping[0], received on target
	 * FIFO, generate a ping response (type 10)  to the sender on the initiator FIFO
	 * reusing received desc.  no return here because we still have to send an SRIO
	 * response (type 13) for the request */
	if ( type == 10 && desc->virt[1] == sf->sd->ping[0] && sf == sf->sd->targ_fifo )
	{
		desc->virt[0] = (tuser >> 16) | (tuser << 16);
		desc->virt[1] = sf->sd->ping[1];
		desc->used = 12;
		desc->resp = sd_fifo_response(desc->virt + 1);
		sd_fifo_tx_burst(sf->sd->init_fifo, &desc, 1);
	}
	else if ( sf->rx_func )
		sf->rx_func(sf, desc);
	else
		sd_desc_free(sf->sd, desc);

	/* Build and send response packet if necessary */
	if ( resp && !(sf->flags & SD_FL_NO_RSP) )
	{
		desc = sd_desc_alloc(sf->sd, GFP_ATOMIC|GFP_DMA);
		BUG_ON(!desc);
		desc->virt[0] = (tuser >> 16) | (tuser << 16);
		desc->virt[1] = 0;
		desc->virt[2] = resp & 0xFFFFFF00; // mask TTL
		desc->used = 12;
		sd_fifo_tx_burst(sf, &desc, 1);
	}
}


/** RX DMA timeout callback */
static void sd_fifo_rx_dma_timeout (unsigned long param)
{
	struct sd_fifo *sf = (struct sd_fifo *)param;
	unsigned long   flags;

	sf->rx.stats.timeouts++;

	spin_lock_irqsave(&sf->rx.lock, flags);
	BUG_ON(!sf->rx_current);

	pr_err("%s: %s: RX DMA timeout: reset RX path\n", sf->name, __func__);
	dmaengine_terminate_all(sf->rx.chan);
	sd_fifo_reset(sf, SD_FR_RX);
	spin_unlock_irqrestore(&sf->rx.lock, flags);
}


static void sd_fifo_rx_dma (struct sd_fifo *sf, uint32_t size);


/** RX DMA completion callback */
static void sd_fifo_rx_dma_complete (void *param)
{
	struct sd_fifo  *sf = param;
	struct sd_desc  *desc;
	enum dma_status  status;
	unsigned long    flags;
	uint32_t         rlr;

	spin_lock_irqsave(&sf->rx.lock, flags);
	BUG_ON(!sf->rx_current);
	desc = sf->rx_current;

	del_timer(&sf->rx.timer);
	status = dma_async_is_tx_complete(sf->rx.chan, sf->rx.cookie, NULL, NULL);
	switch ( status == DMA_ERROR )
	{
		pr_err("%s: %s: RX DMA_ERROR, reset RX\n", sf->name, __func__);
		dmaengine_terminate_all(sf->rx.chan);
		sd_fifo_reset(sf, SD_FR_RX);
		spin_unlock_irqrestore(&sf->rx.lock, flags);
		return;
	}
	sf_debug(sf, "status %d\n", status);

	/* If size is set this is the ACK for the last chunk: dispatch and stop.
	 * After the last chunk is read and the packet dispatched, read RDFO to find out
	 * whether a next packet exists.  If RDFO is non-zero, re-read RLR and start handling
	 * the next packet.
	 */
	if ( desc->used )
	{
		spin_unlock_irqrestore(&sf->rx.lock, flags);
		sd_fifo_rx_finish(sf);
		return;
	}

	/* ACK for chunk before the last: re-read RLR to determine whether this is the last */
	rlr = REG_READ(&sf->regs->rlr);
	sf_trace(sf, "DMA: RLR %08x, desc %p\n", rlr, desc);

	if ( rlr & 0x80000000 ) /* partial */
	{
		sf_trace(sf, "  partial: %08x\n", rlr);
		sd_fifo_rx_dma(sf, rlr & 0x7FFFFFFF);
	}
	else /* remainder */
	{
#ifdef CONFIG_USER_MODULES_SRIO_DEV_FIFO_DEST
		desc->dest = REG_READ(&sf->regs->rdr);
		sf_trace(sf, "desc %p: read RDR %08x\n", desc, desc->dest);
#endif
		sf_trace(sf, "remainder: %08x - %08x = %08x\n", rlr, desc->offs, rlr - desc->offs);
		desc->used = rlr;
		sd_fifo_rx_dma(sf, rlr - desc->offs);
	}
	spin_unlock_irqrestore(&sf->rx.lock, flags);
}


/** Start RX DMA for all of the current RX descriptor's bulk data
 *
 *  \note  Caller should hold &sf->rx.lock
 *
 *  \param  sf    Pointer to sd_fifo struct
 *  \param  size  Number of bytes to transfer
 */
static void sd_fifo_rx_dma (struct sd_fifo *sf, uint32_t size)
{
	struct dma_async_tx_descriptor *xfer;
	struct sd_desc                 *desc;
	dma_addr_t                      phys;

	BUG_ON(!sf->rx_current);
	BUG_ON(!size);

	desc = sf->rx_current;
	BUG_ON(desc->offs + size > sizeof(uint32_t) * SD_FIFO_SIZE);

	/* setup dest within DMA buffer and advance offset for next chunk */
	phys = desc->phys + desc->offs;
	desc->offs += size;
	sf_debug(sf, "set up DMA, %08x bytes from %08x to %08x...\n", 
	         size, sf->rx.phys, phys);
	xfer = sf->rx.chan->device->device_prep_dma_memcpy(sf->rx.chan, phys, sf->rx.phys,
	                                                   size, DMA_CTRL_ACK);
	if ( !xfer )
	{
		pr_err("device_prep_dma_memcpy() failed\n");
		sf->rx.stats.errors++;
		sd_fifo_reset(sf, SD_FR_RX);
		return;
	}
	sf_trace(sf, "device_prep_dma_memcpy(): %p\n", xfer);

	/* setup completion callback */
	xfer->callback       = sd_fifo_rx_dma_complete;
	xfer->callback_param = sf;

	/* submit transaction */
	sf->rx.cookie = xfer->tx_submit(xfer);
	if ( dma_submit_error(sf->rx.cookie) )
	{
		pr_err("tx_submit() failed, stop\n");
		sf->rx.stats.errors++;
		sd_fifo_reset(sf, SD_FR_RX);
		return;
	}
	sf_trace(sf, "tx_submit() ok, cookie %lx\n", (unsigned long)sf->rx.cookie);

	/* failure timer - 1 sec */
	mod_timer(&sf->rx.timer, jiffies + sf->rx.timeout);

	/* start transaction and return */
	sf_trace(sf, "start RX and DMA\n");
	dma_async_issue_pending(sf->rx.chan);
	sf->rx.stats.chunks++;
}


/** Run RX PIO for a chunk of the current RX descriptor's bulk data
 *
 *  \note  Caller should hold &sf->rx.lock
 *
 *  \param  sf    Pointer to sd_fifo struct
 *  \param  size  Number of bytes to transfer
 */
static void sd_fifo_rx_pio (struct sd_fifo *sf, uint32_t size)
{
	struct sd_desc *desc;
	uint32_t       *walk;

	BUG_ON(!sf->rx_current);
	BUG_ON(size & 0x03);

	desc = sf->rx_current;
	walk = (desc->virt + desc->offs);
	BUG_ON(desc->offs + size > sizeof(uint32_t) * SD_FIFO_SIZE);

	sf_trace(sf, "size %08x, offs %08x\n", size, desc->offs);
	desc->offs += size;
	while ( size )
	{
		*walk++ = REG_READ(&sf->data->rdfd);
		size   -= sizeof(uint32_t);
	}
	sf->rx.stats.chunks++;
}


/******** Interrupt handling *******/


/** IRQ handler */
static irqreturn_t sd_fifo_interrupt (int irq, void *arg)
{
	struct sd_desc *desc;
	struct sd_fifo *sf  = (struct sd_fifo *)arg;
	unsigned long   flags;
	uint32_t        isr = REG_READ(&sf->regs->isr);
	uint32_t        ier = REG_READ(&sf->regs->ier);
	uint32_t        rlr;
	uint32_t        rdfo;
	int             rep = 0;

	REG_WRITE(&sf->regs->isr, isr);
	if ( isr & ~ier )
	{
		sd_dump_ixr(sf, "Quashed spurious", isr & ~ier);
		isr &= ier;
	}
	sd_dump_ixr(sf, "ISR", isr);

	/* TX Reset Complete: clear TFPE and re-enable TX path */
	if ( isr & TRC )
	{
		pr_info("TRC: resume normal service\n");
		isr &= ~TFPE;
		REG_RMW(&sf->regs->ier, TRC, TFPE|TSE|TC|TPOE);

		spin_lock_irqsave(&sf->tx.lock, flags);
		if ( !list_empty(&sf->tx_queue) )
		{
			pr_info("TRC: resuming pending TX\n");
			sd_fifo_tx_start(sf);
		}
		spin_unlock_irqrestore(&sf->tx.lock, flags);
	}

	/* TX Complete: upcall recently-finished TX descriptor so caller can clean it up, and
	 * start next descriptor if needed.  Note check for empty before the upcall: the
	 * caller may enqueue a new TX which will start the TX chain if this was the only
	 * descriptor.  If the caller doesn't enqueue a new descriptor the !empty check starts
	 * the next queued descriptor.
	 */
	if ( isr & TC )
	{
		spin_lock_irqsave(&sf->tx.lock, flags);
		if ( list_empty(&sf->tx_queue) )
			sf_debug(sf, "TC: IRQ when not transmitting?\n");
		else
		{
			int empty;
			desc = container_of(sf->tx_queue.next, struct sd_desc, list);
			list_del_init(&desc->list);
			empty = list_empty(&sf->tx_queue);

			sf_debug(sf, "TC: IRQ at end of TX desc %p, cleanup\n", desc);

			/* if response expected, add to retry list */
			if ( desc->resp )
			{
				sf_debug(sf, "desc %p: resp %08x expected, add to rx_retry\n",
				         desc, desc->resp);
				list_add_tail(&desc->list, &sf->tx_retry);
				if ( !timer_pending(&sf->rt_timer) )
				{
					sf_debug(sf, "start retry timer\n");
					mod_timer(&sf->rt_timer, jiffies + RT_TIMER_DELAY);
				}
			}
			else
			{
				sf_debug(sf, "desc %p: no resp expected, free\n", desc);
				/* unlock: caller may call _tx_enqueue */
				if ( sf->tx_func )
				{
					spin_unlock_irqrestore(&sf->tx.lock, flags);
					sf->tx_func(sf, desc->info, 0);
					spin_lock_irqsave(&sf->tx.lock, flags);
				}
				sd_desc_free(sf->sd, desc);
			}

			/* start next TX if needed */
			if ( !empty && !list_empty(&sf->tx_queue) )
			{
				sf_debug(sf, "TC: start next queued\n");
				sd_fifo_tx_start(sf);
			}
			else
			{
				isr &= ~TFPE;
				REG_RMW(&sf->regs->ier, TFPE|TFPF|TSE|TC|TPOE, 0);
			}
		}
		spin_unlock_irqrestore(&sf->tx.lock, flags);
	}

	/* TX FIFO Pointer Empty: In PIO mode TFPE fires when the TX FIFO drains below the
	 * threshold; send the next block
	 */
	if ( isr & TFPE )
	{
		spin_lock_irqsave(&sf->tx.lock, flags);
		if ( list_empty(&sf->tx_queue) )
			sf_trace(sf, "TFPE: IRQ when not transmitting?\n");
		else if ( sf->tx.chan )
			sf_trace(sf, "TFPE: ignored when using DMA\n");
		else
		{
			sf_trace(sf, "TFPE: send next block\n");
			sd_fifo_tx_pio(sf);
		}
		spin_unlock_irqrestore(&sf->tx.lock, flags);
	}

	/* TX Errors: Should never happen, start a reset */
	if ( isr & (TSE | TPOE) )
	{
		pr_err("TX error(s): { %s%s}, reset TX FIFO\n",
		       isr & TSE  ? "TSE "  : "",
		       isr & TPOE ? "TPOE " : "");
		sd_fifo_reset(sf, SD_FR_TX);
	}


	/* RX Reset Complete: clear RFPF and re-enable RX path */
	if ( isr & RRC )
	{
		sf_debug(sf, "RRC: resume normal service\n");
		isr &= ~RFPF;
		REG_RMW(&sf->regs->ier, RRC, RFPF|RC|RPUE|RPORE|RPURE);
	}

	/* RX Complete or RX FIFO Full: Main RX handling; the RC and RFPF IRQs are handled
	 * identically by this code, and may occur together for multiple RX packets.
	 * 
	 * Notes: Reads to the RLR have side-effects in the FIFO hardware, modify with care.
	 * The documentation (Xilinx PG080) is not clear on these or details of the RLR, for
	 * AXI4-Stream FIFO v4.0:
	 * - If a partial packet is present in the FIFO, the MSB will be set and the lower 31
	 *   bits will give the length of the partial packet in bytes.  This repeats across
	 *   multiple chunks which aren't the last chunk.
	 * - If either a full packet, or the last chunk of a long packet, is present, the MSB
	 *   will be clear and the lower 31 buts will give the length of the entire packet in
	 *   bytes.  This should be saved and the sum of all chunks which have been read to
	 *   that point deducted from it to determine the size of the last chunk to read.
	 *
	 * After the last chunk is read and the packet dispatched, read RDFO to find out
	 * whether a next packet exists.  If RDFO is non-zero, re-read RLR and start handling
	 * the next packet.  For PIO mode this is handled in the IRQ handler; for DMA mode
	 * it's handled in sd_fifo_rx_dma_complete() instead.
	 *
	 * If the RX queue is empty, run a PIO loop with sd_fifo_rx_flush() to drain the FIFO.
	 * TODO: add a counter for that
	 */
	if ( isr & (RC|RFPF) )
	{
		rlr = REG_READ(&sf->regs->rlr);

		spin_lock_irqsave(&sf->rx.lock, flags);

next:
		/* suppress handling spurious interrupts when RLR=0 */
		if ( rlr & 0x7fffffff )
		{
			rep++;
			sf_trace(sf, "%s%s: rep %d: RLR %08x\n",
		 	         isr & RC   ? "RC "   : "",
		 	         isr & RFPF ? "RFPF " : "",
			         rep, rlr);

			/* No current descriptor: alloc one from slab */
			if ( !sf->rx_current )
			{
				sf->rx_current = sd_desc_alloc(sf->sd, GFP_ATOMIC|GFP_DMA);
				sf_trace(sf, "rx_current now %p\n", sf->rx_current);

				if ( sf->rx_current )
				{
					desc = sf->rx_current;
					if ( sf->rx.chan && !desc->phys )
					{
						desc->phys = dma_map_single(sf->dev, desc->virt,
						                            sizeof(uint32_t) * SD_FIFO_SIZE,
						                            DMA_DEV_TO_MEM);
						BUG_ON(!desc->phys);
						sf_trace(sf, "RX DMA mapped %p -> %08x\n", desc->virt, desc->phys);
					}
					else
						sf_trace(sf, "Skip RX DMA map\n");
				}
			}

			/* queue empty: flush */
			if ( !sf->rx_current )
			{
				uint32_t size = 0;
				pr_err("RX ring full: flush\n");
				while ( rlr & 0x7fffffff )
				{
					if ( rlr & 0x80000000 ) /* partial */
					{
						size += rlr & 0x7FFFFFFF;
						pr_err("  partial: %08x\n", rlr);
						sd_fifo_rx_flush(sf, rlr & 0x7FFFFFFF);
					}
					else /* remainder */
					{
						pr_err("  remainder: %08x - %08x = %08x\n", rlr, size, rlr - size);
						sd_fifo_rx_flush(sf, rlr - size);
						size = 0;
					}
					rlr  = REG_READ(&sf->regs->rlr);
					sf_trace(sf, "  subseq RLR %08x\n", rlr);
				}
			}

			/* queue head valid: start (RFPF) or complete (RC) read */
			else
			{
				desc = sf->rx_current;
				if ( !desc->offs )
				{
					sf_trace(sf, "RX desc %p: start\n", desc);
					sf->rx.stats.starts++;
				}
				else
					sf_trace(sf, "RX desc %p: continue\n", desc);

				if ( rlr & 0x80000000 ) /* partial */
				{
					sf_trace(sf, "  partial: %08x\n", rlr);

					/* For DMA, disable subsequent IRQs, use DMA completion instead; error
					 * IRQs still enabled */
					if ( sf->rx.chan )
					{
						REG_RMW(&sf->regs->ier, RFPE|RFPF|RC, 0);
						sd_fifo_rx_dma(sf, rlr & 0x7FFFFFFF);
					}
					else
						sd_fifo_rx_pio(sf, rlr & 0x7FFFFFFF);
				}
				else /* remainder */
				{
					sf_trace(sf, "  remainder: %08x - %08x = %08x\n",
					         rlr, desc->offs, rlr - desc->offs);

					desc->used = rlr;
					rlr -= desc->offs;

#ifdef CONFIG_USER_MODULES_SRIO_DEV_FIFO_DEST
					desc->dest = REG_READ(&sf->regs->rdr);
					sf_trace(sf, "desc %p: read RDR %08x\n", desc, desc->dest);
#endif

					/* For DMA, disable subsequent IRQs, use DMA completion instead; error
					 * IRQs still enabled.  For PIO, read the last block and dispatch. */
					if ( sf->rx.chan )
					{
						sf_trace(sf, "start RX DMA\n");
						REG_RMW(&sf->regs->ier, RFPE|RFPF|RC, 0);
						sd_fifo_rx_dma(sf, rlr);
					}
					else
					{
						sf_trace(sf, "start RX PIO\n");
						sd_fifo_rx_pio(sf, rlr);

						/* Finish RX unlocked */
						spin_unlock_irqrestore(&sf->rx.lock, flags);
						sd_fifo_rx_finish(sf);
						spin_lock_irqsave(&sf->rx.lock, flags);

						/* after PIO, if RDFO is nonzero there's the beginnings of a new
						 * packet in the FIFO, which isn't enough to have triggered RFPF
						 */
						if ( (rdfo = REG_READ(&sf->regs->rdfo)) )
						{
							sf_trace(sf, "RDFO %08x after PIO, read next\n", rdfo);
							rlr = REG_READ(&sf->regs->rlr);
							goto next;
						}
					} /* if ( sf->rx.chan ) */
				} /* if ( rlr & 0x80000000 ) */
			} /* if ( !sf->rx_current ) */
		} /* if ( rlr & 0x7fffffff ) */

		spin_unlock_irqrestore(&sf->rx.lock, flags);

		sf_trace(sf, "Past PIO read loop after %d reps\n", rep);
	}

	/* RX Errors: Should never happen, start a reset */
	if ( isr & (RPUE | RPORE | RPURE) )
	{
		pr_err("RX error(s): { %s%s%s}, reset RX FIFO\n",
		       isr & RPUE  ? "RPUE "  : "",
		       isr & RPORE ? "RPORE " : "",
		       isr & RPURE ? "RPURE " : "");

		spin_lock_irqsave(&sf->rx.lock, flags);
		sd_fifo_reset(sf, SD_FR_RX);
		spin_unlock_irqrestore(&sf->rx.lock, flags);
	}

	if ( (isr = REG_READ(&sf->regs->isr)) )
		sd_dump_ixr(sf, "leaving, ISR", isr);
	else
		sf_trace(sf, "leaving\n");
	return IRQ_HANDLED;
}


/******** Init / Exit *******/


/** Setup RX or TX direction struct
 *
 *  \param  fd       Pointer to sd_fifo_dir struct
 *  \param  func     Pointer to callback function
 *  \param  timeout  Timeout for DMA operations in jiffies
 */
void sd_fifo_init_tx (struct sd_fifo *sd, sd_tx_callback func, unsigned long timeout)
{
	sd->tx_func = func;
	sd->tx.timeout = timeout;
}
void sd_fifo_init_rx (struct sd_fifo *sd, sd_rx_callback func, unsigned long timeout)
{
	sd->rx_func = func;
	sd->rx.timeout = timeout;
}


/** Callback function for dma_request_channel() to select appropriate DMA channel */
static bool sd_fifo_init_dma_find (struct dma_chan *chan, void *param)
{
	enum dma_transfer_direction  dir = (enum dma_transfer_direction)param;
	struct dma_slave_caps        caps;

	if ( dma_get_slave_caps(chan, &caps) )
	{
		pr_debug("Candidate chan %s has no slave_caps, skip\n",
		         dev_name(chan->device->dev));
		return 0;
	}

	pr_debug("Candidate chan %s has caps.directions %x, want %x, %s\n",
	         dev_name(chan->device->dev), caps.directions, (1 << dir), 
			 (caps.directions & (1 << dir)) > 0 ? "keep" : "skip");
	return (caps.directions & (1 << dir)) > 0;
}


/** Init DMA for RX or TX direction
 *
 *  \param  fd   Pointer to sd_fifo_dir struct
 *  \param  dir  DMA_DEV_TO_MEM for RX, DMA_MEM_TO_DEV for TX
 *
 *  \return  0 on success, <0 on error
 */
static int sd_fifo_init_dma (struct sd_fifo_dir *fd, enum dma_transfer_direction dir)
{
	dma_cap_mask_t  mask;
	dma_cap_zero(mask);
	dma_cap_set(DMA_MEMCPY, mask);

	fd->chan = dma_request_channel(mask, sd_fifo_init_dma_find, (void *)dir);
	if ( !fd->chan )
	{
		pr_err("%s: dma_request_channel() failed, stop\n", __func__);
		return -1;
	}

	pr_debug("%s: got DMA channel: %s\n", __func__, dev_name(fd->chan->device->dev));
	return 0;
}


/** Init FIFO
 *
 *  \param  pdev  Platform device struct pointer
 *  \param  pref  Prefix for names in devicetree
 *
 *  \return  sd_fifo struct on success, NULL on error
 */
struct sd_fifo *sd_fifo_probe (struct platform_device *pdev, char *pref, unsigned flags)
{
	struct sd_fifo  *sf;
	struct resource *regs;
	struct resource *data;
	struct resource *irq;
	char             name[32];
	int              ret = 0;
	u32              dma = 0;

	if ( !(sf = kzalloc(sizeof(*sf), GFP_KERNEL)) )
	{
		dev_err(&pdev->dev, "memory alloc fail, stop\n");
		return NULL;
	}
	sf->dev = &pdev->dev;
	sf->flags = flags;
	snprintf(sf->name, sizeof(sf->name), "%s.%s", dev_name(sf->dev), pref); 


	snprintf(name, sizeof(name), "%s-regs", pref);
	if ( !(regs = platform_get_resource_byname(pdev, IORESOURCE_MEM, name)) )
		goto resource;

	snprintf(name, sizeof(name), "%s-irq", pref);
	if ( !(irq = platform_get_resource_byname(pdev, IORESOURCE_IRQ, name)) )
		goto resource;

	snprintf(name, sizeof(name), "%s-data", pref);
	if ( !(data = platform_get_resource_byname(pdev, IORESOURCE_MEM, name)) )
		data = regs;

	sf_debug(sf, "using config: regs %08x, data %08x, irq %d\n",
	         regs->start, data->start, irq->start);

	sf->regs = devm_ioremap_resource(sf->dev, regs);
	if ( IS_ERR(sf->regs) )
	{
		dev_err(sf->dev, "regs ioremap fail, stop\n");
		goto free;
	}
	sf_debug(sf, "%s: regs mapped %08x -> %p\n", sf->name, regs->start, sf->regs);

	if ( data == regs )
		sf->data = sf->regs;
	else
	{
		sf->data = devm_ioremap_resource(sf->dev, data);
		if ( IS_ERR(sf->data) )
		{
			dev_err(sf->dev, "data ioremap fail, stop\n");
			goto unmap_regs;
		}
		sf_debug(sf, "%s: data mapped %08x -> %p\n", sf->name, data->start, sf->data);
	}
	sf->phys = data->start;

	sf->irq = irq->start;
	ret = request_irq(sf->irq, sd_fifo_interrupt, IRQF_TRIGGER_HIGH, dev_name(sf->dev), sf);
	if ( ret )
	{
		pr_err("request_irq(%d): %d, stop\n", sf->irq, ret);
		goto unmap_data;
	}
	pr_info("IRQ: %d: OK\n", sf->irq);

	dma = (1 << DMA_MEM_TO_DEV) | (1 << DMA_DEV_TO_MEM);
	snprintf(name, sizeof(name), "sbt,%s-dma", pref);
	of_property_read_u32(pdev->dev.of_node, name, &dma);
	if ( dma & (1 << DMA_MEM_TO_DEV) )
	{
		sf_debug(sf, "Try to setup DMA for TX dir...\n");
		if ( sd_fifo_init_dma(&sf->tx, DMA_MEM_TO_DEV) )
		{
			ret = -1;
			goto irq;
		}
		sf->tx.phys = sf->phys + offsetof(struct sd_fifo_regs, tdfd);
		sf_debug(sf, "%s: TDFD: %08x phys\n", sf->name, sf->tx.phys);
	}

	if ( dma & (1 << DMA_DEV_TO_MEM) )
	{
		sf_debug(sf, "Try to setup DMA for RX dir...\n");
		if ( sd_fifo_init_dma(&sf->rx, DMA_DEV_TO_MEM) )
		{
			ret = -1;
			goto dma;
		}
		sf->rx.phys = sf->phys + offsetof(struct sd_fifo_regs, rdfd);
		sf_debug(sf, "%s: RDFD: %08x phys\n", sf->name, sf->rx.phys);
	}

	spin_lock_init(&sf->rx.lock);
	spin_lock_init(&sf->tx.lock);

	setup_timer(&sf->rx.timer, sd_fifo_rx_dma_timeout, (unsigned long)sf);
	setup_timer(&sf->tx.timer, sd_fifo_tx_dma_timeout, (unsigned long)sf);
	setup_timer(&sf->rt_timer, sd_fifo_tx_retry,       (unsigned long)sf);

	sf->rx_current = NULL;
	INIT_LIST_HEAD(&sf->tx_queue);
	INIT_LIST_HEAD(&sf->tx_retry);

	/* Clear pending IRQs */
	REG_WRITE(&sf->regs->isr, 0xFFFFFFFF);

	/* Reset FIFOs on startup, including a LLR / SRR reset of the Stream interface
	 * For some reason this zeros the IER */
	sd_fifo_reset(sf, SD_FR_ALL);

	/* Full reset of the FIFO zeroes the IER */
	REG_WRITE(&sf->regs->ier, TRC|RRC);

	sf_debug(sf, "Success\n");
	return sf;


dma:
	dma_release_channel(sf->tx.chan);

irq:
	free_irq(sf->irq, sf);

unmap_data:
	if ( sf->data != sf->regs )
		devm_iounmap(sf->dev, sf->data);

unmap_regs:
	devm_iounmap(sf->dev, sf->regs);

free:
	kfree(sf);
	return NULL;


resource:
	dev_err(sf->dev, "resource %s undefined.\n", name);
	return NULL;
}


/** Free FIFO
 *
 *  \param  sf   Pointer to sd_fifo struct
 */
void sd_fifo_free (struct sd_fifo *sf)
{
	/* Disable all IRQs first */
	REG_WRITE(&sf->regs->ier, 0);

	del_timer(&sf->rt_timer);

	if ( sf->tx.chan ) dma_release_channel(sf->tx.chan);
	if ( sf->rx.chan ) dma_release_channel(sf->rx.chan);

	free_irq(sf->irq, sf);

	if ( sf->data != sf->regs )
		devm_iounmap(sf->dev, sf->data);

	devm_iounmap(sf->dev, sf->regs);

	/* Free TX queues and RX current */
	sd_fifo_tx_discard(sf);
	if ( sf->rx_current )
		sd_desc_free(sf->sd, sf->rx_current);

	kfree(sf);
}

