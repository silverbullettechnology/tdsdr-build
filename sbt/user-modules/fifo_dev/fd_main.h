/** \file      fd_main.h
 *  \brief     Control of PL-side FIFO devices from userspace
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
#ifndef _INCLUDE_FD_MAIN_H
#define _INCLUDE_FD_MAIN_H


#define FD_DRIVER_NODE "fifo_dev"

#define FD_IOCTL_MAGIC 1

#define FD_DSXX_CTRL_ENABLE      0x01
#define FD_DSXX_CTRL_RESET       0x02
#define FD_DSXX_CTRL_DISABLE     0x03
#define FD_DSXX_STAT_ENABLED     (1 << 0)
#define FD_DSXX_STAT_DONE        (1 << 1)
#define FD_DSXX_STAT_ERROR       (1 << 2)
#define FD_DSRC_TYPE_INCREMENT   0x00
#define FD_DSRC_TYPE_DECREMENT   0x01

#define FD_LVDS_CTRL_STOP        0x00
#define FD_LVDS_CTRL_TX_ENB      0x01
#define FD_LVDS_CTRL_RX_ENB      0x02
#define FD_LVDS_CTRL_T1R1        0x04
#define FD_LVDS_CTRL_RESET       0x08
#define FD_LVDS_CTRL_TLAST       0x10
#define FD_LVDS_CTRL_TX_SWP      0x20
#define FD_LVDS_CTRL_RX_SWP      0x40

#define FD_TARGT_ADI1            0x01
#define FD_TARGT_ADI2            0x02
#define FD_TARGT_DSX0            0x04
#define FD_TARGT_DSX1            0x08
#define FD_TARGT_NEW             0x10
#define FD_TARGT_MASK            0x1F

#define FD_NEW_CTRL_RX1          0x01
#define FD_NEW_CTRL_RX2          0x02
#define FD_NEW_CTRL_TX1          0x04
#define FD_NEW_CTRL_TX2          0x08


struct fd_fifo_counts
{
	unsigned long  rx_1_ins;
	unsigned long  rx_1_ext;
	unsigned long  rx_2_ins;
	unsigned long  rx_2_ext;
	unsigned long  tx_1_ins;
	unsigned long  tx_1_ext;
	unsigned long  tx_2_ins;
	unsigned long  tx_2_ext;
};

struct fd_new_adi_regs
{
	unsigned long  adi;
	unsigned long  tx;
	unsigned long  ofs;
	unsigned long  val;
};

struct fd_pmon_regs
{
	unsigned long  ofs;
	unsigned long  val;
};


// Target list bitmap from xparameters
#define  FD_IOCG_TARGET_LIST  _IOR(FD_IOCTL_MAGIC, 0, unsigned long *)

// Some old debugging tools:
// Get counters from ADI DPFD/LVDS interface FIFOs. 
// Get clock counters for digital interface
#define  FD_IOCG_FIFO_CNT          _IOR(FD_IOCTL_MAGIC, 1, struct fd_fifo_counts *)
#define  FD_IOCG_ADI1_OLD_CLK_CNT  _IOR(FD_IOCTL_MAGIC, 2, unsigned long *)
#define  FD_IOCG_ADI2_OLD_CLK_CNT  _IOR(FD_IOCTL_MAGIC, 3, unsigned long *)

// Access to ADI LVDS regs
#define  FD_IOCS_ADI1_OLD_CTRL    _IOW(FD_IOCTL_MAGIC, 10, unsigned long)
#define  FD_IOCG_ADI1_OLD_CTRL    _IOR(FD_IOCTL_MAGIC, 11, unsigned long *)
#define  FD_IOCS_ADI1_OLD_TX_CNT  _IOW(FD_IOCTL_MAGIC, 12, unsigned long)
#define  FD_IOCG_ADI1_OLD_TX_CNT  _IOR(FD_IOCTL_MAGIC, 13, unsigned long *)
#define  FD_IOCS_ADI1_OLD_RX_CNT  _IOW(FD_IOCTL_MAGIC, 14, unsigned long)
#define  FD_IOCG_ADI1_OLD_RX_CNT  _IOR(FD_IOCTL_MAGIC, 15, unsigned long *)
#define  FD_IOCG_ADI1_OLD_SUM     _IOR(FD_IOCTL_MAGIC, 16, unsigned long *)
#define  FD_IOCG_ADI1_OLD_LAST    _IOR(FD_IOCTL_MAGIC, 17, unsigned long *)
#define  FD_IOCS_ADI1_OLD_CS_RST  _IOW(FD_IOCTL_MAGIC, 18, unsigned long)

#define  FD_IOCS_ADI2_OLD_CTRL    _IOW(FD_IOCTL_MAGIC, 20, unsigned long)
#define  FD_IOCG_ADI2_OLD_CTRL    _IOR(FD_IOCTL_MAGIC, 21, unsigned long *)
#define  FD_IOCS_ADI2_OLD_TX_CNT  _IOW(FD_IOCTL_MAGIC, 22, unsigned long)
#define  FD_IOCG_ADI2_OLD_TX_CNT  _IOR(FD_IOCTL_MAGIC, 23, unsigned long *)
#define  FD_IOCS_ADI2_OLD_RX_CNT  _IOW(FD_IOCTL_MAGIC, 24, unsigned long)
#define  FD_IOCG_ADI2_OLD_RX_CNT  _IOR(FD_IOCTL_MAGIC, 25, unsigned long *)
#define  FD_IOCG_ADI2_OLD_SUM     _IOR(FD_IOCTL_MAGIC, 26, unsigned long *)
#define  FD_IOCG_ADI2_OLD_LAST    _IOR(FD_IOCTL_MAGIC, 27, unsigned long *)
#define  FD_IOCS_ADI2_OLD_CS_RST  _IOW(FD_IOCTL_MAGIC, 28, unsigned long)

// Arbitrary register access for now 
#define  FD_IOCG_ADI_NEW_REG      _IOR(FD_IOCTL_MAGIC, 30, struct fd_new_adi_regs *)
#define  FD_IOCS_ADI_NEW_REG_SO   _IOW(FD_IOCTL_MAGIC, 31, struct fd_new_adi_regs *)
#define  FD_IOCS_ADI_NEW_REG_RB   _IOW(FD_IOCTL_MAGIC, 32, struct fd_new_adi_regs *)

// Access to data source regs
#define  FD_IOCS_DSRC0_CTRL   _IOW(FD_IOCTL_MAGIC, 40, unsigned long)
#define  FD_IOCG_DSRC0_STAT   _IOR(FD_IOCTL_MAGIC, 41, unsigned long *)
#define  FD_IOCG_DSRC0_BYTES  _IOR(FD_IOCTL_MAGIC, 42, unsigned long *)
#define  FD_IOCS_DSRC0_BYTES  _IOW(FD_IOCTL_MAGIC, 43, unsigned long)
#define  FD_IOCG_DSRC0_SENT   _IOR(FD_IOCTL_MAGIC, 44, unsigned long *)
#define  FD_IOCG_DSRC0_TYPE   _IOR(FD_IOCTL_MAGIC, 45, unsigned long *)
#define  FD_IOCS_DSRC0_TYPE   _IOW(FD_IOCTL_MAGIC, 46, unsigned long)
#define  FD_IOCG_DSRC0_REPS   _IOR(FD_IOCTL_MAGIC, 47, unsigned long *)
#define  FD_IOCS_DSRC0_REPS   _IOW(FD_IOCTL_MAGIC, 48, unsigned long)
#define  FD_IOCG_DSRC0_RSENT  _IOR(FD_IOCTL_MAGIC, 49, unsigned long *)

// Access to data sink regs
#define  FD_IOCS_DSNK0_CTRL   _IOW(FD_IOCTL_MAGIC, 50, unsigned long)
#define  FD_IOCG_DSNK0_STAT   _IOR(FD_IOCTL_MAGIC, 51, unsigned long *)
#define  FD_IOCG_DSNK0_BYTES  _IOR(FD_IOCTL_MAGIC, 52, unsigned long *)
#define  FD_IOCG_DSNK0_SUM    _IOR(FD_IOCTL_MAGIC, 53, unsigned long *)

// Access to data source regs
#define  FD_IOCS_DSRC1_CTRL   _IOW(FD_IOCTL_MAGIC, 60, unsigned long)
#define  FD_IOCG_DSRC1_STAT   _IOR(FD_IOCTL_MAGIC, 61, unsigned long *)
#define  FD_IOCG_DSRC1_BYTES  _IOR(FD_IOCTL_MAGIC, 62, unsigned long *)
#define  FD_IOCS_DSRC1_BYTES  _IOW(FD_IOCTL_MAGIC, 63, unsigned long)
#define  FD_IOCG_DSRC1_SENT   _IOR(FD_IOCTL_MAGIC, 64, unsigned long *)
#define  FD_IOCG_DSRC1_TYPE   _IOR(FD_IOCTL_MAGIC, 65, unsigned long *)
#define  FD_IOCS_DSRC1_TYPE   _IOW(FD_IOCTL_MAGIC, 66, unsigned long)
#define  FD_IOCG_DSRC1_REPS   _IOR(FD_IOCTL_MAGIC, 67, unsigned long *)
#define  FD_IOCS_DSRC1_REPS   _IOW(FD_IOCTL_MAGIC, 68, unsigned long)
#define  FD_IOCG_DSRC1_RSENT  _IOR(FD_IOCTL_MAGIC, 69, unsigned long *)

// Access to data sink regs
#define  FD_IOCS_DSNK1_CTRL   _IOW(FD_IOCTL_MAGIC, 70, unsigned long)
#define  FD_IOCG_DSNK1_STAT   _IOR(FD_IOCTL_MAGIC, 71, unsigned long *)
#define  FD_IOCG_DSNK1_BYTES  _IOR(FD_IOCTL_MAGIC, 72, unsigned long *)
#define  FD_IOCG_DSNK1_SUM    _IOR(FD_IOCTL_MAGIC, 73, unsigned long *)

// Arbitrary register access for now
#define  FD_IOCG_PMON_REG      _IOR(FD_IOCTL_MAGIC, 80, struct fd_pmon_regs *)
#define  FD_IOCS_PMON_REG_SO   _IOW(FD_IOCTL_MAGIC, 81, struct fd_pmon_regs *)
#define  FD_IOCS_PMON_REG_RB   _IOW(FD_IOCTL_MAGIC, 82, struct fd_pmon_regs *)


#endif /* _INCLUDE_FD_MAIN_H */
/* Ends    : fd_main.h */
