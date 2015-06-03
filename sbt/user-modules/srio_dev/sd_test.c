/** \file      sd_test.c
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
#include "sd_fifo.h"
#include "sd_regs.h"
#include "sd_test.h"


static struct proc_dir_entry *sd_test_proc;
static struct srio_dev       *sd_test_dev;
static uint32_t               sd_test_addr = 0x00020002;
static uint32_t               sd_test_pattern = 0;


static uint32_t sd_test_pattern_type6[] =
{
	0x00020002, 0xA0000600, 0x02602FF9, 0x78563412, 0xAA55AA55
};

static struct pattern
{
	const uint32_t *base;
	const size_t    size;
}
sd_test_pattern_list[] =
{
	{ sd_test_pattern_type6, sizeof(sd_test_pattern_type6) },
};


static struct proc_dir_entry *sd_test_addr_proc;
static ssize_t sd_test_addr_read (struct file *f, char __user *u, size_t s, loff_t *o)
{
	char     b[128];
	ssize_t  l = 0;

	if ( *o )
		return 0;

	l = snprintf(b, sizeof(b), "%08x\n", sd_test_addr);

	if ( copy_to_user(u, b, l) )
		return -EFAULT;

	*o += l;
	return l;
}
static ssize_t sd_test_addr_write (struct file *f, const char __user *u, size_t s,
                                   loff_t *o)
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

	for ( p = b; *p && !isxdigit(*p); p++ ) ;
	for ( q = p; *q &&  isxdigit(*q); q++ ) ;
	*q++ = '\0';
	if ( kstrtoul(p, 16, &reg) )
		goto einval;

	printk("address set: '%s' -> '%s' -> %08lx\n", b, p, reg);
	sd_test_addr = reg;

	*o += s;
	return s;

einval:
	return -EINVAL;
}
struct file_operations  sd_test_addr_fops =
{
	read:   sd_test_addr_read,
	write:  sd_test_addr_write,
};


static struct proc_dir_entry *sd_test_pattern_proc;
static ssize_t sd_test_pattern_read (struct file *f, char __user *u, size_t s, loff_t *o)
{
	char     b[128];
	ssize_t  l = 0;

	if ( *o )
		return 0;

	l = snprintf(b, sizeof(b), "%u\n", sd_test_pattern);

	if ( copy_to_user(u, b, l) )
		return -EFAULT;

	*o += l;
	return l;
}
static ssize_t sd_test_pattern_write (struct file *f, const char __user *u, size_t s,
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
	if ( kstrtoul(p, 10, &dest) || dest >= ARRAY_SIZE(sd_test_pattern_list) )
		goto einval;

	printk("pattern set: '%s' -> '%s' -> %lu\n", b, p, dest);
	sd_test_pattern = dest;

	*o += s;
	return s;

einval:
	return -EINVAL;
}
struct file_operations  sd_test_pattern_fops =
{
	read:   sd_test_pattern_read,
	write:  sd_test_pattern_write,
};


static struct proc_dir_entry *sd_test_trigger_proc;
static ssize_t sd_test_trigger_write (struct file *f, const char __user *u, size_t s,
                                      loff_t *o)
{
	struct sd_desc *desc;

	if ( *o )
		return 0;

	if ( sd_test_pattern >= ARRAY_SIZE(sd_test_pattern_list) )
	{
		printk("sd_test_pattern %u invalid\n", sd_test_pattern);
		return -EINVAL;
	}
	if ( sd_test_pattern_list[sd_test_pattern].size > SD_FIFO_SIZE )
	{
		printk("sd_test_pattern %u size %zu over limit %u\n", sd_test_pattern,
		       sd_test_pattern_list[sd_test_pattern].size, SD_FIFO_SIZE);
		return -EINVAL;
	}

	if ( !(desc = sd_desc_alloc(sd_test_dev, GFP_KERNEL|GFP_DMA)) )
	{
		kfree(desc);
		return -ENOMEM;
	}

	memcpy(desc->virt, sd_test_pattern_list[sd_test_pattern].base,
	       sd_test_pattern_list[sd_test_pattern].size);
	desc->used = sd_test_pattern_list[sd_test_pattern].size;

	sd_fifo_tx_enqueue(sd_test_dev->init_fifo, desc);

	*o += s;
	return s;
}
struct file_operations  sd_test_trigger_fops =
{
	write:  sd_test_trigger_write,
};


static struct proc_dir_entry *sd_test_stat_proc;
static ssize_t sd_test_stat_read (struct file *f, char __user *u, size_t s, loff_t *o)
{
	static char  b[4096];
	ssize_t      l = 0;

	if ( *o )
		return 0;

	l = sd_regs_print_srio(sd_test_dev, b, sizeof(b));

	if ( copy_to_user(u, b, l) )
		return -EFAULT;

	*o += l;
	return l;
}
struct file_operations  sd_test_stat_fops =
{
	read:   sd_test_stat_read,
};


static struct proc_dir_entry *sd_test_gt_loopback_proc;
static ssize_t sd_test_gt_loopback_read (struct file *f, char __user *u, size_t s,
                                         loff_t *o)
{
	char     b[128];
	ssize_t  l = 0;

	if ( *o )
		return 0;

	l = snprintf(b, sizeof(b), "%u\n", sd_regs_get_gt_loopback(sd_test_dev));

	if ( copy_to_user(u, b, l) )
		return -EFAULT;

	*o += l;
	return l;
}
static ssize_t sd_test_gt_loopback_write (struct file *f, const char __user *u, size_t s,
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

	sd_regs_set_gt_loopback(sd_test_dev, val);
	printk("gt_loopback set: '%s' -> '%s' -> %lu\n", b, p, val);

	*o += s;
	return s;

einval:
	return -EINVAL;
}
struct file_operations  sd_test_gt_loopback_fops =
{
	read:   sd_test_gt_loopback_read,
	write:  sd_test_gt_loopback_write,
};


static struct proc_dir_entry *sd_test_gt_diffctrl_proc;
static ssize_t sd_test_gt_diffctrl_read (struct file *f, char __user *u, size_t s,
                                         loff_t *o)
{
	char     b[128];
	ssize_t  l = 0;

	if ( *o )
		return 0;

	l = snprintf(b, sizeof(b), "%u\n", sd_regs_get_gt_diffctrl(sd_test_dev));

	if ( copy_to_user(u, b, l) )
		return -EFAULT;

	*o += l;
	return l;
}
static ssize_t sd_test_gt_diffctrl_write (struct file *f, const char __user *u, size_t s,
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

	sd_regs_set_gt_diffctrl(sd_test_dev, val);
	printk("gt_diffctrl set: '%s' -> '%s' -> %lu\n", b, p, val);

	*o += s;
	return s;

einval:
	return -EINVAL;
}
struct file_operations  sd_test_gt_diffctrl_fops =
{
	read:   sd_test_gt_diffctrl_read,
	write:  sd_test_gt_diffctrl_write,
};


static struct proc_dir_entry *sd_test_gt_txprecursor_proc;
static ssize_t sd_test_gt_txprecursor_read (struct file *f, char __user *u, size_t s,
                                         loff_t *o)
{
	char     b[128];
	ssize_t  l = 0;

	if ( *o )
		return 0;

	l = snprintf(b, sizeof(b), "%u\n", sd_regs_get_gt_txprecursor(sd_test_dev));

	if ( copy_to_user(u, b, l) )
		return -EFAULT;

	*o += l;
	return l;
}
static ssize_t sd_test_gt_txprecursor_write (struct file *f, const char __user *u, size_t s,
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

	sd_regs_set_gt_txprecursor(sd_test_dev, val);
	printk("gt_txprecursor set: '%s' -> '%s' -> %lu\n", b, p, val);

	*o += s;
	return s;

einval:
	return -EINVAL;
}
struct file_operations  sd_test_gt_txprecursor_fops =
{
	read:   sd_test_gt_txprecursor_read,
	write:  sd_test_gt_txprecursor_write,
};


static struct proc_dir_entry *sd_test_gt_txpostcursor_proc;
static ssize_t sd_test_gt_txpostcursor_read (struct file *f, char __user *u, size_t s,
                                         loff_t *o)
{
	char     b[128];
	ssize_t  l = 0;

	if ( *o )
		return 0;

	l = snprintf(b, sizeof(b), "%u\n", sd_regs_get_gt_txpostcursor(sd_test_dev));

	if ( copy_to_user(u, b, l) )
		return -EFAULT;

	*o += l;
	return l;
}
static ssize_t sd_test_gt_txpostcursor_write (struct file *f, const char __user *u, size_t s,
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

	sd_regs_set_gt_txpostcursor(sd_test_dev, val);
	printk("gt_txpostcursor set: '%s' -> '%s' -> %lu\n", b, p, val);

	*o += s;
	return s;

einval:
	return -EINVAL;
}
struct file_operations  sd_test_gt_txpostcursor_fops =
{
	read:   sd_test_gt_txpostcursor_read,
	write:  sd_test_gt_txpostcursor_write,
};


static struct proc_dir_entry *sd_test_reset_proc;
static ssize_t sd_test_reset_read (struct file *f, char __user *u, size_t s, loff_t *o)
                                 
{
	if ( *o )
		return 0;

	printk("SRIO reset\n");
	sd_regs_srio_reset(sd_test_dev);
	sd_fifo_reset(sd_test_dev->init_fifo, SD_FR_ALL);
	sd_fifo_reset(sd_test_dev->targ_fifo, SD_FR_ALL);

	return 0;
}
struct file_operations  sd_test_reset_fops =
{
	read:  sd_test_reset_read,
};


static struct proc_dir_entry *sd_test_rxdfelpmreset_proc;
static ssize_t sd_test_rxdfelpmreset_read (struct file *f, char __user *u, size_t s,
                                           loff_t *o)
{
	if ( *o )
		return 0;

	printk("SRIO rxdfelpmreset\n");
	sd_regs_gt_srio_rxdfelpmreset(sd_test_dev);

	return 0;
}
struct file_operations  sd_test_rxdfelpmreset_fops =
{
	read:  sd_test_rxdfelpmreset_read,
};


static struct proc_dir_entry *sd_test_phy_link_reset_proc;
static ssize_t sd_test_phy_link_reset_read (struct file *f, char __user *u, size_t s,
                                            loff_t *o)
{
	if ( *o )
		return 0;

	printk("SRIO phy_link_reset\n");
	sd_regs_gt_phy_link_reset(sd_test_dev);

	return 0;
}
struct file_operations  sd_test_phy_link_reset_fops =
{
	read:  sd_test_phy_link_reset_read,
};


static struct proc_dir_entry *sd_test_force_reinit_proc;
static ssize_t sd_test_force_reinit_read (struct file *f, char __user *u, size_t s,
                                          loff_t *o)
{
	if ( *o )
		return 0;

	printk("SRIO force_reinit\n");
	sd_regs_gt_force_reinit(sd_test_dev);

	return 0;
}
struct file_operations  sd_test_force_reinit_fops =
{
	read:  sd_test_force_reinit_read,
};


static struct proc_dir_entry *sd_test_phy_mce_proc;
static ssize_t sd_test_phy_mce_read (struct file *f, char __user *u, size_t s,
                                     loff_t *o)
{
	if ( *o )
		return 0;

	printk("SRIO phy_mce\n");
	sd_regs_gt_phy_mce(sd_test_dev);

	*o += s;
	return s;
}
struct file_operations  sd_test_phy_mce_fops =
{
	read:  sd_test_phy_mce_read,
};


void sd_test_exit (void)
{
	if ( sd_test_addr_proc          ) proc_remove(sd_test_addr_proc);
	if ( sd_test_pattern_proc       ) proc_remove(sd_test_pattern_proc);
	if ( sd_test_trigger_proc       ) proc_remove(sd_test_trigger_proc);
	if ( sd_test_stat_proc          ) proc_remove(sd_test_stat_proc);
	if ( sd_test_gt_loopback_proc   ) proc_remove(sd_test_gt_loopback_proc);
	if ( sd_test_gt_diffctrl_proc   ) proc_remove(sd_test_gt_diffctrl_proc);
	if ( sd_test_gt_txprecursor_proc  ) proc_remove(sd_test_gt_txprecursor_proc);
	if ( sd_test_gt_txpostcursor_proc ) proc_remove(sd_test_gt_txpostcursor_proc);
	if ( sd_test_reset_proc         ) proc_remove(sd_test_reset_proc);
	if ( sd_test_rxdfelpmreset_proc ) proc_remove(sd_test_rxdfelpmreset_proc);
	if ( sd_test_phy_link_reset_proc) proc_remove(sd_test_phy_link_reset_proc);
	if ( sd_test_force_reinit_proc  ) proc_remove(sd_test_force_reinit_proc);
	if ( sd_test_phy_mce_proc       ) proc_remove(sd_test_phy_mce_proc);

	if ( sd_test_proc ) proc_remove(sd_test_proc);
}

int sd_test_init (struct srio_dev *sd)
{
	sd_test_dev = sd;

	if ( !(sd_test_proc = proc_mkdir("srio", NULL)) )
		return -ENOMEM;

	sd_test_addr_proc = proc_create("address", 0666, sd_test_proc,
	                                &sd_test_addr_fops);
	if ( !sd_test_addr_proc )
		goto fail;


	sd_test_pattern_proc = proc_create("pattern", 0666, sd_test_proc,
	                                   &sd_test_pattern_fops);
	if ( !sd_test_pattern_proc )
		goto fail;

	sd_test_trigger_proc = proc_create("trigger", 0444, sd_test_proc,
	                                   &sd_test_trigger_fops);
	if ( !sd_test_trigger_proc )
		goto fail;

	sd_test_stat_proc = proc_create("status", 0444, sd_test_proc,
	                                &sd_test_stat_fops);
	if ( !sd_test_stat_proc )
		goto fail;

	sd_test_gt_loopback_proc = proc_create("gt_loopback", 0666, sd_test_proc,
	                                       &sd_test_gt_loopback_fops);
	if ( !sd_test_gt_loopback_proc )
		goto fail;

	sd_test_gt_diffctrl_proc = proc_create("gt_diffctrl", 0666, sd_test_proc,
	                                       &sd_test_gt_diffctrl_fops);
	if ( !sd_test_gt_diffctrl_proc )
		goto fail;

	sd_test_gt_txprecursor_proc = proc_create("gt_txprecursor", 0666, sd_test_proc,
	                                       &sd_test_gt_txprecursor_fops);
	if ( !sd_test_gt_txprecursor_proc )
		goto fail;

	sd_test_gt_txpostcursor_proc = proc_create("gt_txpostcursor", 0666, sd_test_proc,
	                                       &sd_test_gt_txpostcursor_fops);
	if ( !sd_test_gt_txpostcursor_proc )
		goto fail;

	sd_test_reset_proc = proc_create("reset", 0666, sd_test_proc,
	                                 &sd_test_reset_fops);
	if ( !sd_test_reset_proc )
		goto fail;

	sd_test_rxdfelpmreset_proc = proc_create("rxdfelpmreset", 0444, sd_test_proc,
	                                         &sd_test_rxdfelpmreset_fops);
	if ( !sd_test_rxdfelpmreset_proc )
		goto fail;

	sd_test_phy_link_reset_proc = proc_create("phy_link_reset", 0444, sd_test_proc,
	                                          &sd_test_phy_link_reset_fops);
	if ( !sd_test_phy_link_reset_proc )
		goto fail;

	sd_test_force_reinit_proc = proc_create("force_reinit", 0444, sd_test_proc,
	                                        &sd_test_force_reinit_fops);
	if ( !sd_test_force_reinit_proc )
		goto fail;

	sd_test_phy_mce_proc = proc_create("phy_mce", 0444, sd_test_proc,
	                                   &sd_test_phy_mce_fops);
	if ( !sd_test_phy_mce_proc )
		goto fail;

	return 0;

fail:
	sd_test_exit();
	return -1;
}

