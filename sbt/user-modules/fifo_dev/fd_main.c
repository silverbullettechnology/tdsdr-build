/** \file      fd_main.c
 *  \brief     Control of PL-side FIFO devices from userspace
 *
 *  \copyright Copyright 2014 Silver Bullet Technologies
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
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/version.h>
#include <linux/module.h>
#include <linux/io.h>
#include <linux/mm.h>
#include <linux/ioport.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/of.h>

#include "fd_main.h"
#include "fd_regs.h"


#define pr_trace(fmt,...) do{ }while(0)
//#define pr_trace pr_debug


static spinlock_t  fd_lock;
static int         fd_users;


static struct fd_dsrc_regs __iomem *fd_dsrc0_regs = NULL;
static struct fd_dsnk_regs __iomem *fd_dsnk0_regs = NULL;
static struct fd_dsrc_regs __iomem *fd_dsrc1_regs = NULL;
static struct fd_dsnk_regs __iomem *fd_dsnk1_regs = NULL;
static struct fd_lvds_regs __iomem *fd_adi1_old_regs = NULL;
static struct fd_lvds_regs __iomem *fd_adi2_old_regs = NULL;

static u32 __iomem *fd_adi1_new_regs = NULL;
static u32 __iomem *fd_adi2_new_regs = NULL;

static u32 __iomem *fd_rx_fifo1_cnt = NULL;
static u32 __iomem *fd_rx_fifo2_cnt = NULL;
static u32 __iomem *fd_tx_fifo1_cnt = NULL;
static u32 __iomem *fd_tx_fifo2_cnt = NULL;

static void __iomem *fd_pmon_regs = NULL;


/******** Userspace interface ********/

static int fd_open (struct inode *inode_p, struct file *file_p)
{
	unsigned long  flags;
	int            ret = -EACCES;

	spin_lock_irqsave(&fd_lock, flags);
	if ( !fd_users )
	{
		pr_debug("%s()\n", __func__);
		fd_users = 1;
		ret = 0;
	}
	spin_unlock_irqrestore(&fd_lock, flags);

	return ret;
}


