/** \file      sd_user.c
 *  \brief     Temporary userspace interface
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
#include <linux/sched.h>
#include <linux/miscdevice.h>
#include <linux/dmaengine.h>
#include <linux/uaccess.h>
#include <linux/ktime.h>
#include <linux/hrtimer.h>
#include <linux/poll.h>
#include <linux/fs.h>
#include <linux/ctype.h>


#include "srio_dev.h"
#include "sd_regs.h"
#include "sd_desc.h"
#include "sd_fifo.h"
#include "sd_user.h"


/******* Static vars *******/

static struct miscdevice  mdev;
static struct srio_dev   *sd_user_dev;

/* List of sd_user_priv structs currently open */
static spinlock_t  lock;
struct list_head   list;

struct sd_user_priv
{
	wait_queue_head_t  wait;
	spinlock_t         lock;
	struct list_head   queue;
	struct list_head   list;
	uint32_t           tuser;
	unsigned long      mbox_sub;
	uint16_t           dbell_min;
	uint16_t           dbell_max;
	uint64_t           swrite_min;
	uint64_t           swrite_max;
};

struct sd_user_q_mesg
{
	struct list_head     list;
	struct sd_user_mesg  mesg;
};



static void hexdump_line (const unsigned char *ptr, const unsigned char *org, int len)
{
	char  buff[80];
	char *p = buff;
	int   i;

	p += sprintf (p, "%04x: ", (unsigned)(ptr - org));

	for ( i = 0; i < len; i++ )
		p += sprintf (p, "%02x ", ptr[i]);

	for ( ; i < 16; i++ )
	{
		*p++ = ' ';
		*p++ = ' ';
		*p++ = ' ';
	}

	for ( i = 0; i < len; i++ )
		*p++ = (ptr[i] > 31 && ptr[i] < 127) ? ptr[i] : '.';
	*p = '\0';

	pr_debug("%s\n", buff);
}


void hexdump_buff (const void *buf, int len)
{
	const unsigned char *org = buf;
	const unsigned char *ptr = buf;
	unsigned char        dup[16];
	int                  sup = 0;

	while ( len >= 16 )
	{
		if ( memcmp(dup, ptr, 16) )
		{
			if ( sup )
			{
				printk("* (%d duplicates)\n", sup);
				sup = 0;
			}
			hexdump_line(ptr, org, 16);
			memcpy(dup, ptr, 16);
		}
		else
			sup++;
		ptr += 16;
		len -= 16;
	}
	if ( sup )
		printk("* (%d duplicates)\n", sup);
	if ( len )
		hexdump_line(ptr, org, len);
}


/******* RX mbox handling *******/


void sd_user_recv_mbox (struct sd_desc *desc, int mbox)
{
	struct sd_user_priv *priv;
	struct list_head    *walk;
	unsigned long        flags;
	uint32_t            *hdr  = desc->virt;
	size_t               size = offsetof(struct sd_user_q_mesg, mesg) + 
	                            offsetof(struct sd_user_mesg, mesg) + 
	                            offsetof(struct sd_user_mesg_mbox, data) + 
	                            desc->used - 12;

	pr_debug("MESSAGE: dispatch %zu bytes to %d listeners (%zu payload)\n",
	         size, sd_user_dev->swrite_users, desc->used - 12);

	pr_debug("  header size %u, expect %u, seg %d/%d\n", 
	         ((hdr[2] >> 4) & 0xFF) + 1, desc->used - 12,
			 (hdr[2] >> 24) & 0xF, (hdr[2] >> 28) & 0xF);

	/* Dispatch to users */
	spin_lock_irqsave(&lock, flags);
	list_for_each(walk, &list)
	{
		priv = container_of(walk, struct sd_user_priv, list);
		if ( priv->mbox_sub & (1 << mbox) )
		{
			struct sd_user_q_mesg *qm = kmalloc(size, GFP_ATOMIC);
			if ( !qm )
			{
				pr_err("swrite dropped: kmalloc() failed\n");
				continue;
			}
			pr_debug("    dispatch\n");

			INIT_LIST_HEAD(&qm->list);

			qm->mesg.type     = 11;
			qm->mesg.size     = size - offsetof(struct sd_user_q_mesg, mesg);
			qm->mesg.dst_addr = hdr[0] >> 16;
			qm->mesg.src_addr = hdr[0] & 0xFFFF;
			qm->mesg.hello[0] = hdr[1];
			qm->mesg.hello[1] = hdr[2];

			qm->mesg.mesg.mbox.mbox   = mbox;
			qm->mesg.mesg.mbox.letter = hdr[1] & 0x03;
			memcpy(qm->mesg.mesg.mbox.data, desc->virt + 12, desc->used - 12);

			list_add_tail(&qm->list, &priv->queue);

			hexdump_buff(qm, size);
			wake_up_interruptible(&priv->wait);
		}
	}
	spin_unlock_irqrestore(&lock, flags);

	// return 0: desc consumed and can be reused, return !0: desc held for reassembly
//	return 0;
}

