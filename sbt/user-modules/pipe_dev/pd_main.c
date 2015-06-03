/** \file      pd_main.c
 *  \brief     Control of PL-side sample pipeline devices
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
#include <linux/delay.h>

#include "pd_main.h"
#include "pd_regs.h"


#define pr_trace(fmt,...) do{ }while(0)
//#define pr_trace pr_debug


static spinlock_t  pd_lock;
static int         pd_users;


static struct pd_adi2axis_regs      __iomem *adi2axis[2];
static struct pd_vita49_clk_regs    __iomem *vita49_clk;
static struct pd_vita49_pack_regs   __iomem *vita49_pack[2];
static struct pd_vita49_unpack_regs __iomem *vita49_unpack[2];
static struct pd_vita49_assem_regs  __iomem *vita49_assem[2];
static struct pd_vita49_trig_regs   __iomem *vita49_trig_adc[2];
static struct pd_vita49_trig_regs   __iomem *vita49_trig_dac[2];
static struct pd_swrite_pack_regs   __iomem *swrite_pack[2];
static struct pd_swrite_unpack_regs __iomem *swrite_unpack;
static struct pd_routing_reg        __iomem *routing_reg;


/******** Userspace interface ********/

static int pd_open (struct inode *inode_p, struct file *file_p)
{
	unsigned long  flags;
	int            ret = -EACCES;

	spin_lock_irqsave(&pd_lock, flags);
	if ( !pd_users )
	{
		pr_debug("%s()\n", __func__);
		pd_users = 1;
		ret = 0;
	}
	spin_unlock_irqrestore(&pd_lock, flags);

	return ret;
}