static long fd_ioctl (struct file *filp, unsigned int cmd, unsigned long arg)
{
	unsigned long  reg;
	int            ret = -ENOSYS;
	int            max = 10;

	pr_trace("%s(cmd %x, arg %08lx)\n", __func__, cmd, arg);

	if ( _IOC_TYPE(cmd) != FD_IOCTL_MAGIC )
		return -EINVAL;

	switch ( cmd )
	{
		case  FD_IOCG_FIFO_CNT:
		{
			struct fd_fifo_counts buff;
			pr_debug("FD_IOCG_FIFO_CNT\n");

			memset(&buff, 0xFF, sizeof(buff));
			if ( fd_rx_fifo1_cnt )
			{
				buff.rx_1_ins = REG_READ(fd_rx_fifo1_cnt);
				buff.rx_1_ext = REG_READ(&fd_rx_fifo1_cnt[1]);
			}
			if ( fd_rx_fifo2_cnt )
			{
				buff.rx_2_ins = REG_READ(fd_rx_fifo2_cnt);
				buff.rx_2_ext = REG_READ(&fd_rx_fifo2_cnt[1]);
			}
			if ( fd_tx_fifo1_cnt )
			{
				buff.tx_1_ins = REG_READ(fd_tx_fifo1_cnt);
				buff.tx_1_ext = REG_READ(&fd_tx_fifo1_cnt[1]);
			}
			if ( fd_tx_fifo2_cnt )
			{
				buff.tx_2_ins = REG_READ(fd_tx_fifo2_cnt);
				buff.tx_2_ext = REG_READ(&fd_tx_fifo2_cnt[1]);
			}

			ret = copy_to_user((void *)arg, &buff, sizeof(buff));
			break;
		}


		case FD_IOCG_ADI1_OLD_CLK_CNT:
			pr_debug("FD_IOCG_CLK_CNT\n");
			if ( fd_adi1_old_regs )
			{
				reg = REG_READ(&fd_adi1_old_regs->cs_rst);
				ret = arg ? put_user(reg, (unsigned long *)arg) : 0;
			}
			else
				printk ("fd_adi1_old_regs NULL, no clk\n");
			break;

		case FD_IOCG_ADI2_OLD_CLK_CNT:
			pr_debug("FD_IOCG_CLK_CNT\n");
			if ( fd_adi2_old_regs )
			{
				reg = REG_READ(&fd_adi2_old_regs->cs_rst);
				ret = arg ? put_user(reg, (unsigned long *)arg) : 0;
			}
			else
				printk ("fd_adi2_old_regs NULL, no clk\n");
			break;


		// Access to data source regs
		case FD_IOCS_DSRC0_CTRL:
			if ( fd_dsrc0_regs )
			{
				pr_debug("FD_IOCS_DSRC0_CTRL %08lx\n", arg);
				REG_WRITE(&fd_dsrc0_regs->ctrl, arg);
				ret = 0;
			}
			else
				ret = -ENODEV;
			break;

		case FD_IOCG_DSRC0_STAT:
			if ( fd_dsrc0_regs )
			{
				reg = REG_READ(&fd_dsrc0_regs->stat);
				pr_debug("FD_IOCG_DSRC0_STAT %08lx\n", reg);
				ret = put_user(reg, (unsigned long *)arg);
			}
			else
				ret = -ENODEV;
			break;

		case FD_IOCG_DSRC0_BYTES:
			if ( fd_dsrc0_regs )
			{
				reg = REG_READ(&fd_dsrc0_regs->bytes);
				pr_debug("FD_IOCG_DSRC0_BYTES %08lx\n", reg);
				ret = put_user(reg, (unsigned long *)arg);
			}
			else
				ret = -ENODEV;
			break;

		case FD_IOCS_DSRC0_BYTES:
			if ( fd_dsrc0_regs )
			{
				pr_debug("FD_IOCS_DSRC0_BYTES %08lx\n", arg);
				REG_WRITE(&fd_dsrc0_regs->bytes, arg);
				ret = 0;
			}
			else
				ret = -ENODEV;
			break;

		case FD_IOCG_DSRC0_SENT:
			if ( fd_dsrc0_regs )
			{
				reg = REG_READ(&fd_dsrc0_regs->sent);
				pr_debug("FD_IOCG_DSRC0_SENT %08lx\n", reg);
				ret = put_user(reg, (unsigned long *)arg);
			}
			else
				ret = -ENODEV;
			break;

		case FD_IOCG_DSRC0_TYPE:
			if ( fd_dsrc0_regs )
			{
				reg = REG_READ(&fd_dsrc0_regs->type);
				pr_debug("FD_IOCG_DSRC0_TYPE %08lx\n", reg);
				ret = put_user(reg, (unsigned long *)arg);
			}
			else
				ret = -ENODEV;
			break;

		case FD_IOCS_DSRC0_TYPE:
			if ( fd_dsrc0_regs )
			{
				pr_debug("FD_IOCS_DSRC0_TYPE %08lx\n", arg);
				REG_WRITE(&fd_dsrc0_regs->type, arg);
				ret = 0;
			}
			else
				ret = -ENODEV;
			break;

		case FD_IOCG_DSRC0_REPS:
			if ( fd_dsrc0_regs )
			{
				reg = REG_READ(&fd_dsrc0_regs->reps);
				pr_debug("FD_IOCG_DSRC0_REPS %08lx\n", reg);
				ret = put_user(reg, (unsigned long *)arg);
			}
			else
				ret = -ENODEV;
			break;

		case FD_IOCS_DSRC0_REPS:
			if ( fd_dsrc0_regs )
			{
				pr_debug("FD_IOCS_DSRC0_REPS %08lx\n", arg);
				REG_WRITE(&fd_dsrc0_regs->reps, arg);
				ret = 0;
			}
			else
				ret = -ENODEV;
			break;

		case FD_IOCG_DSRC0_RSENT:
			if ( fd_dsrc0_regs )
			{
				reg = REG_READ(&fd_dsrc0_regs->rsent);
				pr_debug("FD_IOCG_DSRC0_RSENT %08lx\n", reg);
				ret = put_user(reg, (unsigned long *)arg);
			}
			else
				ret = -ENODEV;
			break;


		// Access to data sink regs
		case FD_IOCS_DSNK0_CTRL:
			if ( fd_dsnk0_regs )
			{
				pr_debug("FD_IOCS_DSNK0_CTRL %08lx\n", arg);
				REG_WRITE(&fd_dsnk0_regs->ctrl, arg);
				ret = 0;
			}
			else
				ret = -ENODEV;
			break;

		case FD_IOCG_DSNK0_STAT:
			if ( fd_dsnk0_regs )
			{
				reg = REG_READ(&fd_dsnk0_regs->stat);
				pr_debug("FD_IOCG_DSNK0_STAT %08lx\n", reg);
				ret = put_user(reg, (unsigned long *)arg);
			}
			else
				ret = -ENODEV;
			break;

		case FD_IOCG_DSNK0_BYTES:
			if ( fd_dsnk0_regs )
			{
				reg = REG_READ(&fd_dsnk0_regs->bytes);
				pr_debug("FD_IOCG_DSNK0_BYTES %08lx\n", reg);
				ret = put_user(reg, (unsigned long *)arg);
			}
			else
				ret = -ENODEV;
			break;

		case FD_IOCG_DSNK0_SUM:
			if ( fd_dsnk0_regs )
			{
				unsigned long sum[2];
				
				sum[0] = REG_READ(&fd_dsnk0_regs->sum_h);
				sum[1] = REG_READ(&fd_dsnk0_regs->sum_l);

				pr_debug("FD_IOCG_DSNK0_SUM %08lx.%08lx\n", sum[0], sum[1]);
				ret = copy_to_user((void *)arg, sum, sizeof(sum));
			}
			else
				ret = -ENODEV;
			break;


		// Access to data source regs
		case FD_IOCS_DSRC1_CTRL:
			if ( fd_dsrc1_regs )
			{
				pr_debug("FD_IOCS_DSRC1_CTRL %08lx\n", arg);
				REG_WRITE(&fd_dsrc1_regs->ctrl, arg);
				ret = 0;
			}
			else
				ret = -ENODEV;
			break;

		case FD_IOCG_DSRC1_STAT:
			if ( fd_dsrc1_regs )
			{
				reg = REG_READ(&fd_dsrc1_regs->stat);
				pr_debug("FD_IOCG_DSRC1_STAT %08lx\n", reg);
				ret = put_user(reg, (unsigned long *)arg);
			}
			else
				ret = -ENODEV;
			break;

		case FD_IOCG_DSRC1_BYTES:
			if ( fd_dsrc1_regs )
			{
				reg = REG_READ(&fd_dsrc1_regs->bytes);
				pr_debug("FD_IOCG_DSRC1_BYTES %08lx\n", reg);
				ret = put_user(reg, (unsigned long *)arg);
			}
			else
				ret = -ENODEV;
			break;

		case FD_IOCS_DSRC1_BYTES:
			if ( fd_dsrc1_regs )
			{
				pr_debug("FD_IOCS_DSRC1_BYTES %08lx\n", arg);
				REG_WRITE(&fd_dsrc1_regs->bytes, arg);
				ret = 0;
			}
			else
				ret = -ENODEV;
			break;

		case FD_IOCG_DSRC1_SENT:
			if ( fd_dsrc1_regs )
			{
				reg = REG_READ(&fd_dsrc1_regs->sent);
				pr_debug("FD_IOCG_DSRC1_SENT %08lx\n", reg);
				ret = put_user(reg, (unsigned long *)arg);
			}
			else
				ret = -ENODEV;
			break;

		case FD_IOCG_DSRC1_TYPE:
			if ( fd_dsrc1_regs )
			{
				reg = REG_READ(&fd_dsrc1_regs->type);
				pr_debug("FD_IOCG_DSRC1_TYPE %08lx\n", reg);
				ret = put_user(reg, (unsigned long *)arg);
			}
			else
				ret = -ENODEV;
			break;

		case FD_IOCS_DSRC1_TYPE:
			if ( fd_dsrc1_regs )
			{
				pr_debug("FD_IOCS_DSRC1_TYPE %08lx\n", arg);
				REG_WRITE(&fd_dsrc1_regs->type, arg);
				ret = 0;
			}
			else
				ret = -ENODEV;
			break;

		case FD_IOCG_DSRC1_REPS:
			if ( fd_dsrc1_regs )
			{
				reg = REG_READ(&fd_dsrc1_regs->reps);
				pr_debug("FD_IOCG_DSRC1_REPS %08lx\n", reg);
				ret = put_user(reg, (unsigned long *)arg);
			}
			else
				ret = -ENODEV;
			break;

		case FD_IOCS_DSRC1_REPS:
			if ( fd_dsrc1_regs )
			{
				pr_debug("FD_IOCS_DSRC1_REPS %08lx\n", arg);
				REG_WRITE(&fd_dsrc1_regs->reps, arg);
				ret = 0;
			}
			else
				ret = -ENODEV;
			break;

		case FD_IOCG_DSRC1_RSENT:
			if ( fd_dsrc1_regs )
			{
				reg = REG_READ(&fd_dsrc1_regs->rsent);
				pr_debug("FD_IOCG_DSRC1_RSENT %08lx\n", reg);
				ret = put_user(reg, (unsigned long *)arg);
			}
			else
				ret = -ENODEV;
			break;


		// Access to data sink regs
		case FD_IOCS_DSNK1_CTRL:
			if ( fd_dsnk1_regs )
			{
				pr_debug("FD_IOCS_DSNK1_CTRL %08lx\n", arg);
				REG_WRITE(&fd_dsnk1_regs->ctrl, arg);
				ret = 0;
			}
			else
				ret = -ENODEV;
			break;

		case FD_IOCG_DSNK1_STAT:
			if ( fd_dsnk1_regs )
			{
				reg = REG_READ(&fd_dsnk1_regs->stat);
				pr_debug("FD_IOCG_DSNK1_STAT %08lx\n", reg);
				ret = put_user(reg, (unsigned long *)arg);
			}
			else
				ret = -ENODEV;
			break;

		case FD_IOCG_DSNK1_BYTES:
			if ( fd_dsnk1_regs )
			{
				reg = REG_READ(&fd_dsnk1_regs->bytes);
				pr_debug("FD_IOCG_DSNK1_BYTES %08lx\n", reg);
				ret = put_user(reg, (unsigned long *)arg);
			}
			else
				ret = -ENODEV;
			break;

		case FD_IOCG_DSNK1_SUM:
			if ( fd_dsnk1_regs )
			{
				unsigned long sum[2];
				
				sum[0] = REG_READ(&fd_dsnk1_regs->sum_h);
				sum[1] = REG_READ(&fd_dsnk1_regs->sum_l);

				pr_debug("FD_IOCG_DSNK1_SUM %08lx.%08lx\n", sum[0], sum[1]);
				ret = copy_to_user((void *)arg, sum, sizeof(sum));
			}
			else
				ret = -ENODEV;
			break;


		// Access to ADI LVDS regs
		case FD_IOCS_ADI1_OLD_CTRL:
			if ( fd_adi1_old_regs )
			{
				pr_debug("FD_IOCS_ADI1_OLD_CTRL %08lx\n", arg);
				REG_WRITE(&fd_adi1_old_regs->ctrl, arg);
				ret = 0;
			}
			else
				ret = -ENODEV;
			break;

		case FD_IOCG_ADI1_OLD_CTRL:
			if ( fd_adi1_old_regs )
			{
				reg = REG_READ(&fd_adi1_old_regs->ctrl);
				pr_debug("FD_IOCG_ADI1_OLD_CTRL %08lx\n", reg);
				ret = put_user(reg, (unsigned long *)arg);
			}
			else
				ret = -ENODEV;
			break;

		case FD_IOCS_ADI1_OLD_TX_CNT:
			if ( fd_adi1_old_regs )
			{
				pr_debug("FD_IOCS_ADI1_OLD_TX_CNT %08lx (%d)\n", arg, max);
				REG_WRITE(&fd_adi1_old_regs->tx_cnt, arg);
				ret = 0;
			}
			else
				ret = -ENODEV;
			break;

		case FD_IOCG_ADI1_OLD_TX_CNT:
			if ( fd_adi1_old_regs )
			{
				reg = REG_READ(&fd_adi1_old_regs->tx_cnt);
				pr_debug("FD_IOCG_ADI1_OLD_TX_CNT %08lx\n", reg);
				ret = put_user(reg, (unsigned long *)arg);
			}
			else
				ret = -ENODEV;
			break;

		case FD_IOCS_ADI1_OLD_RX_CNT:
			if ( fd_adi1_old_regs )
			{
				pr_debug("FD_IOCS_ADI1_OLD_RX_CNT %08lx\n", arg);
				REG_WRITE(&fd_adi1_old_regs->rx_cnt, arg);
				ret = 0;
			}
			else
				ret = -ENODEV;
			break;

		case FD_IOCG_ADI1_OLD_RX_CNT:
			if ( fd_adi1_old_regs )
			{
				reg = REG_READ(&fd_adi1_old_regs->rx_cnt);
				pr_debug("FD_IOCG_ADI1_OLD_RX_CNT %08lx\n", reg);
				ret = put_user(reg, (unsigned long *)arg);
			}
			else
				ret = -ENODEV;
			break;

		case FD_IOCG_ADI1_OLD_SUM:
			if ( fd_adi1_old_regs )
			{
				unsigned long sum[2];
				
				sum[0] = REG_READ(&fd_adi1_old_regs->cs_hi);
				sum[1] = REG_READ(&fd_adi1_old_regs->cs_low);

				pr_debug("FD_IOCG_ADI1_OLD_SUM %08lx.%08lx\n", sum[0], sum[1]);
				ret = copy_to_user((void *)arg, sum, sizeof(sum));
			}
			else
				ret = -ENODEV;
			break;

		case FD_IOCG_ADI1_OLD_LAST:
			if ( fd_adi1_old_regs )
			{
				unsigned long last[2];
				
				last[0] = REG_READ(&fd_adi1_old_regs->tx_hi);
				last[1] = REG_READ(&fd_adi1_old_regs->tx_low);

				pr_debug("FD_IOCG_ADI1_OLD_LAST %08lx.%08lx\n", last[0], last[1]);
				ret = copy_to_user((void *)arg, last, sizeof(last));
			}
			else
				ret = -ENODEV;
			break;

		case FD_IOCS_ADI1_OLD_CS_RST:
			if ( fd_adi1_old_regs )
			{
				pr_debug("FD_IOCS_ADI1_OLD_CS_RST %08lx\n", arg);
				REG_WRITE(&fd_adi1_old_regs->cs_rst, arg);
				ret = 0;
			}
			else
				ret = -ENODEV;
			break;


		case FD_IOCS_ADI2_OLD_CTRL:
			if ( fd_adi2_old_regs )
			{
				pr_debug("FD_IOCS_ADI2_OLD_CTRL %08lx\n", arg);
				REG_WRITE(&fd_adi2_old_regs->ctrl, arg);
				ret = 0;
			}
			else
				ret = -ENODEV;
			break;

		case FD_IOCG_ADI2_OLD_CTRL:
			if ( fd_adi2_old_regs )
			{
				reg = REG_READ(&fd_adi2_old_regs->ctrl);
				pr_debug("FD_IOCG_ADI2_OLD_CTRL %08lx\n", reg);
				ret = put_user(reg, (unsigned long *)arg);
			}
			else
				ret = -ENODEV;
			break;

		case FD_IOCS_ADI2_OLD_TX_CNT:
			if ( fd_adi2_old_regs )
			{
				pr_debug("FD_IOCS_ADI2_OLD_TX_CNT %08lx (%d)\n", arg, max);
				REG_WRITE(&fd_adi2_old_regs->tx_cnt, arg);
				ret = 0;
			}
			else
				ret = -ENODEV;
			break;

		case FD_IOCG_ADI2_OLD_TX_CNT:
			if ( fd_adi2_old_regs )
			{
				reg = REG_READ(&fd_adi2_old_regs->tx_cnt);
				pr_debug("FD_IOCG_ADI2_OLD_TX_CNT %08lx\n", reg);
				ret = put_user(reg, (unsigned long *)arg);
			}
			else
				ret = -ENODEV;
			break;

		case FD_IOCS_ADI2_OLD_RX_CNT:
			if ( fd_adi2_old_regs )
			{
				pr_debug("FD_IOCS_ADI2_OLD_RX_CNT%08lx\n", arg);
				REG_WRITE(&fd_adi2_old_regs->rx_cnt, arg);
				ret = 0;
			}
			else
				ret = -ENODEV;
			break;

		case FD_IOCG_ADI2_OLD_RX_CNT:
			if ( fd_adi2_old_regs )
			{
				reg = REG_READ(&fd_adi2_old_regs->rx_cnt);
				pr_debug("FD_IOCG_ADI2_OLD_RX_CNT %08lx\n", reg);
				ret = put_user(reg, (unsigned long *)arg);
			}
			else
				ret = -ENODEV;
			break;

		case FD_IOCG_ADI2_OLD_SUM:
			if ( fd_adi2_old_regs )
			{
				unsigned long sum[2];
				
				sum[0] = REG_READ(&fd_adi2_old_regs->cs_hi);
				sum[1] = REG_READ(&fd_adi2_old_regs->cs_low);

				pr_debug("FD_IOCG_ADI2_OLD_SUM %08lx.%08lx\n", sum[0], sum[1]);
				ret = copy_to_user((void *)arg, sum, sizeof(sum));
			}
			else
				ret = -ENODEV;
			break;

		case FD_IOCG_ADI2_OLD_LAST:
			if ( fd_adi2_old_regs )
			{
				unsigned long last[2];
				
				last[0] = REG_READ(&fd_adi2_old_regs->tx_hi);
				last[1] = REG_READ(&fd_adi2_old_regs->tx_low);

				pr_debug("FD_IOCG_ADI2_OLD_LAST %08lx.%08lx\n", last[0], last[1]);
				ret = copy_to_user((void *)arg, last, sizeof(last));
			}
			else
				ret = -ENODEV;
			break;

		case FD_IOCS_ADI2_OLD_CS_RST:
			if ( fd_adi2_old_regs )
			{
				pr_debug("FD_IOCS_ADI2_OLD_CS_RST %08lx\n", arg);
				REG_WRITE(&fd_adi2_old_regs->cs_rst, arg);
				ret = 0;
			}
			else
				ret = -ENODEV;
			break;


		// Target list bitmap from xparameters
		case FD_IOCG_TARGET_LIST:
			reg = 0;
			if ( fd_dsrc0_regs ) reg |= FD_TARGT_DSX0;
			if ( fd_dsnk0_regs ) reg |= FD_TARGT_DSX0;
			if ( fd_dsrc1_regs ) reg |= FD_TARGT_DSX1;
			if ( fd_dsnk1_regs ) reg |= FD_TARGT_DSX1;
			if ( fd_adi1_old_regs ) reg |= FD_TARGT_ADI1;
			if ( fd_adi2_old_regs ) reg |= FD_TARGT_ADI2;
			if ( fd_adi1_new_regs ) reg |= FD_TARGT_ADI1|FD_TARGT_NEW;
			if ( fd_adi2_new_regs ) reg |= FD_TARGT_ADI2|FD_TARGT_NEW;
			pr_debug("FD_IOCG_TARGET_LIST %08lx { %s%s%s%s%s}\n", reg, 
			         reg & FD_TARGT_DSX0 ? "dsx0 " : "",
			         reg & FD_TARGT_DSX1 ? "dsx1 " : "",
			         reg & FD_TARGT_NEW  ? "new: " : "",
			         reg & FD_TARGT_ADI1 ? "adi1 " : "",
			         reg & FD_TARGT_ADI2 ? "adi2 " : "");
			ret = put_user(reg, (unsigned long *)arg);
			break;


		// Arbitrary register access for now 
		case FD_IOCG_ADI_NEW_REG:
		case FD_IOCS_ADI_NEW_REG_SO:
		case FD_IOCS_ADI_NEW_REG_RB:
		{
			struct fd_new_adi_regs  regs;
			void __iomem            *addr = NULL;

			if ( (ret = copy_from_user(&regs, (void *)arg, sizeof(regs))) )
			{
				pr_err("failed to copy %d bytes, stop\n", ret);
				return -EFAULT;
			}

			// validate device and regs pointer
			switch ( regs.adi )
			{
				case 0: addr = fd_adi1_new_regs; break;
				case 1: addr = fd_adi2_new_regs; break;
				default:
					pr_err("regs.dev %lu invalid, stop\n", regs.adi);
					return -EINVAL;
			}
			if ( !addr )
			{
				pr_err("register access pointers not setup, stop.\n");
				return -ENODEV;
			}

			switch ( cmd )
			{
				case FD_IOCS_ADI_NEW_REG_SO:
				case FD_IOCS_ADI_NEW_REG_RB:
					REG_WRITE(ADI_NEW_RT_ADDR(addr, regs.ofs, regs.tx), regs.val);
					pr_debug("FD_IOCS_ADI_NEW_REG AD%c %cX +%04lx WRITE %08lx\n",
					         (unsigned char)regs.adi + '1', regs.tx ? 'T' : 'R',
					         regs.ofs, regs.val);
					// FD_IOCS_ADI_NEW_REG_SO: set-only, break here
					// FD_IOCS_ADI_NEW_REG_RB: fall-through, readback
					if ( cmd == FD_IOCS_ADI_NEW_REG_SO )
						break;

				case FD_IOCG_ADI_NEW_REG:
					regs.val = REG_READ(ADI_NEW_RT_ADDR(addr, regs.ofs, regs.tx));
					pr_debug("FD_IOCG_ADI_NEW_REG AD%c %cX +%04lx READ %08lx\n",
					         (unsigned char)regs.adi + '1', regs.tx ? 'T' : 'R',
					         regs.ofs, regs.val);
					break;

			}

			// set-only: no need to copy back to userspace
			if ( cmd == FD_IOCS_ADI_NEW_REG_SO )
				ret = 0;
			// read or write/readback: copy back to userspace
			else if ( (ret = copy_to_user((void *)arg, &regs, sizeof(regs))) )
			{
				pr_err("failed to copy %d bytes, stop\n", ret);
				return -EFAULT;
			}
			break;
		}

		// Arbitrary register access for now
		case FD_IOCG_PMON_REG:
		case FD_IOCS_PMON_REG_SO:
		case FD_IOCS_PMON_REG_RB:
		{
			struct fd_pmon_regs  regs;
			void __iomem        *addr = fd_pmon_regs;

			if ( !addr )
			{
				pr_err("register access pointers not setup, stop.\n");
				return -ENODEV;
			}

			if ( (ret = copy_from_user(&regs, (void *)arg, sizeof(regs))) )
			{
				pr_err("failed to copy %d bytes, stop\n", ret);
				return -EFAULT;
			}
			addr += regs.ofs;

			switch ( cmd )
			{
				case FD_IOCS_PMON_REG_SO:
				case FD_IOCS_PMON_REG_RB:
					REG_WRITE(addr, regs.val);
					pr_debug("FD_IOCS_PMON_REG +%04lx WRITE %08lx\n", regs.ofs, regs.val);
					// FD_IOCS_PMON_REG_SO: set-only, break here
					// FD_IOCS_PMON_REG_RB: fall-through, readback
					if ( cmd == FD_IOCS_PMON_REG_SO )
						break;

				case FD_IOCG_PMON_REG:
					regs.val = REG_READ(addr);
					pr_debug("FD_IOCG_PMON_REG +%04lx READ %08lx\n", regs.ofs, regs.val);
					break;

			}

			// set-only: no need to copy back to userspace
			if ( cmd == FD_IOCS_PMON_REG_SO )
				ret = 0;
			// read or write/readback: copy back to userspace
			else if ( (ret = copy_to_user((void *)arg, &regs, sizeof(regs))) )
			{
				pr_err("failed to copy %d bytes, stop\n", ret);
				return -EFAULT;
			}
			break;
		}
	}

	if ( ret )
		pr_debug("%s(): return %d\n", __func__, ret);

	return ret;
}