void sd_user_recv_swrite (struct sd_desc *desc, uint64_t addr)
{
	struct sd_user_priv *priv;
	struct list_head    *walk;
	unsigned long        flags;
	uint32_t            *hdr  = desc->virt;
	size_t               size = offsetof(struct sd_user_q_mesg, mesg) + 
	                            offsetof(struct sd_user_mesg, mesg) + 
	                            offsetof(struct sd_user_mesg_swrite, data) + 
	                            desc->used - 12;

	pr_debug("SWRITE: dispatch %zu bytes to %d listeners (%zu payload)\n",
	         size, sd_user_dev->swrite_users, desc->used - 12);

	/* Dispatch to users */
	spin_lock_irqsave(&lock, flags);
	list_for_each(walk, &list)
	{
		priv = container_of(walk, struct sd_user_priv, list);
		pr_debug("  %p: %09llx..%09llx\n", priv, priv->swrite_min, priv->swrite_max);
		if ( addr >= priv->swrite_min && addr <= priv->swrite_max )
		{
			struct sd_user_q_mesg *qm = kmalloc(size, GFP_ATOMIC);
			if ( !qm )
			{
				pr_err("swrite dropped: kmalloc() failed\n");
				continue;
			}
			pr_debug("    dispatch\n");

			INIT_LIST_HEAD(&qm->list);

			qm->mesg.type     = 6;
			qm->mesg.size     = size - offsetof(struct sd_user_q_mesg, mesg);
			qm->mesg.dst_addr = hdr[0] >> 16;
			qm->mesg.src_addr = hdr[0] & 0xFFFF;
			qm->mesg.hello[0] = hdr[1];
			qm->mesg.hello[1] = hdr[2];

			qm->mesg.mesg.swrite.addr = addr;
			memcpy(qm->mesg.mesg.swrite.data, desc->virt + 12, desc->used - 12);

			list_add_tail(&qm->list, &priv->queue);

			hexdump_buff(qm, size);
			wake_up_interruptible(&priv->wait);
		}
	}
	spin_unlock_irqrestore(&lock, flags);
}

void sd_user_recv_dbell (struct sd_desc *desc, uint16_t info)
{
	struct sd_user_priv *priv;
	struct list_head    *walk;
	unsigned long        flags;
	uint32_t            *hdr  = desc->virt;
	size_t               size = offsetof(struct sd_user_q_mesg, mesg) + 
	                            offsetof(struct sd_user_mesg, mesg) + 
	                            sizeof(struct sd_user_mesg_dbell);

	pr_debug("DBELL: info %04x, dispatch %zu bytes to %d listeners\n",
	         info, size, sd_user_dev->dbell_users);

	/* Dispatch to users */
	spin_lock_irqsave(&lock, flags);
	list_for_each(walk, &list)
	{
		priv = container_of(walk, struct sd_user_priv, list);
		pr_debug("  %p: %04x..%04x\n", priv, priv->dbell_min, priv->dbell_max);
		if ( info >= priv->dbell_min && info <= priv->dbell_max )
		{
			struct sd_user_q_mesg *qm = kmalloc(size, GFP_ATOMIC);
			if ( !qm )
			{
				pr_err("DBELL dropped: kmalloc() failed\n");
				continue;
			}

			INIT_LIST_HEAD(&qm->list);

			qm->mesg.type     = 10;
			qm->mesg.size     = size - offsetof(struct sd_user_q_mesg, mesg);
			qm->mesg.dst_addr = hdr[0] >> 16;
			qm->mesg.src_addr = hdr[0] & 0xFFFF;
			qm->mesg.hello[0] = hdr[1];
			qm->mesg.hello[1] = hdr[2];

			qm->mesg.mesg.dbell.info = info;

			list_add_tail(&qm->list, &priv->queue);

			hexdump_buff(qm, size);
			wake_up_interruptible(&priv->wait);
		}
	}
	spin_unlock_irqrestore(&lock, flags);
}