static long pd_ioctl (struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct pd_vita49_unpack  unpack;
	struct pd_vita49_ts      ts;
	struct pd_access         access;
	unsigned long            reg;
	unsigned                 dev = cmd & 1;

	pr_trace("%s(cmd %x, arg %08lx)\n", __func__, cmd, arg);
	if ( _IOC_TYPE(cmd) != PD_IOCTL_MAGIC )
		return -EINVAL;

	switch ( cmd )
	{
		/* Access control IOCTLs */
		case PD_IOCG_ACCESS_AVAIL:
			return copy_to_user((void *)arg, &access, sizeof(access));

		case PD_IOCS_ACCESS_REQUEST: /* TODO: implement */
		case PD_IOCS_ACCESS_RELEASE:
			return 0;

		/* ADI2AXIS IOCTLs */
		case PD_IOCS_ADI2AXIS_0_CTRL:
		case PD_IOCS_ADI2AXIS_1_CTRL:
			if ( !adi2axis[dev] )
				return -ENODEV;

			pr_debug("PD_IOCS_ADI2AXIS_%d_CTRL %08lx\n", dev, arg);
			REG_WRITE(&adi2axis[dev]->ctrl, arg);
			return 0;


		case PD_IOCG_ADI2AXIS_0_CTRL:
		case PD_IOCG_ADI2AXIS_1_CTRL:
			if ( !adi2axis[dev] )
				return -ENODEV;

			reg = REG_READ(&adi2axis[dev]->ctrl);
			pr_debug("PD_IOCG_ADI2AXIS_%d_CTRL %08lx\n", dev, reg);
			return put_user(reg, (unsigned long *)arg);


		case PD_IOCG_ADI2AXIS_0_STAT:
		case PD_IOCG_ADI2AXIS_1_STAT:
			if ( !adi2axis[dev] )
				return -ENODEV;

			reg = REG_READ(&adi2axis[dev]->stat);
			pr_debug("PD_IOCG_ADI2AXIS_%d_STAT %08lx\n", dev, reg);
			return put_user(reg, (unsigned long *)arg);


		case PD_IOCS_ADI2AXIS_0_BYTES:
		case PD_IOCS_ADI2AXIS_1_BYTES:
			if ( !adi2axis[dev] )
				return -ENODEV;

			pr_debug("PD_IOCS_ADI2AXIS_%d_BYTES %08lx\n", dev, arg);
			REG_WRITE(&adi2axis[dev]->bytes, arg);
			return 0;


		case PD_IOCG_ADI2AXIS_0_BYTES:
		case PD_IOCG_ADI2AXIS_1_BYTES:
			if ( !adi2axis[dev] )
				return -ENODEV;

			reg = REG_READ(&adi2axis[dev]->bytes);
			pr_debug("PD_IOCG_ADI2AXIS_%d_BYTES %08lx\n", dev, reg);
			return put_user(reg, (unsigned long *)arg);



		/* VITA49 CLOCK IOCTLs */
		case PD_IOCS_VITA49_CLK_CTRL:
			if ( !vita49_clk )
				return -ENODEV;

			pr_debug("PD_IOCS_VITA49_CLK_CTRL %08lx\n", arg);
			REG_WRITE(&vita49_clk->ctrl, arg);
			return 0;


		case PD_IOCG_VITA49_CLK_CTRL:
			if ( !vita49_clk )
				return -ENODEV;

			reg = REG_READ(&vita49_clk->ctrl);
			pr_debug("PD_IOCG_VITA49_CLK_CTRL %08lx\n", reg);
			return put_user(reg, (unsigned long *)arg);


		case PD_IOCG_VITA49_CLK_STAT:
			if ( !vita49_clk )
				return -ENODEV;

			reg = REG_READ(&vita49_clk->stat);
			pr_debug("PD_IOCG_VITA49_CLK_STAT %08lx\n", reg);
			return put_user(reg, (unsigned long *)arg);


		case PD_IOCS_VITA49_CLK_TSI:
			if ( !vita49_clk )
				return -ENODEV;

			pr_debug("PD_IOCS_VITA49_CLK_TSI %08lx\n", arg);
			REG_WRITE(&vita49_clk->tsi_prog, arg);
			return 0;


		case PD_IOCG_VITA49_CLK_TSI:
			if ( !vita49_clk )
				return -ENODEV;

			reg = REG_READ(&vita49_clk->tsi_prog);
			pr_debug("PD_IOCG_VITA49_CLK_TSI %08lx\n", reg);
			return put_user(reg, (unsigned long *)arg);


		case PD_IOCG_VITA49_CLK_READ_1:
		case PD_IOCG_VITA49_CLK_READ_0:
			if ( !vita49_clk )
				return -ENODEV;

			// unsync'd read, so check for rollover during and retry if necessary
			do
			{
				if ( dev )
				{
					ts.tsf_lo = REG_READ(&vita49_clk->tsf_0_lo);
					ts.tsf_hi = REG_READ(&vita49_clk->tsf_0_hi);
					ts.tsi    = REG_READ(&vita49_clk->tsi_0);
					reg       = REG_READ(&vita49_clk->tsf_0_lo);
				}
				else
				{
					ts.tsf_lo = REG_READ(&vita49_clk->tsf_1_lo);
					ts.tsf_hi = REG_READ(&vita49_clk->tsf_1_hi);
					ts.tsi    = REG_READ(&vita49_clk->tsi_1);
					reg       = REG_READ(&vita49_clk->tsf_1_lo);
				}
			}
			while ( reg < ts.tsf_lo );
			pr_debug("PD_IOCG_VITA49_CLK_READ_%d %08lx.%08lx%08lx\n",
			         dev, ts.tsi, ts.tsf_hi, ts.tsf_lo);
			return copy_to_user((void *)arg, &ts, sizeof(ts));



		/* VITA49_PACK */
		case PD_IOCS_VITA49_PACK_0_CTRL:
		case PD_IOCS_VITA49_PACK_1_CTRL:
			if ( !vita49_pack[dev] )
				return -ENODEV;

			pr_debug("PD_IOCS_VITA49_PACK_%d_CTRL %08lx\n", dev, arg);
			REG_WRITE(&vita49_pack[dev]->ctrl, arg);
			return 0;


		case PD_IOCG_VITA49_PACK_0_CTRL:
		case PD_IOCG_VITA49_PACK_1_CTRL:
			if ( !vita49_pack[dev] )
				return -ENODEV;

			reg = REG_READ(&vita49_pack[dev]->ctrl);
			pr_debug("PD_IOCG_VITA49_PACK_%d_CTRL %08lx\n", dev, reg);
			return put_user(reg, (unsigned long *)arg);


		case PD_IOCG_VITA49_PACK_0_STAT:
		case PD_IOCG_VITA49_PACK_1_STAT:
			if ( !vita49_pack[dev] )
				return -ENODEV;

			reg = REG_READ(&vita49_pack[dev]->stat);
			pr_debug("PD_IOCG_VITA49_PACK_%d_STAT %08lx\n", dev, reg);
			return put_user(reg, (unsigned long *)arg);


		case PD_IOCS_VITA49_PACK_0_STRM_ID:
		case PD_IOCS_VITA49_PACK_1_STRM_ID:
			if ( !vita49_pack[dev] )
				return -ENODEV;

			pr_debug("PD_IOCS_VITA49_PACK_%d_STRM_ID %08lx\n", dev, arg);
			REG_WRITE(&vita49_pack[dev]->streamid, arg);
			return 0;


		case PD_IOCG_VITA49_PACK_0_STRM_ID:
		case PD_IOCG_VITA49_PACK_1_STRM_ID:
			if ( !vita49_pack[dev] )
				return -ENODEV;

			reg = REG_READ(&vita49_pack[dev]->streamid);
			pr_debug("PD_IOCG_VITA49_PACK_%d_STRM_ID %08lx\n", dev, reg);
			return put_user(reg, (unsigned long *)arg);


		case PD_IOCS_VITA49_PACK_0_PKT_SIZE:
		case PD_IOCS_VITA49_PACK_1_PKT_SIZE:
			if ( !vita49_pack[dev] )
				return -ENODEV;

			pr_debug("PD_IOCS_VITA49_PACK_%d_PKT_SIZE %08lx\n", dev, arg);
			REG_WRITE(&vita49_pack[dev]->pkt_size, arg);
			return 0;


		case PD_IOCG_VITA49_PACK_0_PKT_SIZE:
		case PD_IOCG_VITA49_PACK_1_PKT_SIZE:
			if ( !vita49_pack[dev] )
				return -ENODEV;

			reg = REG_READ(&vita49_pack[dev]->pkt_size);
			pr_debug("PD_IOCG_VITA49_PACK_%d_PKT_SIZE %08lx\n", dev, reg);
			return put_user(reg, (unsigned long *)arg);


		case PD_IOCS_VITA49_PACK_0_TRAILER:
		case PD_IOCS_VITA49_PACK_1_TRAILER:
			if ( !vita49_pack[dev] )
				return -ENODEV;

			pr_debug("PD_IOCS_VITA49_PACK_%d_TRAILER %08lx\n", dev, arg);
			REG_WRITE(&vita49_pack[dev]->trailer, arg);
			return 0;


		case PD_IOCG_VITA49_PACK_0_TRAILER:
		case PD_IOCG_VITA49_PACK_1_TRAILER:
			if ( !vita49_pack[dev] )
				return -ENODEV;

			reg = REG_READ(&vita49_pack[dev]->trailer);
			pr_debug("PD_IOCG_VITA49_PACK_%d_TRAILER %08lx\n", dev, reg);
			return put_user(reg, (unsigned long *)arg);



		/* VITA49_UNPACK IOCTLs */
		case PD_IOCS_VITA49_UNPACK_0_CTRL:
		case PD_IOCS_VITA49_UNPACK_1_CTRL:
			if ( !vita49_unpack[dev] )
				return -ENODEV;

			pr_debug("PD_IOCS_VITA49_UNPACK_%d_TRAILER %08lx\n", dev, arg);
			REG_WRITE(&vita49_pack[dev]->trailer, arg);
			return 0;


		case PD_IOCG_VITA49_UNPACK_0_CTRL:
		case PD_IOCG_VITA49_UNPACK_1_CTRL:
			if ( !vita49_unpack[dev] )
				return -ENODEV;

			reg = REG_READ(&vita49_unpack[dev]->ctrl);
			pr_debug("PD_IOCG_VITA49_UNPACK_%d_CTRL %08lx\n", dev, reg);
			return put_user(reg, (unsigned long *)arg);


		case PD_IOCG_VITA49_UNPACK_0_STAT:
		case PD_IOCG_VITA49_UNPACK_1_STAT:
			if ( !vita49_unpack[dev] )
				return -ENODEV;

			reg = REG_READ(&vita49_unpack[dev]->stat);
			pr_debug("PD_IOCG_VITA49_UNPACK_%d_STATE %08lx\n", dev, reg);
			return put_user(reg, (unsigned long *)arg);


		case PD_IOCS_VITA49_UNPACK_0_STRM_ID:
		case PD_IOCS_VITA49_UNPACK_1_STRM_ID:
			if ( !vita49_unpack[dev] )
				return -ENODEV;

			pr_debug("PD_IOCS_VITA49_UNPACK_%d_TRAILER %08lx\n", dev, arg);
			REG_WRITE(&vita49_pack[dev]->trailer, arg);
			return 0;


		case PD_IOCG_VITA49_UNPACK_0_STRM_ID:
		case PD_IOCG_VITA49_UNPACK_1_STRM_ID:
			if ( !vita49_unpack[dev] )
				return -ENODEV;

			reg = REG_READ(&vita49_unpack[dev]->streamid);
			pr_debug("PD_IOCG_VITA49_UNPACK_%d_STRM_ID %08lx\n", dev, reg);
			return put_user(reg, (unsigned long *)arg);


		case PD_IOCG_VITA49_UNPACK_0_COUNTS:
		case PD_IOCG_VITA49_UNPACK_1_COUNTS:
			if ( !vita49_unpack[dev] )
				return -ENODEV;

			unpack.pkt_rcv_cnt   = REG_READ(&vita49_unpack[dev]->pkt_rcv_cnt);
			unpack.pkt_drop_cnt  = REG_READ(&vita49_unpack[dev]->pkt_drop_cnt);
			unpack.pkt_size_err  = REG_READ(&vita49_unpack[dev]->pkt_size_err);
			unpack.pkt_type_err  = REG_READ(&vita49_unpack[dev]->pkt_type_err);
			unpack.pkt_order_err = REG_READ(&vita49_unpack[dev]->pkt_order_err);
			unpack.ts_order_err  = REG_READ(&vita49_unpack[dev]->ts_order_err);
			unpack.strm_id_err   = REG_READ(&vita49_unpack[dev]->strm_id_err);
			unpack.trailer_err   = REG_READ(&vita49_unpack[dev]->trailer_err);
			return copy_to_user((void *)arg, &unpack, sizeof(unpack));



		/* VITA49_ASSEM */
		case PD_IOCS_VITA49_ASSEM_0_CMD:
		case PD_IOCS_VITA49_ASSEM_1_CMD:
			if ( !vita49_assem[dev] )
				return -ENODEV;

			pr_debug("PD_IOCS_VITA49_ASSEM_%d_CMD %08lx\n", dev, arg);
			REG_WRITE(&vita49_assem[dev]->cmd, arg);
			return 0;


		case PD_IOCG_VITA49_ASSEM_0_CMD:
		case PD_IOCG_VITA49_ASSEM_1_CMD:
			if ( !vita49_assem[dev] )
				return -ENODEV;

			reg = REG_READ(&vita49_assem[dev]->cmd);
			pr_debug("PD_IOCG_VITA49_ASSEM_%d_CMD %08lx\n", dev, reg);
			return put_user(reg, (unsigned long *)arg);


		case PD_IOCS_VITA49_ASSEM_0_HDR_ERR:
		case PD_IOCS_VITA49_ASSEM_1_HDR_ERR:
			if ( !vita49_assem[dev] )
				return -ENODEV;

			pr_debug("PD_IOCS_VITA49_ASSEM_%d_HDR_ERR %08lx\n", dev, arg);
			REG_WRITE(&vita49_assem[dev]->hdr_err_cnt, arg);
			return 0;


		case PD_IOCG_VITA49_ASSEM_0_HDR_ERR:
		case PD_IOCG_VITA49_ASSEM_1_HDR_ERR:
			if ( !vita49_assem[dev] )
				return -ENODEV;

			reg = REG_READ(&vita49_assem[dev]->hdr_err_cnt);
			pr_debug("PD_IOCG_VITA49_ASSEM_%d_HDR_ERR %08lx\n", dev, reg);
			return put_user(reg, (unsigned long *)arg);



		/* VITA49_TRIGGER IOCTLs */
		case PD_IOCS_VITA49_TRIG_ADC_0_CTRL:
		case PD_IOCS_VITA49_TRIG_ADC_1_CTRL:
			if ( !vita49_trig_adc[dev] )
				return -ENODEV;

			pr_debug("PD_IOCS_VITA49_TRIG_ADC_%d_CTRL %08lx\n", dev, arg);
			REG_WRITE(&vita49_trig_adc[dev]->ctrl, arg);
			return 0;


		case PD_IOCG_VITA49_TRIG_ADC_0_CTRL:
		case PD_IOCG_VITA49_TRIG_ADC_1_CTRL:
			if ( !vita49_trig_adc[dev] )
				return -ENODEV;

			reg = REG_READ(&vita49_trig_adc[dev]->ctrl);
			pr_debug("PD_IOCG_VITA49_TRIG_ADC_%d_CTRL %08lx\n", dev, reg);
			return put_user(reg, (unsigned long *)arg);


		case PD_IOCG_VITA49_TRIG_ADC_0_STAT:
		case PD_IOCG_VITA49_TRIG_ADC_1_STAT:
			if ( !vita49_trig_adc[dev] )
				return -ENODEV;

			reg = REG_READ(&vita49_trig_adc[dev]->stat);
			pr_debug("PD_IOCG_VITA49_TRIG_ADC_%d_STAT %08lx\n", dev, reg);
			return put_user(reg, (unsigned long *)arg);


		case PD_IOCS_VITA49_TRIG_ADC_0_TS:
		case PD_IOCS_VITA49_TRIG_ADC_1_TS:
			if ( !vita49_trig_adc[dev] )
				return -ENODEV;

			if ( copy_from_user(&ts, (void *)arg, sizeof(ts)) )
				return -EFAULT;

			pr_debug("PD_IOCS_VITA49_TRIG_ADC_%d_TS %08lx.%08lx%08lx\n",
			         dev, ts.tsi, ts.tsf_hi, ts.tsf_lo);
			REG_WRITE(&vita49_trig_adc[dev]->tsf_lo_trig, ts.tsf_lo);
			REG_WRITE(&vita49_trig_adc[dev]->tsf_hi_trig, ts.tsf_hi);
			REG_WRITE(&vita49_trig_adc[dev]->tsi_trig,    ts.tsi);
			return copy_to_user((void *)arg, &ts, sizeof(ts));


		case PD_IOCG_VITA49_TRIG_ADC_0_TS:
		case PD_IOCG_VITA49_TRIG_ADC_1_TS:
			if ( !vita49_trig_adc[dev] )
				return -ENODEV;

			ts.tsf_lo = REG_READ(&vita49_trig_adc[dev]->tsf_lo_trig);
			ts.tsf_hi = REG_READ(&vita49_trig_adc[dev]->tsf_hi_trig);
			ts.tsi    = REG_READ(&vita49_trig_adc[dev]->tsi_trig);
			pr_debug("PD_IOCG_VITA49_TRIG_ADC_%d_TS %08lx.%08lx%08lx\n",
			         dev, ts.tsi, ts.tsf_hi, ts.tsf_lo);
			return copy_to_user((void *)arg, &ts, sizeof(ts));
			return copy_to_user((void *)arg, &ts, sizeof(ts));



		case PD_IOCS_VITA49_TRIG_DAC_0_CTRL:
		case PD_IOCS_VITA49_TRIG_DAC_1_CTRL:
			if ( !vita49_trig_dac[dev] )
				return -ENODEV;

			pr_debug("PD_IOCS_VITA49_TRIG_DAC_%d_CTRL %08lx\n", dev, arg);
			REG_WRITE(&vita49_trig_dac[dev]->ctrl, arg);
			return 0;


		case PD_IOCG_VITA49_TRIG_DAC_0_CTRL:
		case PD_IOCG_VITA49_TRIG_DAC_1_CTRL:
			if ( !vita49_trig_dac[dev] )
				return -ENODEV;

			reg = REG_READ(&vita49_trig_dac[dev]->ctrl);
			pr_debug("PD_IOCG_VITA49_TRIG_DAC_%d_CTRL %08lx\n", dev, reg);
			return put_user(reg, (unsigned long *)arg);


		case PD_IOCG_VITA49_TRIG_DAC_0_STAT:
		case PD_IOCG_VITA49_TRIG_DAC_1_STAT:
			if ( !vita49_trig_dac[dev] )
				return -ENODEV;

			reg = REG_READ(&vita49_trig_dac[dev]->stat);
			pr_debug("PD_IOCG_VITA49_TRIG_DAC_%d_STAT %08lx\n", dev, reg);
			return put_user(reg, (unsigned long *)arg);


		case PD_IOCS_VITA49_TRIG_DAC_0_TS:
		case PD_IOCS_VITA49_TRIG_DAC_1_TS:
			if ( !vita49_trig_dac[dev] )
				return -ENODEV;

			if ( copy_from_user(&ts, (void *)arg, sizeof(ts)) )
				return -EFAULT;

			pr_debug("PD_IOCS_VITA49_TRIG_DAC_%d_TS %08lx.%08lx%08lx\n",
			         dev, ts.tsi, ts.tsf_hi, ts.tsf_lo);
			REG_WRITE(&vita49_trig_dac[dev]->tsf_lo_trig, ts.tsf_lo);
			REG_WRITE(&vita49_trig_dac[dev]->tsf_hi_trig, ts.tsf_hi);
			REG_WRITE(&vita49_trig_dac[dev]->tsi_trig,    ts.tsi);
			return copy_to_user((void *)arg, &ts, sizeof(ts));


		case PD_IOCG_VITA49_TRIG_DAC_0_TS:
		case PD_IOCG_VITA49_TRIG_DAC_1_TS:
			if ( !vita49_trig_dac[dev] )
				return -ENODEV;

			ts.tsf_lo = REG_READ(&vita49_trig_dac[dev]->tsf_lo_trig);
			ts.tsf_hi = REG_READ(&vita49_trig_dac[dev]->tsf_hi_trig);
			ts.tsi    = REG_READ(&vita49_trig_dac[dev]->tsi_trig);
			pr_debug("PD_IOCG_VITA49_TRIG_DAC_%d_TS %08lx.%08lx%08lx\n",
			         dev, ts.tsi, ts.tsf_hi, ts.tsf_lo);
			return copy_to_user((void *)arg, &ts, sizeof(ts));
			return copy_to_user((void *)arg, &ts, sizeof(ts));




		/* SWRITE_PACK IOCTLs */
		case PD_IOCS_SWRITE_PACK_0_CMD:
		case PD_IOCS_SWRITE_PACK_1_CMD:
			if ( !swrite_pack[dev] )
				return -ENODEV;

			pr_debug("PD_IOCS_SWRITE_PACK_%d_CMD %08lx\n", dev, arg);
			REG_WRITE(&swrite_pack[dev]->cmd, arg);
			return 0;


		case PD_IOCG_SWRITE_PACK_0_CMD:
		case PD_IOCG_SWRITE_PACK_1_CMD:
			if ( !swrite_pack[dev] )
				return -ENODEV;

			reg = REG_READ(&swrite_pack[dev]->cmd);
			pr_debug("PD_IOCG_SWRITE_PACK_%d_CMD %08lx\n", dev, reg);
			return put_user(reg, (unsigned long *)arg);


		case PD_IOCS_SWRITE_PACK_0_ADDR:
		case PD_IOCS_SWRITE_PACK_1_ADDR:
			if ( !swrite_pack[dev] )
				return -ENODEV;

			pr_debug("PD_IOCS_SWRITE_PACK_%d_ADDR %08lx\n", dev, arg);
			REG_WRITE(&swrite_pack[dev]->cmd, arg);
			return 0;


		case PD_IOCG_SWRITE_PACK_0_ADDR:
		case PD_IOCG_SWRITE_PACK_1_ADDR:
			if ( !swrite_pack[dev] )
				return -ENODEV;

			reg = REG_READ(&swrite_pack[dev]->addr);
			pr_debug("PD_IOCG_SWRITE_PACK_%d_ADDR %08lx\n", dev, reg);
			return put_user(reg, (unsigned long *)arg);


		case PD_IOCS_SWRITE_PACK_0_SRCDEST:
		case PD_IOCS_SWRITE_PACK_1_SRCDEST:
			if ( !swrite_pack[dev] )
				return -ENODEV;

			pr_debug("PD_IOCS_SWRITE_PACK_%d_SRCDEST %08lx\n", dev, arg);
			REG_WRITE(&swrite_pack[dev]->cmd, arg);
			return 0;


		case PD_IOCG_SWRITE_PACK_0_SRCDEST:
		case PD_IOCG_SWRITE_PACK_1_SRCDEST:
			if ( !swrite_pack[dev] )
				return -ENODEV;

			reg = REG_READ(&swrite_pack[dev]->srcdest);
			pr_debug("PD_IOCG_SWRITE_PACK_%d_SRCDEST %08lx\n", dev, reg);
			return put_user(reg, (unsigned long *)arg);




		/* SWRITE_UNPACK IOCTLs */
		case PD_IOCS_SWRITE_UNPACK_CMD:
			if ( !swrite_unpack )
				return -ENODEV;

			pr_debug("PD_IOCS_SWRITE_UNPACK_CMD %08lx\n", arg);
			REG_WRITE(&swrite_unpack->cmd, arg);
			return 0;


		case PD_IOCG_SWRITE_UNPACK_CMD:
			if ( !swrite_unpack )
				return -ENODEV;

			reg = REG_READ(&swrite_unpack->cmd);
			pr_debug("PD_IOCG_SWRITE_UNPACK_CMD %08lx\n", reg);
			return put_user(reg, (unsigned long *)arg);


		case PD_IOCS_SWRITE_UNPACK_ADDR_0:
			if ( !swrite_unpack )
				return -ENODEV;

			pr_debug("PD_IOCS_SWRITE_UNPACK_ADDR_0 %08lx\n", arg);
			REG_WRITE(&swrite_unpack->addr_0, arg);
			return 0;


		case PD_IOCS_SWRITE_UNPACK_ADDR_1:
			if ( !swrite_unpack )
				return -ENODEV;

			pr_debug("PD_IOCS_SWRITE_UNPACK_ADDR_1 %08lx\n", arg);
			REG_WRITE(&swrite_unpack->addr_1, arg);
			return 0;


		case PD_IOCG_SWRITE_UNPACK_ADDR_0:
			if ( !swrite_unpack )
				return -ENODEV;

			reg = REG_READ(&swrite_unpack->addr_0);
			pr_debug("PD_IOCG_SWRITE_UNPACK_ADDR_0 %08lx\n", reg);
			return put_user(reg, (unsigned long *)arg);


		case PD_IOCG_SWRITE_UNPACK_ADDR_1:
			if ( !swrite_unpack )
				return -ENODEV;

			reg = REG_READ(&swrite_unpack->addr_1);
			pr_debug("PD_IOCG_SWRITE_UNPACK_ADDR_1 %08lx\n", reg);
			return put_user(reg, (unsigned long *)arg);



		/* ROUTING_REG IOCTLs */
		case PD_IOCS_ROUTING_REG_ADC_SW_DEST:
			if ( !routing_reg )
				return -ENODEV;

			pr_debug("PD_IOCS_ROUTING_REG_ADC_SW_DEST %08lx\n", arg);
			REG_WRITE(&routing_reg->adc_sw_dest, arg);
			return 0;


		case PD_IOCG_ROUTING_REG_ADC_SW_DEST:
			if ( !routing_reg )
				return -ENODEV;

			reg = REG_READ(&routing_reg->adc_sw_dest);
			pr_debug("PD_IOCG_ROUTING_REG_ADC_SW_DEST %08lx\n", reg);
			return put_user(reg, (unsigned long *)arg);
	}

	pr_err("%s(): unknown cmd %08x\n", __func__, cmd);
	return -EINVAL;
}