static int fd_release (struct inode *inode_p, struct file *file_p)
{
	unsigned long  flags;
	int            ret = -EBADF;

	spin_lock_irqsave(&fd_lock, flags);
	if ( fd_users )
	{
		pr_debug("%s()\n", __func__);
		fd_users = 0;
		ret = 0;
	}
	spin_unlock_irqrestore(&fd_lock, flags);

	return ret;
}


static struct file_operations fops = 
{
	open:           fd_open,
	unlocked_ioctl: fd_ioctl,
	release:        fd_release,
};

static struct miscdevice mdev = { MISC_DYNAMIC_MINOR, FD_DRIVER_NODE, &fops };



static int fd_remove (struct platform_device *pdev)
{
	misc_deregister(&mdev);

	return 0;
}

static void __iomem *fd_iomap (struct platform_device *pdev, char *name)
{
	struct resource *res;
	void __iomem    *ret;

	if ( !(res = platform_get_resource_byname(pdev, IORESOURCE_MEM, name)) )
		return NULL;

	ret = devm_ioremap_resource(&pdev->dev, res);
	if ( IS_ERR(ret) )
	{
		dev_err(&pdev->dev, "%s: resource %s ioremap fail, skip\n", __func__, name);
		return NULL;
	}

	pr_info("%s: %s -> start %08x -> mapped %p\n", dev_name(&pdev->dev),
	        name, res->start, ret);
	return ret;
}


