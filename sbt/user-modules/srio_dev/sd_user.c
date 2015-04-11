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
static struct srio_dev   *sd_dev;

struct sd_user_priv
{
	wait_queue_head_t  wait;
	spinlock_t         lock;
	struct list_head   queue;
	uint32_t           tuser;
};

struct sd_user_mesg
{
	struct list_head  list;
	size_t            size;
	char              data[0];
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
		*p++ = isprint(ptr[i]) ? ptr[i] : '.';
	*p = '\0';

	pr_debug("%s\n", buff);
}

static void hexdump_buff (const unsigned char *buf, int len)
{
	const unsigned char *ptr = buf;

	while ( len >= 16 )
	{
		hexdump_line(ptr, buf, 16);
		ptr += 16;
		len -= 16;
	}
	if ( len )
		hexdump_line(ptr, buf, len);
}


/******* RX mbox handling *******/


void sd_user_recv_targ (struct sd_fifo *fifo, struct sd_desc *desc)
{
	pr_debug("%s: TX desc %p done\n", __func__, desc);
	hexdump_buff(desc->virt, desc->used);
	sd_fifo_rx_enqueue(fifo, desc);
}


void sd_user_recv_init (struct sd_fifo *fifo, struct sd_desc *desc)
{
	pr_debug("%s: TX desc %p done\n", __func__, desc);
	hexdump_buff(desc->virt, desc->used);
	sd_fifo_rx_enqueue(fifo, desc);
}


void sd_user_tx_done (struct sd_fifo *fifo, struct sd_desc *desc)
{
	pr_debug("%s: TX desc %p done\n", __func__, desc);
	sd_desc_free(desc);
	kfree(desc);
}


/******* VFS glue, miscdevice, init/exit *******/


static int sd_user_open (struct inode *i, struct file *f)
{
	struct sd_user_priv *priv;

	if ( !(priv = kmalloc(sizeof(*priv), GFP_KERNEL)) )
		return -ENOMEM;

	spin_lock_init(&priv->lock);
	init_waitqueue_head(&priv->wait);
	INIT_LIST_HEAD(&priv->queue);
	priv->tuser = 0x00020002;

	f->private_data = priv;
	pr_debug("%s: open: f %p, priv %p\n", __func__, f, priv);

	return 0;
}

static unsigned int sd_user_poll (struct file *f, poll_table *p)
{
	struct sd_user_priv *priv = f->private_data;
	unsigned long   flags;
	unsigned int    mask = POLLOUT | POLLWRNORM; /* Can always write */

	poll_wait(f, &priv->wait, p);

	spin_lock_irqsave(&priv->lock, flags);
	if ( !list_empty(&priv->queue) )
		mask = POLLIN | POLLRDNORM;
	spin_unlock_irqrestore(&priv->lock, flags);

	return mask;
}

static ssize_t sd_user_read (struct file *f, char __user *b, size_t s, loff_t *o)
{
	struct sd_user_priv *priv = f->private_data;
	struct sd_user_mesg *mesg;
	unsigned long   flags;
	size_t          size;

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
	mesg = container_of(priv->queue.next, struct sd_user_mesg, list);
	size = mesg->size;
	list_del_init(&mesg->list);
	spin_unlock_irqrestore(&priv->lock, flags);
	pr_debug("%s: %zu: %s", __func__, mesg->size, mesg->data);

	if ( size > s || copy_to_user(b, mesg->data, size) )
	{
		pr_err("Message dropped\n");
		kfree(mesg);
		return -ENOSPC;
	}
	kfree(mesg);

	// fix up position and return number of bytes copied
	*o +=  size;
	return size;
}


