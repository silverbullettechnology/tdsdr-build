/** \file      sd_rdma.h
 *  \brief     Remote access Rapidio stack operations
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
#ifndef _INCLUDE_SD_RDMA_H
#define _INCLUDE_SD_RDMA_H
#include <linux/kernel.h>
#include <linux/rio.h>


int sd_rdma_cread (struct rio_mport *mport, int index, u16 destid, u8 hopcount,
                   u32 offset, int len, u32 *data)
{
	pr_debug("sd_rdma_cread(mport %p, index %d, destid 0x%x, hopcount %d, offset 0x%x,"
	         " len %d, *data)\n", mport, index, destid, hopcount, offset, len);

	return -ENOSYS;
}


int sd_rdma_cwrite (struct rio_mport *mport, int index, u16 destid, u8 hopcount,
                    u32 offset, int len, u32 data)
{
	pr_debug("sd_rdma_cwrite(mport %p, index %d, destid 0x%x, hopcount %d, offset 0x%x,"
	         " len %d, data 0x%x)\n", mport, index, destid, hopcount, offset, len, data);
	return -ENOSYS;
}


#endif /* _INCLUDE_SD_RDMA_H */