static const char *fifo_name[2] = { "targ", "init" };
void sd_user_recv_mesg (struct sd_fifo *fifo, struct sd_desc *desc, int init)
{
	uint32_t *hdr = desc->virt;
	int       mbox;
	int       type;

	pr_debug("RX %zu bytes on %s fifo\n", desc->used, fifo_name[init]);
	hexdump_buff(desc->virt, desc->used);

	if ( desc->used < 12 )
	{
		pr_err("Short message ignored\n");
		goto done;
	}

	type = (hdr[2] >> 20) & 0x0F;
	switch ( type )
	{
		// SWRITE: parse/dispatch if users registered, requeue
		case 6:
			if ( sd_user_dev->swrite_users )
			{
				uint64_t  addr = hdr[2] & 3;
				addr <<= 32;
				addr  |= hdr[1];
				sd_user_recv_swrite(desc, addr);
			}
			else
				pr_debug("SWRITE dropped: no users listening\n");
			break;

		// SWRITE and DBELL: dispatch and requeue
		case 10:
			if ( sd_user_dev->dbell_users )
			{
				uint16_t  info = hdr[1] >> 16;
				sd_user_recv_dbell(desc, info);
			}
			else
				pr_debug("DBELL dropped: no users listening\n");
			break;

		// MESSAGE: descriptor held during reassembly, requeue when done
		case 11: 
			mbox = (hdr[1] >> 3) & 0x3F;
			if ( mbox < RIO_MAX_MBOX && sd_user_dev->mbox_users[mbox] )
				sd_user_recv_mbox(desc, mbox);
			else 
				pr_err("MESSAGE: mbox 0x%x, dropped\n", mbox);
			break;

		default:
			pr_debug("Unsupported type %d ignored\n", type);
			break;
	}

done:
	sd_fifo_rx_enqueue(fifo, desc);
}


void sd_user_recv_targ (struct sd_fifo *fifo, struct sd_desc *desc)
{
	sd_user_recv_mesg(fifo, desc, 0);
}
void sd_user_recv_init (struct sd_fifo *fifo, struct sd_desc *desc)
{
	sd_user_recv_mesg(fifo, desc, 1);
}


void sd_user_tx_done (struct sd_fifo *fifo, struct sd_desc *desc)
{
	pr_debug("%s: TX desc %p done\n", __func__, desc);
	sd_desc_free(desc);
	kfree(desc);
}


/******* VFS glue, miscdevice, init/exit *******/


static void sd_user_mbox_count (void)
{
	struct sd_user_priv *priv;
	struct list_head    *walk;
	unsigned long        flags;
	int                  count[RIO_MAX_MBOX];
	int                  mbox;

	memset(count, 0, sizeof(count));
	spin_lock_irqsave(&lock, flags);
	list_for_each(walk, &list)
	{
		priv = container_of(walk, struct sd_user_priv, list);
		for ( mbox = 0; mbox < RIO_MAX_MBOX; mbox++ )
			if ( priv->mbox_sub & (1 << mbox) )
				count[mbox]++;
	}
	for ( mbox = 0; mbox < RIO_MAX_MBOX; mbox++ )
		sd_user_dev->mbox_users[mbox] = count[mbox];
	spin_unlock_irqrestore(&lock, flags);
}

static void sd_user_dbell_count (void)
{
	struct sd_user_priv *priv;
	struct list_head    *walk;
	unsigned long        flags;
	int                  count = 0;

	spin_lock_irqsave(&lock, flags);
	list_for_each(walk, &list)
	{
		priv = container_of(walk, struct sd_user_priv, list);
		if ( priv->dbell_min <= priv->dbell_max )
			count++;
	}
	sd_user_dev->dbell_users = count;
	spin_unlock_irqrestore(&lock, flags);
}

static void sd_user_swrite_count (void)
{
	struct sd_user_priv *priv;
	struct list_head    *walk;
	unsigned long        flags;
	int                  count = 0;

	spin_lock_irqsave(&lock, flags);
	list_for_each(walk, &list)
	{
		priv = container_of(walk, struct sd_user_priv, list);
		if ( priv->swrite_min <= priv->swrite_max )
			count++;
	}
	sd_user_dev->swrite_users = count;
	spin_unlock_irqrestore(&lock, flags);
}