static int fd_probe (struct platform_device *pdev)
{
	int  ret;

	fd_dsrc0_regs    = fd_iomap(pdev, "dsrc0");
	fd_dsnk0_regs    = fd_iomap(pdev, "dsnk0");
	fd_dsrc1_regs    = fd_iomap(pdev, "dsrc1");
	fd_dsnk1_regs    = fd_iomap(pdev, "dsnk1");
	fd_adi1_old_regs = fd_iomap(pdev, "adi1-old");
	fd_adi2_old_regs = fd_iomap(pdev, "adi2-old");
	fd_adi1_new_regs = fd_iomap(pdev, "adi1-new");
	fd_adi2_new_regs = fd_iomap(pdev, "adi2-new");
	fd_rx_fifo1_cnt  = fd_iomap(pdev, "rx-fifo1");
	fd_rx_fifo2_cnt  = fd_iomap(pdev, "rx-fifo2");
	fd_tx_fifo1_cnt  = fd_iomap(pdev, "tx-fifo1");
	fd_tx_fifo2_cnt  = fd_iomap(pdev, "tx-fifo2");
	fd_pmon_regs     = fd_iomap(pdev, "pmon");

	if ( (ret = misc_register(&mdev)) < 0 )
	{
		pr_err("misc_register() failed\n");
		ret = -EIO;
		goto fail;
	}

	pr_info("registered successfully\n");
	return 0;

fail:
	if ( fd_dsrc0_regs    )  devm_iounmap(&pdev->dev, fd_dsrc0_regs);
	if ( fd_dsnk0_regs    )  devm_iounmap(&pdev->dev, fd_dsnk0_regs);
	if ( fd_dsrc1_regs    )  devm_iounmap(&pdev->dev, fd_dsrc1_regs);
	if ( fd_dsnk1_regs    )  devm_iounmap(&pdev->dev, fd_dsnk1_regs);
	if ( fd_adi1_old_regs )  devm_iounmap(&pdev->dev, fd_adi1_old_regs);
	if ( fd_adi2_old_regs )  devm_iounmap(&pdev->dev, fd_adi2_old_regs);
	if ( fd_adi1_new_regs )  devm_iounmap(&pdev->dev, fd_adi1_new_regs);
	if ( fd_adi2_new_regs )  devm_iounmap(&pdev->dev, fd_adi2_new_regs);
	if ( fd_rx_fifo1_cnt  )  devm_iounmap(&pdev->dev, fd_rx_fifo1_cnt);
	if ( fd_rx_fifo2_cnt  )  devm_iounmap(&pdev->dev, fd_rx_fifo2_cnt);
	if ( fd_tx_fifo1_cnt  )  devm_iounmap(&pdev->dev, fd_tx_fifo1_cnt);
	if ( fd_tx_fifo2_cnt  )  devm_iounmap(&pdev->dev, fd_tx_fifo2_cnt);
	if ( fd_pmon_regs     )  devm_iounmap(&pdev->dev, fd_pmon_regs);

	return ret;
}


static const struct of_device_id fd_ids[] = {
	{ .compatible = "sbt,fifo-dev" },
	{}
};

static struct platform_driver fd_driver = {
	.driver = {
		.name = "fifo-dev",
		.owner = THIS_MODULE,
		.of_match_table = fd_ids,
	},
	.probe  = fd_probe,
	.remove = fd_remove,
};

static int __init fd_init (void)
{
	return platform_driver_register(&fd_driver);
}
module_init(fd_init);

static void __exit fd_exit (void)
{
	platform_driver_unregister(&fd_driver);
}
module_exit(fd_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Morgan Hughes <morgan.hughes@silver-bullet-tech.com>");
MODULE_DESCRIPTION("SBT FIFO device driver");

