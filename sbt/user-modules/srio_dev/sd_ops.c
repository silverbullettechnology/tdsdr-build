/** \file      sd_ops.c
 *  \brief     Stateless Rapidio stack operations
 *
 *  \copyright Copyright 2015 Silver Bullet Technologies
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
#include <linux/rio.h>

#include "srio_dev.h"
#include "sd_desc.h"
#include "sd_ops.h"


int sd_ops_lcread (struct rio_mport *mport, int index, u32 offset, int len, u32 *data)
{
	struct srio_dev *sd = mport->priv;
	void __iomem    *p;

printk("mport %p, priv %p, index %d, offset 0x%x, len %d, data %p\n",
       mport, sd, index, offset, len, data);

	/* 32-bit length only, check offset */
	if ( len != sizeof(u32) || offset >= RIO_MAINT_SPACE_SZ )
		return -EINVAL;

	p = sd->maint + offset;
printk("p %p\n", p);
	*data = REG_READ(p);
	return 0;
}

int sd_ops_lcwrite (struct rio_mport *mport, int index, u32 offset, int len, u32 data)
{
	struct srio_dev *sd = mport->priv;

	/* 32-bit length only, check offset */
	if ( len != sizeof(u32) || offset >= RIO_MAINT_SPACE_SZ )
		return -EINVAL;

	REG_WRITE(sd->maint + offset, data);
	return 0;
}

int sd_ops_dsend (struct rio_mport *mport, int index, u16 destid, u16 data)
{
//	struct srio_dev *sd  = mport->priv;
//	struct sd_desc   desc;

	printk("sd_ops_dsend(mport %p, index %d, destid 0x%x, data 0x%x)\n",
	       mport, index, destid, data);

	// 02 00 02 00  00 06 00 a0  f9 2f 60 02
	// 02 00 02 00  00 06 00 a0  01 20 60 00
//	if ( sd_desc_alloc(&desc, 

//	*hdr++ = sd_loop_tuser; // src/dst -> tuser
//	*hdr++ = 0xa0000600; // lsw
//	*hdr++ = 0x02602ff9; // msw

	return -ENOSYS;
}