static ssize_t sd_user_write (struct file *f, const char __user *b, size_t s, loff_t *o)
{
	struct sd_user_priv *priv = f->private_data;
	struct sd_desc *desc;
	uint8_t        *dest;
	u32            *hdr;

	if ( !(desc = kzalloc(sizeof(*desc), GFP_KERNEL)) )
		return -ENOMEM;

	if ( sd_desc_alloc(desc, BUFF_SIZE, GFP_KERNEL) )
	{
		kfree(desc);
		return -ENOMEM;
	}

	pr_debug("%s(f, b %p, s %zu, o %lld)\n", __func__, b, s, *o);

	// HELLO header (see PG007 Fig 3-1)
	hdr = (u32 *)desc->virt;
	*hdr++ = priv->tuser;
	*hdr++ = 0xa0000600; // lsw
	*hdr++ = 0x02602ff9; // msw
	desc->used = (hdr - (u32 *)desc->virt) * sizeof(u32);

	if ( (desc->used + s) > BUFF_SIZE )
		return -EINVAL;

	dest = desc->virt + desc->used;
	if ( copy_from_user(dest, b, s) )
		return -EFAULT;

	desc->used += s;
	if ( desc->used & 0x03 )
		desc->used = (desc->used + 1) & ~0x03;

	pr_debug("%s desc %p used %zu\n", __func__, desc, desc->used);

	// enqueue in initiator fifo, desc will be cleaned up in sd_user_tx_done()
	sd_fifo_tx_enqueue(sd_dev->init_fifo, desc);

	return s;
}


static long sd_user_ioctl (struct file *f, unsigned int cmd, unsigned long arg)
{
//	unsigned long  flags;
//	uint16_t       reg;
//	uint16_t      *dst;
//	int            val;

//	if ( _IOC_TYPE(cmd) != MAGIC_pt )
//		return -EINVAL;

	switch ( cmd )
	{
#if 0
		case sd_user_IOCG_EXTTRIG:
			val = readw(base16 + 0x02);
			val &= 0x0200;
			return put_user(val, (int *)arg);

		case sd_user_IOCS_EXTTRIG:
			get_user(val, (int *)arg);
			spin_lock_irqsave(&lock, flags);
			adc_stop();
			reg = readw(base16 + 0x02);
			reg &= ~0x0200;
			if ( val )
				reg |= 0x0200;
			writew(reg, base16 + 0x02);
			spin_unlock_irqrestore(&lock, flags);
			return 0;

		case sd_user_IOCG_ADC_SAMPLE:
			spin_lock_irqsave(&lock, flags);
			val = ((insert + RING_SIZE) - extract) & RING_MASK;
			if ( val > ((numchan + 1) * 2) )
				val = ((numchan + 1) * 2);

			dst = (uint16_t *)arg;
			while ( val-- )
			{
				if ( copy_to_user(dst, &ring[extract], sizeof(uint16_t)) )
					break;

				extract++;
				extract &= RING_MASK;
				dst++;
			}

			spin_unlock_irqrestore(&lock, flags);
			return val;
#endif
	}

	return -EINVAL;
}

static int sd_user_release (struct inode *i, struct file *f)
{
	struct list_head *temp, *walk;
	struct sd_user_priv   *priv = f->private_data;
	struct sd_user_mesg   *mesg;
	unsigned long     flags;

	wake_up_interruptible(&priv->wait);

	spin_lock_irqsave(&priv->lock, flags);
	list_for_each_safe(walk, temp, &priv->queue)
	{
		mesg = container_of(walk, struct sd_user_mesg, list);
		pr_debug("%s: %zu: %s", __func__, mesg->size, mesg->data);
		list_del_init(&mesg->list);
		kfree(mesg);
	}

	// unregister callback, mbox 
	spin_unlock_irqrestore(&priv->lock, flags);

	pr_debug("%s: close: f %p, priv %p\n", __func__, f, priv);
	f->private_data = NULL;
	kfree(priv);

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
	"srio",
	&sd_user_fops
};

int sd_user_init (struct srio_dev *sd)
{
	int ret;

	sd_dev = sd;

	if ( (ret = misc_register(&mdev)) < 0 )
	{
		pr_err("misc_register() failed\n");
		return ret;
	}

	sd_fifo_init_dir(&sd->init_fifo->rx, sd_user_recv_init, HZ);
	sd_fifo_init_dir(&sd->targ_fifo->rx, sd_user_recv_targ, HZ);
	sd_fifo_init_dir(&sd->init_fifo->tx, sd_user_tx_done, HZ);
	sd_fifo_init_dir(&sd->targ_fifo->tx, sd_user_tx_done, HZ);

	// success
	return 0;
}


void sd_user_exit (void)
{
	sd_fifo_init_dir(&sd_dev->init_fifo->tx, NULL, HZ);
	sd_fifo_init_dir(&sd_dev->init_fifo->rx, NULL, HZ);
	sd_fifo_init_dir(&sd_dev->targ_fifo->tx, NULL, HZ);
	sd_fifo_init_dir(&sd_dev->targ_fifo->rx, NULL, HZ);
	misc_deregister(&mdev);
}