static int sd_user_open (struct inode *i, struct file *f)
{
	struct sd_user_priv *priv;
	unsigned long        flags;

	if ( !(priv = kzalloc(sizeof(*priv), GFP_KERNEL)) )
		return -ENOMEM;

	spin_lock_init(&priv->lock);
	init_waitqueue_head(&priv->wait);
	INIT_LIST_HEAD(&priv->queue);
	INIT_LIST_HEAD(&priv->list);
	priv->tuser      = 0x00020002;
	priv->mbox_sub   = 0;
	priv->dbell_min  = 0xFFFF;
	priv->dbell_max  = 0x0000;
	priv->swrite_min = 0x3FFFFFFFF;
	priv->swrite_max = 0x000000000;

	f->private_data = priv;
	pr_debug("%s: open: f %p, priv %p\n", __func__, f, priv);

	/* Add to list of open users */
	spin_lock_irqsave(&lock, flags);
	list_add_tail(&priv->list, &list);
	spin_unlock_irqrestore(&lock, flags);

// temporary
//sd_user_mbox_count();
//sd_user_dbell_count();
//sd_user_swrite_count();

	return 0;
}

static unsigned int sd_user_poll (struct file *f, poll_table *p)
{
	struct sd_user_priv *priv = f->private_data;
	unsigned long        flags;
	unsigned int         mask = POLLOUT | POLLWRNORM; /* Can always write */

	poll_wait(f, &priv->wait, p);

	spin_lock_irqsave(&priv->lock, flags);
	if ( !list_empty(&priv->queue) )
		mask = POLLIN | POLLRDNORM;
	spin_unlock_irqrestore(&priv->lock, flags);

	return mask;
}

static ssize_t sd_user_read (struct file *f, char __user *b, size_t s, loff_t *o)
{
	struct sd_user_priv   *priv = f->private_data;
	struct sd_user_q_mesg *qm;
	unsigned long          flags;
	size_t                 size;

	// no data available for read: return EAGAIN or sleep
	while ( list_empty(&priv->queue) )
	{
		if ( f->f_flags & O_NONBLOCK )
		{
			pr_debug("%s: read: empty & nonblock, return -EAGAIN\n", __func__);
			return -EAGAIN;
		}

		pr_debug("%s: read: empty & block, wait...\n", __func__);
		if ( wait_event_interruptible(priv->wait, (! list_empty(&priv->queue)) ) )
		{
			pr_debug("%s: read: empty & block, waited, -ERESTARTSYS\n", __func__);
			return -ERESTARTSYS;
		}
	}
	pr_debug("%s: read: not empty\n", __func__);

	// detach mesg from queue
	spin_lock_irqsave(&priv->lock, flags);
	qm = container_of(priv->queue.next, struct sd_user_q_mesg, list);
	size = qm->mesg.size;
	list_del_init(&qm->list);
	spin_unlock_irqrestore(&priv->lock, flags);
	pr_debug("%s: type %d size %zu\n", __func__, qm->mesg.type, qm->mesg.size);

	if ( size > s || copy_to_user(b, &qm->mesg, size) )
	{
		pr_err("Message dropped\n");
		kfree(qm);
		return -ENOSPC;
	}
	kfree(qm);

	// fix up position and return number of bytes copied
	*o +=  size;
	return size;
}