static int pd_release (struct inode *inode_p, struct file *file_p)
{
	unsigned long  flags;
	int            ret = -EBADF;

	spin_lock_irqsave(&pd_lock, flags);
	if ( pd_users )
	{
		pr_debug("%s()\n", __func__);
		pd_users = 0;
		ret = 0;
	}
	spin_unlock_irqrestore(&pd_lock, flags);

	return ret;
}


static struct file_operations fops = 
{
	open:           pd_open,
	unlocked_ioctl: pd_ioctl,
	release:        pd_release,
};

static struct miscdevice mdev = { MISC_DYNAMIC_MINOR, PD_DRIVER_NODE, &fops };



static int pd_remove (struct platform_device *pdev)
{
	misc_deregister(&mdev);

	return 0;
}

static void __iomem *pd_iomap (struct platform_device *pdev, char *name)
{
	struct resource *res;
	void __iomem    *ret;

	if ( !(res = platform_get_resource_byname(pdev, IORESOURCE_MEM, name)) )
	{
		dev_err(&pdev->dev, "%s: resource %s not defined, stop\n", __func__, name);
		return NULL;
	}

	ret = devm_ioremap_resource(&pdev->dev, res);
	if ( IS_ERR(ret) )
	{
		dev_err(&pdev->dev, "%s: resource %s ioremap fail, stop\n", __func__, name);
		return NULL;
	}

	pr_info("%s: %s -> start %08x -> mapped %p\n", dev_name(&pdev->dev),
	        name, res->start, ret);
	return ret;
}


