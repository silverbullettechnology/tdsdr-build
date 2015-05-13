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

#include "sd_mesg.h"


#define  SD_USER_DEV_NODE  "srio"


#define MAGIC_SD_USER 1

// Get/set local device-ID (return address on TX, filtered on RX)
#define SD_USER_IOCG_LOC_DEV_ID     _IOR(MAGIC_SD_USER, 0, unsigned long *)
#define SD_USER_IOCS_LOC_DEV_ID     _IOW(MAGIC_SD_USER, 1, unsigned long)

// Get/set bitmask of mailbox subscriptions.  To subscribe to a mailbox set the appropriate
// bit: ie to subscribe to mailbox 2 set bit (1 << 2), to unsubscribe clear the same bit
#define SD_USER_IOCG_MBOX_SUB       _IOR(MAGIC_SD_USER, 2, unsigned long *)
#define SD_USER_IOCS_MBOX_SUB       _IOW(MAGIC_SD_USER, 3, unsigned long)

// Get/set range of doorbell values to be notified for
#define SD_USER_IOCG_DBELL_SUB      _IOR(MAGIC_SD_USER, 4, uint16_t *)
#define SD_USER_IOCS_DBELL_SUB      _IOW(MAGIC_SD_USER, 5, uint16_t *)

// Get/set range of swrite addresses to be notified for
#define SD_USER_IOCG_SWRITE_SUB     _IOR(MAGIC_SD_USER, 6, uint64_t *)
#define SD_USER_IOCS_SWRITE_SUB     _IOW(MAGIC_SD_USER, 7, uint64_t *)

// SYS_REG controls
#define SD_USER_IOCS_SRIO_RESET     _IOW(MAGIC_SD_USER, 20, unsigned long)
#define SD_USER_IOCS_RXDFELPMRESET  _IOW(MAGIC_SD_USER, 21, unsigned long)
#define SD_USER_IOCS_PHY_LINK_RESET _IOW(MAGIC_SD_USER, 22, unsigned long)
#define SD_USER_IOCS_FORCE_REINIT   _IOW(MAGIC_SD_USER, 23, unsigned long)
#define SD_USER_IOCS_PHY_MCE        _IOW(MAGIC_SD_USER, 24, unsigned long)

// SYS_REG status reporting
#define SD_USER_IOCG_STATUS         _IOR(MAGIC_SD_USER, 30, unsigned long *)

// Get/set test/tuning values
#define SD_USER_IOCG_SWRITE_BYPASS  _IOR(MAGIC_SD_USER, 40, unsigned long *)
#define SD_USER_IOCS_SWRITE_BYPASS  _IOW(MAGIC_SD_USER, 41, unsigned long)
#define SD_USER_IOCG_GT_LOOPBACK    _IOR(MAGIC_SD_USER, 42, unsigned long *)
#define SD_USER_IOCS_GT_LOOPBACK    _IOW(MAGIC_SD_USER, 43, unsigned long)
#define SD_USER_IOCG_GT_DIFFCTRL    _IOR(MAGIC_SD_USER, 44, unsigned long *)
#define SD_USER_IOCS_GT_DIFFCTRL    _IOW(MAGIC_SD_USER, 45, unsigned long)
#define SD_USER_IOCG_GT_TXPRECURS   _IOR(MAGIC_SD_USER, 46, unsigned long *)
#define SD_USER_IOCS_GT_TXPRECURS   _IOW(MAGIC_SD_USER, 47, unsigned long)
#define SD_USER_IOCG_GT_TXPOSTCURS  _IOR(MAGIC_SD_USER, 48, unsigned long *)
#define SD_USER_IOCS_GT_TXPOSTCURS  _IOW(MAGIC_SD_USER, 49, unsigned long)
#define SD_USER_IOCG_GT_RXLPMEN     _IOR(MAGIC_SD_USER, 50, unsigned long *)
#define SD_USER_IOCS_GT_RXLPMEN     _IOW(MAGIC_SD_USER, 51, unsigned long)

struct sd_user_cm_ctrl
{
	unsigned  ch;
	unsigned  sel;
	unsigned  trim;
};

// Control of RX common-mode termination for CH 0..3.
#define SD_USER_IOCG_CM_CTRL        _IOR(MAGIC_SD_USER, 60, struct sd_user_cm_ctrl *)
#define SD_USER_IOCS_CM_CTRL        _IOW(MAGIC_SD_USER, 61, struct sd_user_cm_ctrl *)


#ifdef __KERNEL__
int  sd_user_init (struct srio_dev *sd);
void sd_user_exit (void);
#endif


#endif /* _INCLUDE_SD_USER_H_ */
