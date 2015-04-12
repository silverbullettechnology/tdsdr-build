/** \file      sd_user.h
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
#ifndef _INCLUDE_SD_USER_H_
#define _INCLUDE_SD_USER_H_

#ifdef __KERNEL__
#include <linux/kernel.h>
#include <linux/ioctl.h>
#else
#include <stdint.h>
#include <sys/ioctl.h>
#endif


struct sd_user_mesg_mbox
{
	int   mbox;
	int   letter;
	char  data[0];
};

struct sd_user_mesg_swrite
{
	uint64_t  addr;
	uint32_t  data[0];
};

struct sd_user_mesg_dbell
{
	uint16_t  info;
};

struct sd_user_mesg
{
	int       type;
	size_t    size;
	uint16_t  dst_addr;
	uint16_t  src_addr;
	uint32_t  hello[2];

	union
	{
		struct sd_user_mesg_mbox    mbox;
		struct sd_user_mesg_dbell   dbell;
		struct sd_user_mesg_swrite  swrite;
		char                        data[0];
	}
	mesg;

};


#define MAGIC_SD_USER 1

// Get/set local device-ID (return address on TX, filtered on RX)
#define SD_USER_IOCG_LOC_DEV_ID     _IOR(MAGIC_SD_USER, 0, unsigned long)
#define SD_USER_IOCS_LOC_DEV_ID     _IOW(MAGIC_SD_USER, 1, unsigned long)

// Get/set bitmask of mailbox subscriptions.  To subscribe to a mailbox set the appropriate
// bit: ie to subscribe to mailbox 2 set bit (1 << 2), to unsubscribe clear the same bit
#define SD_USER_IOCG_MBOX_SUB       _IOR(MAGIC_SD_USER, 2, unsigned long)
#define SD_USER_IOCS_MBOX_SUB       _IOW(MAGIC_SD_USER, 3, unsigned long)

// Get/set range of doorbell values to be notified for
#define SD_USER_IOCG_DBELL_SUB      _IOR(MAGIC_SD_USER, 4, uint16_t[2])
#define SD_USER_IOCS_DBELL_SUB      _IOW(MAGIC_SD_USER, 5, uint16_t[2])

// Get/set range of swrite addresses to be notified for
#define SD_USER_IOCG_SWRITE_SUB     _IOR(MAGIC_SD_USER, 6, uint64_t[2])
#define SD_USER_IOCS_SWRITE_SUB     _IOW(MAGIC_SD_USER, 7, uint64_t[2])


#ifdef __KERNEL__
int  sd_user_init (struct srio_dev *sd);
void sd_user_exit (void);
#endif


#endif /* _INCLUDE_SD_USER_H_ */