static ssize_t sd_user_write (struct file *f, const char __user *b, size_t s, loff_t *o)
{
	struct sd_user_mesg *mesg;
	struct sd_desc      *desc;
	u32                 *hdr;
	int                  size = s;

	pr_debug("%s(f, b %p, s %zu, o %lld)\n", __func__, b, s, *o);
	if ( s > PAGE_SIZE || s < offsetof(struct sd_user_mesg, mesg) )
		return -EINVAL;

	if ( !(mesg = (struct sd_user_mesg *)__get_free_page(GFP_KERNEL)) )
		return -ENOMEM;

	if ( copy_from_user(mesg, b, s) )
	{
		free_page((unsigned long)mesg);
		return -EFAULT;
	}

	if ( !(desc = kzalloc(sizeof(*desc), GFP_KERNEL)) )
	{
		free_page((unsigned long)mesg);
		return -ENOMEM;
	}

	if ( sd_desc_alloc(desc, BUFF_SIZE, GFP_KERNEL) )
	{
		kfree(desc);
		free_page((unsigned long)mesg);
		return -ENOMEM;
	}

	// TUSER word, then HELLO header (see PG007 Fig 3-1)
	hdr = (u32 *)desc->virt;
	hdr[0] = (mesg->src_addr << 16) | mesg->dst_addr;
	switch ( mesg->type )
	{
		case 6: // SWRITE
			hdr[1]  = mesg->mesg.swrite.addr & 0xFFFFFFFF;
			hdr[2]  = (mesg->mesg.swrite.addr >> 32) & 0x03;
			hdr[2] |= 0x00600000;
			size -= offsetof(struct sd_user_mesg,        mesg) +
			        offsetof(struct sd_user_mesg_swrite, data);
			memcpy(desc->virt + 12, mesg->mesg.swrite.data, size);
			desc->used = size + 12;
			break;

		case 10: // DBELL
			hdr[1] = mesg->mesg.dbell.info << 16;
			hdr[2] = 0x00A00000; // TODO: TID
			desc->used = 12;
			break;

		case 11: // MESSAGE
			size -= offsetof(struct sd_user_mesg,      mesg) +
			        offsetof(struct sd_user_mesg_mbox, data);
			if ( size > 256 ) // fragmentation not yet supported
			{
				sd_desc_free(desc);
				kfree(desc);
				free_page((unsigned long)mesg);
				return -EFBIG;
			}
			hdr[1] = (mesg->mesg.mbox.letter & 3) | (mesg->mesg.mbox.mbox & 0x3F) << 4;
			hdr[2] = (size - 1) << 4 | 0x00B00000;
			memcpy(desc->virt + 12, mesg->mesg.mbox.data, size);
			desc->used = size + 12;
			break;

		default:
			sd_desc_free(desc);
			kfree(desc);
			free_page((unsigned long)mesg);
			return -EINVAL;
	}
	free_page((unsigned long)mesg);

	if ( desc->used & 0x03 )
		desc->used = (desc->used + 1) & ~0x03;

	pr_debug("%s desc %p used %zu\n", __func__, desc, desc->used);

	// enqueue in initiator fifo, desc will be cleaned up in sd_user_tx_done()
	sd_fifo_tx_enqueue(sd_user_dev->init_fifo, desc);

	return s;
}


