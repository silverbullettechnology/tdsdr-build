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
#include <linux/gpio.h>
#include <linux/ctype.h>

#include "srio_dev.h"
#include "loop/test.h"
#include "loop/gpio.h"
#include "loop/proc.h"


static struct proc_dir_entry *sd_loop_proc_root;


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

	printk("dest set: %lu\n", dest);
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


static struct proc_dir_entry *sd_loop_proc_srio_stat;
static ssize_t sd_loop_proc_srio_stat_read (struct file *f, char __user *u, size_t s,
                                            loff_t *o)
{
	static char  b[4096];
	ssize_t      l = 0;

	if ( *o )
		return 0;

	l = sd_loop_gpio_print_srio(b, sizeof(b));

	if ( copy_to_user(u, b, l) )
		return -EFAULT;

	*o += l;
	return l;
}
struct file_operations  sd_loop_proc_srio_stat_fops =
{
	read:   sd_loop_proc_srio_stat_read,
};


static struct proc_dir_entry *sd_loop_proc_prbs_stat;
static ssize_t sd_loop_proc_prbs_stat_read (struct file *f, char __user *u, size_t s,
                                            loff_t *o)
{
	static char  b[4096];
	ssize_t      l = 0;

	if ( *o )
		return 0;

	l = sd_loop_gpio_print_gt_prbs(b, sizeof(b));

	if ( copy_to_user(u, b, l) )
		return -EFAULT;

	*o += l;
	return l;
}
struct file_operations  sd_loop_proc_prbs_stat_fops =
{
	read:   sd_loop_proc_prbs_stat_read,
};


static struct proc_dir_entry *sd_loop_proc_force_err;
static ssize_t sd_loop_proc_force_err_read (struct file *f, char __user *u, size_t s,
                                            loff_t *o)
{
	char     b[128];
	ssize_t  l = 0;

	if ( *o )
		return 0;

	l = snprintf(b, sizeof(b), "%d\n", gpio_get_value(GT_TXPRBSFORCEERR_PIN));

	if ( copy_to_user(u, b, l) )
		return -EFAULT;

	*o += l;
	return l;
}
static ssize_t sd_loop_proc_force_err_write (struct file *f, const char __user *u, size_t s,
                                             loff_t *o)
{
	char  b[128];

	if ( *o )
		return 0;

	if ( s >= sizeof(b) )
		s = sizeof(b) - 1;
	b[s] = '\0';

	if ( copy_from_user(b, u, s) )
		return -EFAULT;
	
	switch ( b[0] )
	{
		case '0': sd_loop_gpio_prbs_force_err(0); break;
		case '1': sd_loop_gpio_prbs_force_err(1); break;
	}
	printk("force_err: %d\n", gpio_get_value(GT_TXPRBSFORCEERR_PIN));

	*o += s;
	return s;
}
struct file_operations  sd_loop_proc_force_err_fops =
{
	read:   sd_loop_proc_force_err_read,
	write:  sd_loop_proc_force_err_write,
};


static unsigned long gt_prbs;
static struct proc_dir_entry *sd_loop_proc_gt_prbs;
static ssize_t sd_loop_proc_gt_prbs_read (struct file *f, char __user *u, size_t s,
                                          loff_t *o)
{
	char     b[128];
	ssize_t  l = 0;

	if ( *o )
		return 0;

	l = snprintf(b, sizeof(b), "%lu\n", gt_prbs);

	if ( copy_to_user(u, b, l) )
		return -EFAULT;

	*o += l;
	return l;
}
static ssize_t sd_loop_proc_gt_prbs_write (struct file *f, const char __user *u, size_t s,
                                           loff_t *o)
{
	unsigned long  val;
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
	if ( kstrtoul(p, 10, &val) )
		goto einval;

	sd_loop_gpio_set_gt_prbs(gt_prbs = val);
	printk("gt_prbs set: %lu\n", val);

	*o += s;
	return s;

einval:
	return -EINVAL;
}
struct file_operations  sd_loop_proc_gt_prbs_fops =
{
	read:   sd_loop_proc_gt_prbs_read,
	write:  sd_loop_proc_gt_prbs_write,
};


static unsigned long gt_loopback;
static struct proc_dir_entry *sd_loop_proc_gt_loopback;
static ssize_t sd_loop_proc_gt_loopback_read (struct file *f, char __user *u, size_t s,
                                              loff_t *o)
{
	char     b[128];
	ssize_t  l = 0;

	if ( *o )
		return 0;

	l = snprintf(b, sizeof(b), "%lu\n", gt_loopback);

	if ( copy_to_user(u, b, l) )
		return -EFAULT;

	*o += l;
	return l;
}
static ssize_t sd_loop_proc_gt_loopback_write (struct file *f, const char __user *u, size_t s,
                                               loff_t *o)
{
	unsigned long  val;
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
	if ( kstrtoul(p, 10, &val) )
		goto einval;

	sd_loop_gpio_set_gt_loopback(gt_loopback = val);
	printk("gt_loopback set: %lu\n", val);

	*o += s;
	return s;

einval:
	return -EINVAL;
}
struct file_operations  sd_loop_proc_gt_loopback_fops =
{
	read:   sd_loop_proc_gt_loopback_read,
	write:  sd_loop_proc_gt_loopback_write,
};


