/** \file      sd_mesg.h
 *  \brief     Logical message type
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
#ifndef _INCLUDE_SD_MESG_H_
#define _INCLUDE_SD_MESG_H_

#ifdef __KERNEL__
#include <linux/kernel.h>
#include <linux/slab.h>
#else
#include <stdint.h>
#endif


// Max size of an sd_mesg, plus some headroom
#define  SD_MESG_SIZE  (1024 + 65536)


struct sd_mesg_mbox
{
	int       mbox;
	int       letter;
	uint32_t  data[0];
};

struct sd_mesg_swrite
{
	uint64_t  addr;
	uint32_t  data[0];
};

struct sd_mesg_dbell
{
	uint16_t  info;
};

struct sd_mesg_stream
{
	uint16_t  sid;
	uint8_t   cos;
	uint8_t   data[0];
};

struct sd_mesg
{
	int       type;
	size_t    size;
	uint16_t  dst_addr;
	uint16_t  src_addr;
	uint32_t  hello[2];

	union
	{
		struct sd_mesg_mbox    mbox;
		struct sd_mesg_dbell   dbell;
		struct sd_mesg_swrite  swrite;
		struct sd_mesg_stream  stream;
		char                   data[0];
	}
	mesg;

};


#ifdef __KERNEL__
struct sd_mesg *sd_mesg_alloc (int type, size_t size, gfp_t flags);
void sd_mesg_free (struct sd_mesg *mesg);
#endif


#endif /* _INCLUDE_SD_MESG_H_ */