static long sd_user_ioctl (struct file *f, unsigned int cmd, unsigned long arg)
{
	struct sd_user_priv *priv = f->private_data;
	unsigned long        flags;
	unsigned long        val;
	uint16_t             u16x2[2];
	uint64_t             u64x2[2];

	if ( _IOC_TYPE(cmd) != MAGIC_SD_USER )
		return -EINVAL;

	printk("%s(f, cmd %x, arg %lx)\n", __func__, cmd, arg);

	switch ( cmd )
	{
		// Get local device-ID (return address on TX, filtered on RX)
		case SD_USER_IOCG_LOC_DEV_ID:
			val = priv->tuser & 0xFFFF;
			return put_user(val, (unsigned long *)arg);

		// Set local device-ID (return address on TX, filtered on RX)
		case SD_USER_IOCS_LOC_DEV_ID:
			spin_lock_irqsave(&priv->lock, flags);
			priv->tuser &= ~0xFFFF;
			priv->tuser |= arg & 0xFFFF;
			spin_unlock_irqrestore(&priv->lock, flags);
			return 0;


		// Get bitmask of mailbox subscriptions.  To subscribe to a mailbox set the
		// appropriate bit: ie to subscribe to mailbox 2 set bit (1 << 2), to unsubscribe
		// clear the same bit
		case SD_USER_IOCG_MBOX_SUB:
			val = priv->mbox_sub;
			return put_user(val, (unsigned long *)arg);

		// Set bitmask of mailbox subscriptions.  To subscribe to a mailbox set the
		// appropriate bit: ie to subscribe to mailbox 2 set bit (1 << 2), to unsubscribe
		// clear the same bit
		case SD_USER_IOCS_MBOX_SUB:
			spin_lock_irqsave(&priv->lock, flags);
			priv->mbox_sub = arg;
			spin_unlock_irqrestore(&priv->lock, flags);
			sd_user_mbox_count();
			pr_debug("%p: set mbox_sub %lx\n", priv, priv->mbox_sub);
			return 0;


		// Get range of doorbell values to be notified for
		case SD_USER_IOCG_DBELL_SUB:
			u16x2[0] = priv->dbell_min;
			u16x2[1] = priv->dbell_max;
			return copy_to_user((void *)arg, u16x2, sizeof(u16x2));

		// Set range of doorbell values to be notified for
		case SD_USER_IOCS_DBELL_SUB:
			if ( copy_from_user(u16x2, (void *)arg, sizeof(u16x2)) )
				return -EFAULT;

			if ( u16x2[0] > 0xFFFF || u16x2[1] > 0xFFFF )
				return -EINVAL;

			if ( u16x2[0] > u16x2[1] )
			{
				uint16_t tmp = u16x2[0];
				u16x2[0]     = u16x2[1];
				u16x2[1]     = tmp;
			}

			spin_lock_irqsave(&priv->lock, flags);
			priv->dbell_min = u16x2[0];
			priv->dbell_max = u16x2[1];
			spin_unlock_irqrestore(&priv->lock, flags);
			sd_user_dbell_count();
			pr_debug("%p: set dbell_min %x, max %x\n",
			         priv, priv->dbell_min, priv->dbell_max);
			return 0;


		// Get range of swrite addresses to be notified for
		case SD_USER_IOCG_SWRITE_SUB:
			u64x2[0] = priv->swrite_min;
			u64x2[1] = priv->swrite_max;
			return copy_to_user((void *)arg, u64x2, sizeof(u64x2));

		// Set range of swrite addresses to be notified for
		case SD_USER_IOCS_SWRITE_SUB:
			if ( copy_from_user(u64x2, (void *)arg, sizeof(u64x2)) )
				return -EFAULT;

			if ( u64x2[0] > 0x3FFFFFFFF || u64x2[1] > 0x3FFFFFFFF )
				return -EINVAL;

			if ( u64x2[0] > u64x2[1] )
			{
				uint64_t tmp = u64x2[0];
				u64x2[0]     = u64x2[1];
				u64x2[1]     = tmp;
			}

			spin_lock_irqsave(&priv->lock, flags);
			priv->swrite_min = u64x2[0];
			priv->swrite_max = u64x2[1];
			spin_unlock_irqrestore(&priv->lock, flags);
			sd_user_swrite_count();
			pr_debug("%p: set swrite_min %llx, max %llx\n",
			         priv, priv->swrite_min, priv->swrite_max);
			return 0;


		// SYS_REG controls
		case SD_USER_IOCS_SRIO_RESET:
			pr_debug("SD_USER_IOCS_SRIO_RESET:\n");
			sd_regs_srio_reset(sd_user_dev);
			return 0;

		case SD_USER_IOCG_STATUS:
			val = REG_READ(&sd_user_dev->sys_regs->ctrl);
			pr_debug("SD_USER_IOCG_STATUS: %08lx\n", val);
			return put_user(val, (unsigned long *)arg);

		case SD_USER_IOCG_GT_LOOPBACK:
			val = sd_regs_get_gt_loopback(sd_user_dev);
			pr_debug("SD_USER_IOCG_GT_LOOPBACK: %lu\n", val);
			return put_user(val, (unsigned long *)arg);

		case SD_USER_IOCS_GT_LOOPBACK:
			pr_debug("SD_USER_IOCS_GT_LOOPBACK: %lu\n", arg);
			sd_regs_set_gt_loopback(sd_user_dev, arg);
			return 0;

		case SD_USER_IOCG_GT_DIFFCTRL:
			val = sd_regs_get_gt_diffctrl(sd_user_dev);
			pr_debug("SD_USER_IOCG_GT_DIFFCTRL: %lu\n", val);
			return put_user(val, (unsigned long *)arg);

		case SD_USER_IOCS_GT_DIFFCTRL:
			pr_debug("SD_USER_IOCS_GT_DIFFCTRL: %lu\n", arg);
			sd_regs_set_gt_diffctrl(sd_user_dev, arg);
			return 0;

		case SD_USER_IOCG_GT_TXPRECURS:
			val = sd_regs_get_gt_txprecursor(sd_user_dev);
			pr_debug("SD_USER_IOCG_GT_TXPRECURS: %lu\n", val);
			return put_user(val, (unsigned long *)arg);

		case SD_USER_IOCS_GT_TXPRECURS:
			pr_debug("SD_USER_IOCS_GT_TXPRECURS: %lu\n", arg);
			sd_regs_set_gt_txprecursor(sd_user_dev, arg);
			return 0;

		case SD_USER_IOCG_GT_TXPOSTCURS:
			val = sd_regs_get_gt_txpostcursor(sd_user_dev);
			pr_debug("SD_USER_IOCG_GT_TXPOSTCURS: %lu\n", val);
			return put_user(val, (unsigned long *)arg);

		case SD_USER_IOCS_GT_TXPOSTCURS:
			pr_debug("SD_USER_IOCS_GT_TXPOSTCURS: %lu\n", arg);
			sd_regs_set_gt_txpostcursor(sd_user_dev, arg);
			return 0;

		case SD_USER_IOCG_GT_RXLPMEN:
			val = sd_regs_get_gt_rxlpmen(sd_user_dev);
			pr_debug("SD_USER_IOCG_GT_RXLPMEN: %lu\n", val);
			return put_user(val, (unsigned long *)arg);

		case SD_USER_IOCS_GT_RXLPMEN:
			pr_debug("SD_USER_IOCS_GT_RXLPMEN: %lu", arg);
			sd_regs_set_gt_rxlpmen(sd_user_dev, arg);
			return 0;
	}

	return -EINVAL;
}

