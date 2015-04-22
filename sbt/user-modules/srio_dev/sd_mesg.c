/** \file      sd_mesg.c
 *  \brief     Message buffer type
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


#include "sd_mesg.h"


struct sd_mesg *sd_mesg_alloc (int type, size_t size, gfp_t flags)
{
	struct sd_mesg *mesg;

	switch ( type )
	{
		case 6:  // SWRITE
			size += offsetof(struct sd_mesg,        mesg) +
			        offsetof(struct sd_mesg_swrite, data);
			break;

		case 10: // DBELL just needs the fixed size, ignore payload
			size = offsetof(struct sd_mesg, mesg) + sizeof(struct sd_mesg_dbell);
			break;

		case 11: // MESSAGE
			size += offsetof(struct sd_mesg,      mesg) +
			        offsetof(struct sd_mesg_mbox, data);
			break;

//		case 13: // RESPONSE
//			size = offsetof(struct sd_mesg, mesg) + sizeof(struct sd_mesg_response);
//			break;

		default:
			return NULL;
	}

	if ( !(mesg = kzalloc(size, flags)) )
		return NULL;

	mesg->type = size;
	mesg->size = size;

	return mesg;
}

void sd_mesg_free (struct sd_mesg *mesg)
{
	kfree(mesg);
}