static struct proc_dir_entry *sd_loop_proc_pcie_srio_sel;
static ssize_t sd_loop_proc_pcie_srio_sel_read (struct file *f, char __user *u, size_t s,
                                                loff_t *o)
{
	char     b[128];
	ssize_t  l = 0;

	if ( *o )
		return 0;

	l = snprintf(b, sizeof(b), "%d\n", gpio_get_value(SRIO_PCIE_SEL_PIN));

	if ( copy_to_user(u, b, l) )
		return -EFAULT;

	*o += l;
	return l;
}
static ssize_t sd_loop_proc_pcie_srio_sel_write (struct file *f, const char __user *u, size_t s,
                                                 loff_t *o)
{
	char  b[128];

	if ( *o )
		return 0;

	if ( s >= sizeof(b) )
		s = sizeof(b) - 1;
	b[s] = '\0';

	if ( copy_from_user(b, u, s) )
		return -EFAULT;

	switch ( b[0] )
	{
		case '0': gpio_set_value(SRIO_PCIE_SEL_PIN, 0); break;
		case '1': gpio_set_value(SRIO_PCIE_SEL_PIN, 1); break;
	}
	printk("force_err: %d\n", gpio_get_value(SRIO_PCIE_SEL_PIN));

	*o += s;
	return s;
}
struct file_operations  sd_loop_proc_pcie_srio_sel_fops =
{
	read:   sd_loop_proc_pcie_srio_sel_read,
	write:  sd_loop_proc_pcie_srio_sel_write,
};




void sd_loop_proc_exit (void)
{
	if ( sd_loop_proc_dest_init ) proc_remove(sd_loop_proc_dest_init);
	if ( sd_loop_proc_dest_targ ) proc_remove(sd_loop_proc_dest_targ);
	if ( sd_loop_proc_dest_fifo ) proc_remove(sd_loop_proc_dest_fifo);
	if ( sd_loop_proc_prbs_stat ) proc_remove(sd_loop_proc_prbs_stat);
	if ( sd_loop_proc_srio_stat ) proc_remove(sd_loop_proc_srio_stat);

	if ( sd_loop_proc_gt_loopback   ) proc_remove(sd_loop_proc_gt_loopback);
	if ( sd_loop_proc_gt_prbs       ) proc_remove(sd_loop_proc_gt_prbs);
	if ( sd_loop_proc_force_err     ) proc_remove(sd_loop_proc_force_err);
	if ( sd_loop_proc_pcie_srio_sel ) proc_remove(sd_loop_proc_pcie_srio_sel);

	if ( sd_loop_proc_root ) proc_remove(sd_loop_proc_root);
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

	sd_loop_proc_srio_stat = proc_create("srio_stat", 0444, sd_loop_proc_root,
	                                     &sd_loop_proc_srio_stat_fops);
	if ( !sd_loop_proc_srio_stat )
		goto fail;

	sd_loop_proc_prbs_stat = proc_create("prbs_stat", 0444, sd_loop_proc_root,
	                                     &sd_loop_proc_prbs_stat_fops);
	if ( !sd_loop_proc_prbs_stat )
		goto fail;

	sd_loop_proc_force_err = proc_create("force_err", 0666, sd_loop_proc_root,
	                                     &sd_loop_proc_force_err_fops);
	if ( !sd_loop_proc_force_err )
		goto fail;

	sd_loop_proc_gt_loopback = proc_create("gt_loopback", 0666, sd_loop_proc_root,
	                                       &sd_loop_proc_gt_loopback_fops);
	if ( !sd_loop_proc_gt_loopback )
		goto fail;

	sd_loop_proc_gt_prbs = proc_create("gt_prbs", 0666, sd_loop_proc_root,
	                                   &sd_loop_proc_gt_prbs_fops);
	if ( !sd_loop_proc_gt_prbs )
		goto fail;

	sd_loop_proc_pcie_srio_sel = proc_create("pcie_srio_sel", 0666, sd_loop_proc_root,
	                                         &sd_loop_proc_pcie_srio_sel_fops);
	if ( !sd_loop_proc_pcie_srio_sel )
		goto fail;


	return 0;

fail:
	sd_loop_proc_exit();
	return -1;
}