static int sd_user_release (struct inode *i, struct file *f)
{
	struct list_head *temp, *walk;
	struct sd_user_priv     *priv = f->private_data;
	struct sd_user_q_mesg   *qm;
	unsigned long            flags;

	wake_up_interruptible(&priv->wait);

	spin_lock_irqsave(&priv->lock, flags);

	list_for_each_safe(walk, temp, &priv->queue)
	{
		qm = container_of(walk, struct sd_user_q_mesg, list);
		pr_debug("%s: type %d size %zu", __func__, qm->mesg.type, qm->mesg.size);
		list_del_init(&qm->list);
		kfree(qm);
	}

	// unregister callback, mbox 
	spin_unlock_irqrestore(&priv->lock, flags);

	/* Remove from the list of open users */
	spin_lock_irqsave(&lock, flags);
	list_del_init(&priv->list);
	spin_unlock_irqrestore(&lock, flags);

	pr_debug("%s: close: f %p, priv %p\n", __func__, f, priv);
	f->private_data = NULL;
	kfree(priv);

	sd_user_mbox_count();
	sd_user_dbell_count();
	sd_user_swrite_count();

	return 0;
}

static struct file_operations sd_user_fops = 
{
	open:           sd_user_open,
	poll:           sd_user_poll,
	read:           sd_user_read,
	write:          sd_user_write,
	unlocked_ioctl: sd_user_ioctl,
	release:        sd_user_release,
};


static struct miscdevice mdev =
{
	MISC_DYNAMIC_MINOR,
	SD_USER_DEV_NODE,
	&sd_user_fops
};

int sd_user_init (struct srio_dev *sd)
{
	int ret;

	sd_user_dev = sd;

	if ( (ret = misc_register(&mdev)) < 0 )
	{
		pr_err("misc_register() failed\n");
		return ret;
	}

	spin_lock_init(&lock);
	INIT_LIST_HEAD(&list);

	sd_fifo_init_dir(&sd->init_fifo->rx, sd_user_recv_init, HZ);
	sd_fifo_init_dir(&sd->targ_fifo->rx, sd_user_recv_targ, HZ);
	sd_fifo_init_dir(&sd->init_fifo->tx, sd_user_tx_done, HZ);
	sd_fifo_init_dir(&sd->targ_fifo->tx, sd_user_tx_done, HZ);

	// success
	return 0;
}


void sd_user_exit (void)
{
	sd_fifo_init_dir(&sd_user_dev->init_fifo->tx, NULL, HZ);
	sd_fifo_init_dir(&sd_user_dev->init_fifo->rx, NULL, HZ);
	sd_fifo_init_dir(&sd_user_dev->targ_fifo->tx, NULL, HZ);
	sd_fifo_init_dir(&sd_user_dev->targ_fifo->rx, NULL, HZ);
	misc_deregister(&mdev);
}

