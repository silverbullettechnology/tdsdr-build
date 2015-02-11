/** \file      loop/gpio.c
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
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/ctype.h>

#include "srio_dev.h"
#include "loop/test.h"
#include "loop/proc.h"


static struct proc_dir_entry *sd_loop_proc_root;
static struct proc_dir_entry *sd_loop_proc_dest_init;
static struct proc_dir_entry *sd_loop_proc_dest_targ;
static struct proc_dir_entry *sd_loop_proc_dest_fifo;


static struct proc_dir_entry *sd_loop_proc_dest_init;
static ssize_t sd_loop_proc_dest_init_read (struct file *f, char __user *u, size_t s,
                                            loff_t *o)
{
	char     b[128];
	ssize_t  l = 0;

	if ( *o )
		return 0;

	l = snprintf(b, sizeof(b), "%08x\n", sd_loop_get_dest_init_reg());

	if ( copy_to_user(u, b, l) )
		return -EFAULT;

	*o += l;
	return l;
}
static ssize_t sd_loop_proc_dest_init_write (struct file *f, const char __user *u,
                                             size_t s, loff_t *o)
{
	unsigned long  reg;
	char           b[128];
	char          *p, *q;

	if ( *o )
		return 0;

	if ( s >= sizeof(b) )
		s = sizeof(b) - 1;
	b[s] = '\0';

	if ( copy_from_user(b, u, s) )
		return -EFAULT;

	for ( p = b; *p && !isdigit(*p); p++ ) ;
	for ( q = p; *q &&  isdigit(*q); q++ ) ;
	*q++ = '\0';
	if ( kstrtoul(p, 16, &reg) )
		goto einval;

	printk("dest init: %08lx\n", reg);
	sd_loop_set_dest_init_reg(reg);

	*o += s;
	return s;

einval:
	return -EINVAL;
}
struct file_operations  sd_loop_proc_dest_init_fops =
{
	read:   sd_loop_proc_dest_init_read,
	write:  sd_loop_proc_dest_init_write,
};


static struct proc_dir_entry *sd_loop_proc_dest_targ;
static ssize_t sd_loop_proc_dest_targ_read (struct file *f, char __user *u, size_t s,
                                           loff_t *o)
{
	char     b[128];
	ssize_t  l = 0;

	if ( *o )
		return 0;

	l = snprintf(b, sizeof(b), "%08x\n", sd_loop_get_dest_targ_reg());

	if ( copy_to_user(u, b, l) )
		return -EFAULT;

	*o += l;
	return l;
}
static ssize_t sd_loop_proc_dest_targ_write (struct file *f, const char __user *u,
                                            size_t s, loff_t *o)
{
	unsigned long  reg;
	char           b[128];
	char          *p, *q;

	if ( *o )
		return 0;

	if ( s >= sizeof(b) )
		s = sizeof(b) - 1;
	b[s] = '\0';

	if ( copy_from_user(b, u, s) )
		return -EFAULT;

	for ( p = b; *p && !isdigit(*p); p++ ) ;
	for ( q = p; *q &&  isdigit(*q); q++ ) ;
	*q++ = '\0';
	if ( kstrtoul(p, 16, &reg) )
		goto einval;

	printk("dest targ: %08lx\n", reg);
	sd_loop_set_dest_targ_reg(reg);

	*o += s;
	return s;

einval:
	return -EINVAL;
}
struct file_operations  sd_loop_proc_dest_targ_fops =
{
	read:   sd_loop_proc_dest_targ_read,
	write:  sd_loop_proc_dest_targ_write,
};


static struct proc_dir_entry *sd_loop_proc_dest_fifo;
static ssize_t sd_loop_proc_dest_fifo_read (struct file *f, char __user *u, size_t s,
                                          loff_t *o)
{
	char     b[128];
	ssize_t  l = 0;

	if ( *o )
		return 0;

	l = snprintf(b, sizeof(b), "%d\n", sd_loop_tx_dest);

	if ( copy_to_user(u, b, l) )
		return -EFAULT;

	*o += l;
	return l;
}
static ssize_t sd_loop_proc_dest_fifo_write (struct file *f, const char __user *u, size_t s,
                                           loff_t *o)
{
	unsigned long  dest;
	char           b[128];
	char          *p, *q;

	if ( *o )
		return 0;

	if ( s >= sizeof(b) )
		s = sizeof(b) - 1;
	b[s] = '\0';

	if ( copy_from_user(b, u, s) )
		return -EFAULT;

	for ( p = b; *p && !isdigit(*p); p++ ) ;
	for ( q = p; *q &&  isdigit(*q); q++ ) ;
	*q++ = '\0';
	if ( kstrtoul(p, 10, &dest) )
		goto einval;

	printk("dest set: %lu bytes\n", dest);
	sd_loop_tx_dest = dest;

	*o += s;
	return s;

einval:
	return -EINVAL;
}
struct file_operations  sd_loop_proc_dest_fifo_fops =
{
	read:   sd_loop_proc_dest_fifo_read,
	write:  sd_loop_proc_dest_fifo_write,
};





void sd_loop_proc_exit (void)
{
	if ( sd_loop_proc_dest_init  ) proc_remove(sd_loop_proc_dest_init);
	if ( sd_loop_proc_dest_targ   ) proc_remove(sd_loop_proc_dest_targ);
	if ( sd_loop_proc_dest_fifo    ) proc_remove(sd_loop_proc_dest_fifo);

	if ( sd_loop_proc_root       ) proc_remove(sd_loop_proc_root);
}

int sd_loop_proc_init (void)
{
	if ( !(sd_loop_proc_root = proc_mkdir("srio_dev", NULL)) )
		return -ENOMEM;

	sd_loop_proc_dest_init = proc_create("init", 0666, sd_loop_proc_root,
	                                     &sd_loop_proc_dest_init_fops);
	if ( !sd_loop_proc_dest_init )
		goto fail;

	sd_loop_proc_dest_targ = proc_create("targ", 0666, sd_loop_proc_root,
	                                     &sd_loop_proc_dest_targ_fops);
	if ( !sd_loop_proc_dest_targ )
		goto fail;


	sd_loop_proc_dest_fifo = proc_create("fifo", 0666, sd_loop_proc_root,
	                                     &sd_loop_proc_dest_fifo_fops);
	if ( !sd_loop_proc_dest_fifo )
		goto fail;

	return 0;

fail:
	sd_loop_proc_exit();
	return -1;
}

