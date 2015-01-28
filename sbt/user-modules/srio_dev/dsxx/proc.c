/** \file      dsxx/proc.c
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
#include <linux/slab.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/ctype.h>

#include "srio_dev.h"
#include "sd_fifo.h"
#include "dsxx/test.h"
#include "dsxx/proc.h"


static struct proc_dir_entry *sd_dsxx_proc_root;
static struct proc_dir_entry *sd_dsxx_proc_rx;
static struct proc_dir_entry *sd_dsxx_proc_tx;


static struct proc_dir_entry *sd_dsxx_proc_rx_length;
static ssize_t sd_dsxx_proc_rx_length_read (struct file *f, char __user *u, size_t s,
                                            loff_t *o)
{
	char     b[128];
	ssize_t  l = 0;

	if ( *o )
		return 0;

	l = snprintf(b, sizeof(b), "%u\n", sd_dsxx_rx_length);

	if ( copy_to_user(u, b, l) )
		return -EFAULT;

	*o += l;
	return l;
}
static ssize_t sd_dsxx_proc_rx_length_write (struct file *f, const char __user *u,
                                             size_t s, loff_t *o)
{
	unsigned long  len;
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
	if ( kstrtoul(p, 10, &len) || (len & 3) )
		goto einval;

	printk("length set: %lu bytes\n", len);
	sd_dsxx_rx_length = len;

	*o += s;
	return s;

einval:
	return -EINVAL;
}
struct file_operations  sd_dsxx_proc_rx_length_fops =
{
	read:   sd_dsxx_proc_rx_length_read,
	write:  sd_dsxx_proc_rx_length_write,
};


static struct proc_dir_entry *sd_dsxx_proc_rx_pause;
static ssize_t sd_dsxx_proc_rx_pause_read (struct file *f, char __user *u, size_t s,
                                           loff_t *o)
{
	char     b[128];
	ssize_t  l = 0;

	if ( *o )
		return 0;

	l = snprintf(b, sizeof(b), "%u %u\n", sd_dsxx_rx_pause_on, sd_dsxx_rx_pause_off);

	if ( copy_to_user(u, b, l) )
		return -EFAULT;

	*o += l;
	return l;
}
static ssize_t sd_dsxx_proc_rx_pause_write (struct file *f, const char __user *u,
                                            size_t s, loff_t *o)
{
	unsigned long  on, off;
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
	if ( kstrtoul(p, 10, &on) )
		goto einval;

	for ( p = q; *p && !isdigit(*p); p++ ) ;
	for ( q = p; *q &&  isdigit(*q); q++ ) ;
	*q++ = '\0';
	if ( kstrtoul(p, 10, &off) )
		goto einval;

	printk("pause set: on %lu off %lu\n", on, off);
	sd_dsxx_rx_pause_on  = on;
	sd_dsxx_rx_pause_off = off;

	*o += s;
	return s;

einval:
	return -EINVAL;
}
struct file_operations  sd_dsxx_proc_rx_pause_fops =
{
	read:   sd_dsxx_proc_rx_pause_read,
	write:  sd_dsxx_proc_rx_pause_write,
};


static struct proc_dir_entry *sd_dsxx_proc_rx_reps;
static ssize_t sd_dsxx_proc_rx_reps_read (struct file *f, char __user *u, size_t s,
                                          loff_t *o)
{
	char     b[128];
	ssize_t  l = 0;

	if ( *o )
		return 0;

	l = snprintf(b, sizeof(b), "%u\n", sd_dsxx_rx_reps);

	if ( copy_to_user(u, b, l) )
		return -EFAULT;

	*o += l;
	return l;
}
static ssize_t sd_dsxx_proc_rx_reps_write (struct file *f, const char __user *u, size_t s,
                                           loff_t *o)
{
	unsigned long  len;
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
	if ( kstrtoul(p, 10, &len) )
		goto einval;

	printk("reps set: %lu bytes\n", len);
	sd_dsxx_rx_reps = len;

	*o += s;
	return s;

einval:
	return -EINVAL;
}
struct file_operations  sd_dsxx_proc_rx_reps_fops =
{
	read:   sd_dsxx_proc_rx_reps_read,
	write:  sd_dsxx_proc_rx_reps_write,
};


static ssize_t sd_dsxx_proc_stats_read (struct file *f, char __user *u, size_t s,
                                        loff_t *o, struct sd_fifo_stats *fs,
                                        struct sd_dsxx_stats *as)
{
	char *b, *e, *p;

	if ( *o )
		return 0;

	if ( (p = b = kzalloc(4096, GFP_KERNEL)) )
		e = b + 4096;
	else
		return -ENOMEM;

	p += scnprintf(p, e - p, "FIFO:\n");
	p += scnprintf(p, e - p, "  starts:    %4lu\n", fs->starts);
	p += scnprintf(p, e - p, "  chunks:    %4lu\n", fs->chunks);
	p += scnprintf(p, e - p, "  completes: %4lu\n", fs->completes);
	p += scnprintf(p, e - p, "  timeouts:  %4lu\n", fs->timeouts);
	p += scnprintf(p, e - p, "  errors:    %4lu\n", fs->errors);
	p += scnprintf(p, e - p, "  resets:    %4lu\n", fs->resets);

	p += scnprintf(p, e - p, "Test:\n");
	p += scnprintf(p, e - p, "  starts:    %4lu\n", as->starts);
	p += scnprintf(p, e - p, "  timeouts:  %4lu\n", as->timeouts);
	p += scnprintf(p, e - p, "  valids:    %4lu\n", as->valids);
	p += scnprintf(p, e - p, "  errors:    %4lu\n", as->errors);

	if ( copy_to_user(u, b, p - b) )
	{
		kfree(b);
		return -EFAULT;
	}

	kfree(b);
	*o += p - b;
	return p - b;
}


static struct proc_dir_entry *sd_dsxx_proc_rx_stats;
static ssize_t sd_dsxx_proc_rx_stats_read (struct file *f, char __user *u, size_t s,
                                           loff_t *o)
{
	return sd_dsxx_proc_stats_read(f, u, s, o, &sd_dsxx_fifo.rx.stats, &sd_dsxx_rx_stats);
}
static ssize_t sd_dsxx_proc_rx_stats_write (struct file *f, const char __user *u,
                                            size_t s, loff_t *o)
{
	memset(&sd_dsxx_fifo.rx.stats, 0, sizeof(struct sd_fifo_stats));
	memset(&sd_dsxx_rx_stats,      0, sizeof(struct sd_dsxx_stats));

	*o += s;
	return s;
}
struct file_operations  sd_dsxx_proc_rx_stats_fops =
{
	read:   sd_dsxx_proc_rx_stats_read,
	write:  sd_dsxx_proc_rx_stats_write,
};


static struct proc_dir_entry *sd_dsxx_proc_rx_trigger;
static ssize_t sd_dsxx_proc_rx_trigger_read (struct file *f, char __user *u, size_t s,
                                             loff_t *o)
{
	sd_dsxx_rx_trigger();
	return 0;
}
static ssize_t sd_dsxx_proc_rx_trigger_write (struct file *f, const char __user *u,
                                              size_t s, loff_t *o)
{
	sd_dsxx_rx_trigger();
	return s;
}
struct file_operations  sd_dsxx_proc_rx_trigger_fops =
{
	read:   sd_dsxx_proc_rx_trigger_read,
	write:  sd_dsxx_proc_rx_trigger_write,
};


static struct proc_dir_entry *sd_dsxx_proc_tx_length;
static ssize_t sd_dsxx_proc_tx_length_read (struct file *f, char __user *u, size_t s,
                                            loff_t *o)
{
	char     b[128];
	ssize_t  l = 0;

	if ( *o )
		return 0;

	l = snprintf(b, sizeof(b), "%u\n", sd_dsxx_tx_length);

	if ( copy_to_user(u, b, l) )
		return -EFAULT;

	*o += l;
	return l;
}
static ssize_t sd_dsxx_proc_tx_length_write (struct file *f, const char __user *u,
                                             size_t s, loff_t *o)
{
	unsigned long  len;
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
	if ( kstrtoul(p, 10, &len) || (len & 3) )
		goto einval;

	printk("length set: %lu bytes\n", len);
	sd_dsxx_tx_length = len;

	*o += s;
	return s;

einval:
	return -EINVAL;
}
struct file_operations  sd_dsxx_proc_tx_length_fops =
{
	read:   sd_dsxx_proc_tx_length_read,
	write:  sd_dsxx_proc_tx_length_write,
};


static struct proc_dir_entry *sd_dsxx_proc_tx_pause;
static ssize_t sd_dsxx_proc_tx_pause_read (struct file *f, char __user *u, size_t s,
                                           loff_t *o)
{
	char     b[128];
	ssize_t  l = 0;

	if ( *o )
		return 0;

	l = snprintf(b, sizeof(b), "%u %u\n", sd_dsxx_tx_pause_on, sd_dsxx_tx_pause_off);

	if ( copy_to_user(u, b, l) )
		return -EFAULT;

	*o += l;
	return l;
}
static ssize_t sd_dsxx_proc_tx_pause_write (struct file *f, const char __user *u,
                                            size_t s, loff_t *o)
{
	unsigned long  on, off;
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
	if ( kstrtoul(p, 10, &on) )
		goto einval;

	for ( p = q; *p && !isdigit(*p); p++ ) ;
	for ( q = p; *q &&  isdigit(*q); q++ ) ;
	*q++ = '\0';
	if ( kstrtoul(p, 10, &off) )
		goto einval;

	printk("pause set: on %lu off %lu\n", on, off);
	sd_dsxx_tx_pause_on  = on;
	sd_dsxx_tx_pause_off = off;

	*o += s;
	return s;

einval:
	return -EINVAL;
}
struct file_operations  sd_dsxx_proc_tx_pause_fops =
{
	read:   sd_dsxx_proc_tx_pause_read,
	write:  sd_dsxx_proc_tx_pause_write,
};


static struct proc_dir_entry *sd_dsxx_proc_tx_burst;
static ssize_t sd_dsxx_proc_tx_burst_read (struct file *f, char __user *u, size_t s, loff_t *o)
{
	char     b[128];
	ssize_t  l = 0;

	if ( *o )
		return 0;

	l = snprintf(b, sizeof(b), "%u\n", sd_dsxx_tx_burst);

	if ( copy_to_user(u, b, l) )
		return -EFAULT;

	*o += l;
	return l;
}
static ssize_t sd_dsxx_proc_tx_burst_write (struct file *f, const char __user *u,
                                            size_t s, loff_t *o)
{
	unsigned long  len;
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
	if ( kstrtoul(p, 10, &len) )
		goto einval;

	printk("burst set: %lu bytes\n", len);
	sd_dsxx_tx_burst = len;

	*o += s;
	return s;

einval:
	return -EINVAL;
}
struct file_operations  sd_dsxx_proc_tx_burst_fops =
{
	read:   sd_dsxx_proc_tx_burst_read,
	write:  sd_dsxx_proc_tx_burst_write,
};


static struct proc_dir_entry *sd_dsxx_proc_tx_stats;
static ssize_t sd_dsxx_proc_tx_stats_read (struct file *f, char __user *u, size_t s,
                                           loff_t *o)
{
	return sd_dsxx_proc_stats_read(f, u, s, o, &sd_dsxx_fifo.tx.stats, &sd_dsxx_tx_stats);
}
static ssize_t sd_dsxx_proc_tx_stats_write (struct file *f, const char __user *u,
                                            size_t s, loff_t *o)
{
	memset(&sd_dsxx_fifo.tx.stats, 0, sizeof(struct sd_fifo_stats));
	memset(&sd_dsxx_tx_stats,      0, sizeof(struct sd_dsxx_stats));

	*o += s;
	return s;
}
struct file_operations  sd_dsxx_proc_tx_stats_fops =
{
	read:   sd_dsxx_proc_tx_stats_read,
	write:  sd_dsxx_proc_tx_stats_write,
};


static struct proc_dir_entry *sd_dsxx_proc_tx_trigger;
static ssize_t sd_dsxx_proc_tx_trigger_read (struct file *f, char __user *u, size_t s,
                                             loff_t *o)
{
	printk("%s()\n", __func__);
	return 0;
}
static ssize_t sd_dsxx_proc_tx_trigger_write (struct file *f, const char __user *u,
                                              size_t s, loff_t *o)
{
	printk("%s()\n", __func__);
	return 0;
}
struct file_operations  sd_dsxx_proc_tx_trigger_fops =
{
	read:   sd_dsxx_proc_tx_trigger_read,
	write:  sd_dsxx_proc_tx_trigger_write,
};


void sd_dsxx_proc_exit (void)
{
	if ( sd_dsxx_proc_rx_length  ) proc_remove(sd_dsxx_proc_rx_length);
	if ( sd_dsxx_proc_rx_pause   ) proc_remove(sd_dsxx_proc_rx_pause);
	if ( sd_dsxx_proc_rx_reps    ) proc_remove(sd_dsxx_proc_rx_reps);
	if ( sd_dsxx_proc_rx_trigger ) proc_remove(sd_dsxx_proc_rx_trigger);
	if ( sd_dsxx_proc_rx_stats   ) proc_remove(sd_dsxx_proc_rx_stats);
	if ( sd_dsxx_proc_rx         ) proc_remove(sd_dsxx_proc_rx);

	if ( sd_dsxx_proc_tx_length  ) proc_remove(sd_dsxx_proc_tx_length);
	if ( sd_dsxx_proc_tx_pause   ) proc_remove(sd_dsxx_proc_tx_pause);
	if ( sd_dsxx_proc_tx_burst   ) proc_remove(sd_dsxx_proc_tx_burst);
	if ( sd_dsxx_proc_tx_trigger ) proc_remove(sd_dsxx_proc_tx_trigger);
	if ( sd_dsxx_proc_tx_stats   ) proc_remove(sd_dsxx_proc_tx_stats);
	if ( sd_dsxx_proc_tx         ) proc_remove(sd_dsxx_proc_tx);

	if ( sd_dsxx_proc_root       ) proc_remove(sd_dsxx_proc_root);
}

int sd_dsxx_proc_init (void)
{
	if ( !(sd_dsxx_proc_root = proc_mkdir("srio_exp", NULL)) )
		return -ENOMEM;


	if ( !(sd_dsxx_proc_rx = proc_mkdir("rx", sd_dsxx_proc_root)) )
		goto fail;

	sd_dsxx_proc_rx_length = proc_create("length", 0666, sd_dsxx_proc_rx,
	                                     &sd_dsxx_proc_rx_length_fops);
	if ( !sd_dsxx_proc_rx_length )
		goto fail;

	sd_dsxx_proc_rx_pause = proc_create("pause", 0666, sd_dsxx_proc_rx,
	                                    &sd_dsxx_proc_rx_pause_fops);
	if ( !sd_dsxx_proc_rx_pause )
		goto fail;

	sd_dsxx_proc_rx_reps = proc_create("reps", 0666, sd_dsxx_proc_rx,
	                                   &sd_dsxx_proc_rx_reps_fops);
	if ( !sd_dsxx_proc_rx_reps )
		goto fail;

	sd_dsxx_proc_rx_stats = proc_create("stats", 0666, sd_dsxx_proc_rx,
	                                    &sd_dsxx_proc_rx_stats_fops);
	if ( !sd_dsxx_proc_rx_stats )
		goto fail;

	sd_dsxx_proc_rx_trigger = proc_create("trigger", 0444, sd_dsxx_proc_rx,
	                                      &sd_dsxx_proc_rx_trigger_fops);
	if ( !sd_dsxx_proc_rx_trigger )
		goto fail;


	if ( !(sd_dsxx_proc_tx = proc_mkdir("tx", sd_dsxx_proc_root)) )
		goto fail;

	sd_dsxx_proc_tx_pause = proc_create("pause", 0666, sd_dsxx_proc_tx,
	                                    &sd_dsxx_proc_tx_pause_fops);
	if ( !sd_dsxx_proc_tx_pause )
		goto fail;

	sd_dsxx_proc_tx_burst = proc_create("burst", 0666, sd_dsxx_proc_tx,
	                                    &sd_dsxx_proc_tx_burst_fops);
	if ( !sd_dsxx_proc_tx_burst )
		goto fail;

	sd_dsxx_proc_tx_stats = proc_create("stats", 0666, sd_dsxx_proc_tx,
	                                    &sd_dsxx_proc_tx_stats_fops);
	if ( !sd_dsxx_proc_tx_stats )
		goto fail;

	sd_dsxx_proc_tx_trigger = proc_create("trigger", 0444, sd_dsxx_proc_tx,
	                                      &sd_dsxx_proc_tx_trigger_fops);
	if ( !sd_dsxx_proc_tx_trigger )
		goto fail;

	sd_dsxx_proc_tx_length = proc_create("length", 0666, sd_dsxx_proc_tx,
	                                     &sd_dsxx_proc_tx_length_fops);
	if ( !sd_dsxx_proc_tx_length )
		goto fail;

	return 0;

fail:
	sd_dsxx_proc_exit();
	return -1;
}