static int pd_probe (struct platform_device *pdev)
{
	int  dev;
	int  ret = -ENXIO;

	if ( !(adi2axis[0] = pd_iomap(pdev, "adi2axis-0")) )
		goto fail;

	if ( !(adi2axis[1] = pd_iomap(pdev, "adi2axis-1")) )
		goto fail;

	if ( !(vita49_clk = pd_iomap(pdev, "vita49-clk")) )
		goto fail;

	if ( !(vita49_pack[0] = pd_iomap(pdev, "vita49-pack-0")) )
		goto fail;

	if ( !(vita49_pack[1] = pd_iomap(pdev, "vita49-pack-1")) )
		goto fail;

	if ( !(vita49_unpack[0] = pd_iomap(pdev, "vita49-unpack-0")) )
		goto fail;

	if ( !(vita49_unpack[1] = pd_iomap(pdev, "vita49-unpack-1")) )
		goto fail;

	if ( !(vita49_assem[0] = pd_iomap(pdev, "vita49-assem-0")) )
		goto fail;

	if ( !(vita49_assem[1] = pd_iomap(pdev, "vita49-assem-1")) )
		goto fail;

	if ( !(vita49_trig_adc[0] = pd_iomap(pdev, "vita49-trig-adc-0")) )
		goto fail;

	if ( !(vita49_trig_adc[1] = pd_iomap(pdev, "vita49-trig-adc-1")) )
		goto fail;

	if ( !(vita49_trig_dac[0] = pd_iomap(pdev, "vita49-trig-dac-0")) )
		goto fail;

	if ( !(vita49_trig_dac[1] = pd_iomap(pdev, "vita49-trig-dac-1")) )
		goto fail;

	if ( !(swrite_pack[0] = pd_iomap(pdev, "swrite-pack-0")) )
		goto fail;

	if ( !(swrite_pack[1] = pd_iomap(pdev, "swrite-pack-1")) )
		goto fail;

	if ( !(swrite_unpack = pd_iomap(pdev, "swrite-unpack")) )
		goto fail;

	if ( !(routing_reg = pd_iomap(pdev, "routing-reg")) )
		goto fail;

	if ( (ret = misc_register(&mdev)) < 0 )
	{
		pr_err("misc_register() failed\n");
		ret = -EIO;
		goto fail;
	}

	// initial setup: enable the pipeline for local passthru access
	REG_WRITE(&vita49_clk->ctrl, PD_VITA49_CLK_CTRL_RESET);
	REG_WRITE(&vita49_clk->ctrl, 0);
	for ( dev = 0; dev < 2; dev++ )
	{
		// reset blocks in the PL
		REG_WRITE(&adi2axis[dev]->ctrl,        PD_ADI2AXIS_CTRL_RESET);
		REG_WRITE(&vita49_pack[dev]->ctrl,     PD_VITA49_PACK_CTRL_RESET);
		REG_WRITE(&vita49_unpack[dev]->ctrl,   PD_VITA49_UNPACK_CTRL_RESET);
		REG_WRITE(&vita49_trig_dac[dev]->ctrl, PD_VITA49_TRIG_CTRL_RESET);
		REG_WRITE(&vita49_trig_adc[dev]->ctrl, PD_VITA49_TRIG_CTRL_RESET);
		REG_WRITE(&vita49_assem[dev]->cmd,     PD_VITA49_ASSEM_CMD_RESET);
		msleep(1);

		// relax reset
		REG_WRITE(&vita49_pack[dev]->ctrl,     0);
		REG_WRITE(&vita49_unpack[dev]->ctrl,   0);
		REG_WRITE(&vita49_trig_dac[dev]->ctrl, 0);
		REG_WRITE(&vita49_trig_adc[dev]->ctrl, 0);
		REG_WRITE(&vita49_assem[dev]->cmd,     0);
		msleep(1);

		// enable passthru
		REG_WRITE(&vita49_pack[dev]->ctrl,     PD_VITA49_PACK_CTRL_PASSTHRU);
		REG_WRITE(&vita49_unpack[dev]->ctrl,   PD_VITA49_UNPACK_CTRL_PASSTHRU);
		REG_WRITE(&vita49_trig_dac[dev]->ctrl, PD_VITA49_TRIG_CTRL_PASSTHRU);
		REG_WRITE(&vita49_trig_adc[dev]->ctrl, PD_VITA49_TRIG_CTRL_PASSTHRU);
		REG_WRITE(&vita49_assem[dev]->cmd,     PD_VITA49_ASSEM_CMD_PASSTHRU);
		REG_WRITE(&adi2axis[dev]->ctrl,        PD_ADI2AXIS_CTRL_LEGACY);
		REG_WRITE(&adi2axis[dev]->bytes,       8 << 20); // 8MB / 1MS
	}

	pr_info("registered successfully\n");
	return 0;

fail:
	if ( adi2axis[0]        )  devm_iounmap(&pdev->dev, adi2axis[0]);
	if ( adi2axis[1]        )  devm_iounmap(&pdev->dev, adi2axis[1]);
	if ( vita49_clk         )  devm_iounmap(&pdev->dev, vita49_clk);
	if ( vita49_pack[0]     )  devm_iounmap(&pdev->dev, vita49_pack[0]);
	if ( vita49_pack[1]     )  devm_iounmap(&pdev->dev, vita49_pack[1]);
	if ( vita49_unpack[0]   )  devm_iounmap(&pdev->dev, vita49_unpack[0]);
	if ( vita49_unpack[1]   )  devm_iounmap(&pdev->dev, vita49_unpack[1]);
	if ( vita49_assem[0]    )  devm_iounmap(&pdev->dev, vita49_assem[0]);
	if ( vita49_assem[1]    )  devm_iounmap(&pdev->dev, vita49_assem[1]);
	if ( vita49_trig_adc[0] )  devm_iounmap(&pdev->dev, vita49_trig_adc[0]);
	if ( vita49_trig_adc[1] )  devm_iounmap(&pdev->dev, vita49_trig_adc[1]);
	if ( vita49_trig_dac[0] )  devm_iounmap(&pdev->dev, vita49_trig_dac[0]);
	if ( vita49_trig_dac[1] )  devm_iounmap(&pdev->dev, vita49_trig_dac[1]);
	if ( swrite_pack[0]     )  devm_iounmap(&pdev->dev, swrite_pack[0]);
	if ( swrite_pack[1]     )  devm_iounmap(&pdev->dev, swrite_pack[1]);
	if ( swrite_unpack      )  devm_iounmap(&pdev->dev, swrite_unpack);
	if ( routing_reg        )  devm_iounmap(&pdev->dev, routing_reg);

	return ret;
}


static const struct of_device_id pd_ids[] = {
	{ .compatible = "sbt,pipe-dev" },
	{}
};

static struct platform_driver pd_driver = {
	.driver = {
		.name = "pipe-dev",
		.owner = THIS_MODULE,
		.of_match_table = pd_ids,
	},
	.probe  = pd_probe,
	.remove = pd_remove,
};

static int __init pd_init (void)
{
	return platform_driver_register(&pd_driver);
}
module_init(pd_init);

static void __exit pd_exit (void)
{
	platform_driver_unregister(&pd_driver);
}
module_exit(pd_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Morgan Hughes <morgan.hughes@silver-bullet-tech.com>");
MODULE_DESCRIPTION("SBT sample pipeline device driver");

