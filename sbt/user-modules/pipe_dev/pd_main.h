/** \file      pd_main.h
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
#ifndef _INCLUDE_PD_MAIN_H
#define _INCLUDE_PD_MAIN_H


#define PD_DRIVER_NODE "pipe_dev"
#define PD_IOCTL_MAGIC 1


/* ADI2AXIS bits */
#define PD_ADI2AXIS_CTRL_RESET     0x00000000 /* reset module            */
#define PD_ADI2AXIS_CTRL_LEGACY    0x00000001 /* enable legacy mode      */
#define PD_ADI2AXIS_CTRL_TRIGGER   0x00000002 /* trigger controlled mode */
#define PD_ADI2AXIS_STAT_ENB_TRIG  0x00000001 /* module is both enabled and triggered */
#define PD_ADI2AXIS_STAT_COMPLETE  0x00000002 /* data transfer is complete */

/* VITA49 CLOCK bits */
#define PD_VITA49_CLK_CTRL_ENABLE   0x00000001 /* enable clock                */
#define PD_VITA49_CLK_CTRL_RESET    0x00000002 /* reset clock                 */
#define PD_VITA49_CLK_CTRL_SET_INT  0x00000004 /* set integer seconds counter */

/* VITA49_PACK bits */
#define PD_VITA49_PACK_CTRL_ENABLE    0x00000001 /* enable module       */
#define PD_VITA49_PACK_CTRL_RESET     0x00000002 /* reset module        */
#define PD_VITA49_PACK_CTRL_PASSTHRU  0x00000004 /* enable pass-through */
#define PD_VITA49_PACK_CTRL_TRAILER   0x00000008 /* enable trailer value */

/* VITA49_UNPACK bits */
#define PD_VITA49_UNPACK_CTRL_ENABLE    0x00000001  /* enable module       */
#define PD_VITA49_UNPACK_CTRL_RESET     0x00000002  /* reset module        */
#define PD_VITA49_UNPACK_CTRL_PASSTHRU  0x00000004  /* enable pass-through */
#define PD_VITA49_UNPACK_CTRL_TRAILER   0x00000008  /* enable trailer      */

/* VITA49_ASSEM bits */
#define PD_VITA49_ASSEM_CMD_ENABLE    0x00000001  /* enable module       */
#define PD_VITA49_ASSEM_CMD_RESET     0x00000002  /* reset module        */
#define PD_VITA49_ASSEM_CMD_PASSTHRU  0x00000004  /* enable pass-through */

/* VITA49_TRIGGER bits */
#define PD_VITA49_TRIG_CTRL_ENABLE    0x00000001  /* enable clock        */
#define PD_VITA49_TRIG_CTRL_RESET     0x00000002  /* reset module        */
#define PD_VITA49_TRIG_CTRL_TRIG_ON   0x00000004  /* set on trigger      */
#define PD_VITA49_TRIG_CTRL_TRIG_OFF  0x00000008  /* set off trigger     */
#define PD_VITA49_TRIG_CTRL_PASSTHRU  0x00000010  /* enable pass-through */

/* SWRITE_PACK bits */
#define PD_SWRITE_PACK_CMD_START  0x00000001  /* start module */
#define PD_SWRITE_PACK_CMD_RESET  0x00000002  /* reset module */

/* SWRITE_UNPACK bits */
#define PD_SWRITE_UNPACK_CTRL_START  0x00000001  /* start module       */
#define PD_SWRITE_UNPACK_CTRL_RESET  0x00000002  /* reset module        */

/* ROUTING_REG bits */
#define PD_ROUTING_REG_ADC_SW_DEST_0  0x00000001  /* route to SRIO */
#define PD_ROUTING_REG_ADC_SW_DEST_1  0x00000002  /* route to SRIO */

/* Access control bits */
#define PD_ACCESS_AD1_RX  0x00000001  /* RX path on AD1 */
#define PD_ACCESS_AD2_RX  0x00000002  /* RX path on AD2 */
#define PD_ACCESS_AD1_TX  0x00000004  /* TX path on AD1 */
#define PD_ACCESS_AD2_TX  0x00000008  /* TX path on AD2 */
#define PD_ACCESS_MASK    0x0000000F  /* Bitmask for all bits */

/* Integer + Fractional timestamp struct */
struct pd_vita49_ts
{
	unsigned long  tsi;     /* integer counter                */
	unsigned long  tsf_hi;  /* msw of fractional counter      */
	unsigned long  tsf_lo;  /* lsw of fractional counter      */
};

/* Unpack counters struct */
struct pd_vita49_unpack
{
	unsigned long  pkt_rcv_cnt;    /* packets received counter                  */
	unsigned long  pkt_drop_cnt;   /* packets dropped counter                   */
	unsigned long  pkt_size_err;   /* incorrect packet size counter             */
	unsigned long  pkt_type_err;   /* unsupport VITA49 packet type counter      */
	unsigned long  pkt_order_err;  /* incorrect packet number order counter     */
	unsigned long  ts_order_err;   /* incorrect packet timestamp order counter  */
	unsigned long  strm_id_err;    /* non-matching stream id counter            */
	unsigned long  trailer_err;    /* packet with trailer (unsupported) counter */
};


/* Note: IOCTL numbers are chosen so the LSB selects AD1/AD2 path and pd_ioctl() depends
 * on this.  Use care if changing the numbers below.  Check for mismatches by piping the
 * #define lines through a filter like this:
 *   grep _0 | rev | cut -d, -f2- | rev | grep '[13579]$'
 *   grep _1 | rev | cut -d, -f2- | rev | grep '[02468]$'
 */

/* Access control IOCTLs */
#define  PD_IOCG_ACCESS_AVAIL    _IOR(PD_IOCTL_MAGIC, 0, unsigned long *)
#define  PD_IOCS_ACCESS_REQUEST  _IOW(PD_IOCTL_MAGIC, 1, unsigned long)
#define  PD_IOCS_ACCESS_RELEASE  _IOW(PD_IOCTL_MAGIC, 2, unsigned long)

/* ADI2AXIS IOCTLs */
#define  PD_IOCS_ADI2AXIS_0_CTRL   _IOW(PD_IOCTL_MAGIC, 10, unsigned long)
#define  PD_IOCS_ADI2AXIS_1_CTRL   _IOW(PD_IOCTL_MAGIC, 11, unsigned long)
#define  PD_IOCG_ADI2AXIS_0_CTRL   _IOR(PD_IOCTL_MAGIC, 12, unsigned long *)
#define  PD_IOCG_ADI2AXIS_1_CTRL   _IOR(PD_IOCTL_MAGIC, 13, unsigned long *)
#define  PD_IOCG_ADI2AXIS_0_STAT   _IOR(PD_IOCTL_MAGIC, 14, unsigned long *)
#define  PD_IOCG_ADI2AXIS_1_STAT   _IOR(PD_IOCTL_MAGIC, 15, unsigned long *)
#define  PD_IOCS_ADI2AXIS_0_BYTES  _IOW(PD_IOCTL_MAGIC, 16, unsigned long)
#define  PD_IOCS_ADI2AXIS_1_BYTES  _IOW(PD_IOCTL_MAGIC, 17, unsigned long)
#define  PD_IOCG_ADI2AXIS_0_BYTES  _IOR(PD_IOCTL_MAGIC, 18, unsigned long *)
#define  PD_IOCG_ADI2AXIS_1_BYTES  _IOR(PD_IOCTL_MAGIC, 19, unsigned long *)

/* VITA49 CLOCK IOCTLs */
#define  PD_IOCS_VITA49_CLK_CTRL    _IOW(PD_IOCTL_MAGIC, 30, unsigned long)
#define  PD_IOCG_VITA49_CLK_CTRL    _IOR(PD_IOCTL_MAGIC, 31, unsigned long *)
#define  PD_IOCG_VITA49_CLK_STAT    _IOR(PD_IOCTL_MAGIC, 34, unsigned long *)
#define  PD_IOCS_VITA49_CLK_TSI     _IOW(PD_IOCTL_MAGIC, 35, unsigned long)
#define  PD_IOCG_VITA49_CLK_TSI     _IOR(PD_IOCTL_MAGIC, 36, unsigned long *)
#define  PD_IOCG_VITA49_CLK_READ_1  _IOR(PD_IOCTL_MAGIC, 37, struct pd_vita49_ts *)
#define  PD_IOCG_VITA49_CLK_READ_0  _IOR(PD_IOCTL_MAGIC, 38, struct pd_vita49_ts *)

/* VITA49_PACK IOCTLs */
#define  PD_IOCS_VITA49_PACK_0_CTRL      _IOW(PD_IOCTL_MAGIC, 40, unsigned long)
#define  PD_IOCS_VITA49_PACK_1_CTRL      _IOW(PD_IOCTL_MAGIC, 41, unsigned long)
#define  PD_IOCG_VITA49_PACK_0_CTRL      _IOR(PD_IOCTL_MAGIC, 42, unsigned long *)
#define  PD_IOCG_VITA49_PACK_1_CTRL      _IOR(PD_IOCTL_MAGIC, 43, unsigned long *)
#define  PD_IOCG_VITA49_PACK_0_STAT      _IOR(PD_IOCTL_MAGIC, 44, unsigned long *)
#define  PD_IOCG_VITA49_PACK_1_STAT      _IOR(PD_IOCTL_MAGIC, 45, unsigned long *)
#define  PD_IOCS_VITA49_PACK_0_STRM_ID   _IOW(PD_IOCTL_MAGIC, 46, unsigned long)
#define  PD_IOCS_VITA49_PACK_1_STRM_ID   _IOW(PD_IOCTL_MAGIC, 47, unsigned long)
#define  PD_IOCG_VITA49_PACK_0_STRM_ID   _IOR(PD_IOCTL_MAGIC, 48, unsigned long *)
#define  PD_IOCG_VITA49_PACK_1_STRM_ID   _IOR(PD_IOCTL_MAGIC, 49, unsigned long *)
#define  PD_IOCS_VITA49_PACK_0_PKT_SIZE  _IOW(PD_IOCTL_MAGIC, 50, unsigned long)
#define  PD_IOCS_VITA49_PACK_1_PKT_SIZE  _IOW(PD_IOCTL_MAGIC, 51, unsigned long)
#define  PD_IOCG_VITA49_PACK_0_PKT_SIZE  _IOR(PD_IOCTL_MAGIC, 52, unsigned long *)
#define  PD_IOCG_VITA49_PACK_1_PKT_SIZE  _IOR(PD_IOCTL_MAGIC, 53, unsigned long *)
#define  PD_IOCS_VITA49_PACK_0_TRAILER   _IOW(PD_IOCTL_MAGIC, 54, unsigned long)
#define  PD_IOCS_VITA49_PACK_1_TRAILER   _IOW(PD_IOCTL_MAGIC, 55, unsigned long)
#define  PD_IOCG_VITA49_PACK_0_TRAILER   _IOR(PD_IOCTL_MAGIC, 56, unsigned long *)
#define  PD_IOCG_VITA49_PACK_1_TRAILER   _IOR(PD_IOCTL_MAGIC, 57, unsigned long *)

/* VITA49_UNPACK IOCTLs */
#define  PD_IOCS_VITA49_UNPACK_0_CTRL     _IOW(PD_IOCTL_MAGIC, 60, unsigned long)
#define  PD_IOCS_VITA49_UNPACK_1_CTRL     _IOW(PD_IOCTL_MAGIC, 61, unsigned long)
#define  PD_IOCG_VITA49_UNPACK_0_CTRL     _IOR(PD_IOCTL_MAGIC, 62, unsigned long *)
#define  PD_IOCG_VITA49_UNPACK_1_CTRL     _IOR(PD_IOCTL_MAGIC, 63, unsigned long *)
#define  PD_IOCG_VITA49_UNPACK_0_STAT     _IOR(PD_IOCTL_MAGIC, 64, unsigned long *)
#define  PD_IOCG_VITA49_UNPACK_1_STAT     _IOR(PD_IOCTL_MAGIC, 65, unsigned long *)
#define  PD_IOCS_VITA49_UNPACK_0_STRM_ID  _IOW(PD_IOCTL_MAGIC, 66, unsigned long)
#define  PD_IOCS_VITA49_UNPACK_1_STRM_ID  _IOW(PD_IOCTL_MAGIC, 67, unsigned long)
#define  PD_IOCG_VITA49_UNPACK_0_STRM_ID  _IOR(PD_IOCTL_MAGIC, 68, unsigned long *)
#define  PD_IOCG_VITA49_UNPACK_1_STRM_ID  _IOR(PD_IOCTL_MAGIC, 69, unsigned long *)
#define  PD_IOCG_VITA49_UNPACK_0_COUNTS   _IOR(PD_IOCTL_MAGIC, 70, struct pd_vita49_unpack *)
#define  PD_IOCG_VITA49_UNPACK_1_COUNTS   _IOR(PD_IOCTL_MAGIC, 71, struct pd_vita49_unpack *)

/* VITA49_ASSEM IOCTLs */
#define  PD_IOCS_VITA49_ASSEM_0_CMD      _IOW(PD_IOCTL_MAGIC, 80, unsigned long)
#define  PD_IOCS_VITA49_ASSEM_1_CMD      _IOW(PD_IOCTL_MAGIC, 81, unsigned long)
#define  PD_IOCG_VITA49_ASSEM_0_CMD      _IOR(PD_IOCTL_MAGIC, 82, unsigned long *)
#define  PD_IOCG_VITA49_ASSEM_1_CMD      _IOR(PD_IOCTL_MAGIC, 83, unsigned long *)
#define  PD_IOCS_VITA49_ASSEM_0_HDR_ERR  _IOW(PD_IOCTL_MAGIC, 84, unsigned long)
#define  PD_IOCS_VITA49_ASSEM_1_HDR_ERR  _IOW(PD_IOCTL_MAGIC, 85, unsigned long)
#define  PD_IOCG_VITA49_ASSEM_0_HDR_ERR  _IOR(PD_IOCTL_MAGIC, 86, unsigned long *)
#define  PD_IOCG_VITA49_ASSEM_1_HDR_ERR  _IOR(PD_IOCTL_MAGIC, 87, unsigned long *)

/* VITA49_TRIGGER IOCTLs */
#define  PD_IOCS_VITA49_TRIG_ADC_0_CTRL  _IOW(PD_IOCTL_MAGIC, 90, unsigned long)
#define  PD_IOCS_VITA49_TRIG_ADC_1_CTRL  _IOW(PD_IOCTL_MAGIC, 91, unsigned long)
#define  PD_IOCG_VITA49_TRIG_ADC_0_CTRL  _IOR(PD_IOCTL_MAGIC, 92, unsigned long *)
#define  PD_IOCG_VITA49_TRIG_ADC_1_CTRL  _IOR(PD_IOCTL_MAGIC, 93, unsigned long *)
#define  PD_IOCG_VITA49_TRIG_ADC_0_STAT  _IOR(PD_IOCTL_MAGIC, 94, unsigned long *)
#define  PD_IOCG_VITA49_TRIG_ADC_1_STAT  _IOR(PD_IOCTL_MAGIC, 95, unsigned long *)
#define  PD_IOCS_VITA49_TRIG_ADC_0_TS    _IOW(PD_IOCTL_MAGIC, 96, struct pd_vita49_ts *)
#define  PD_IOCS_VITA49_TRIG_ADC_1_TS    _IOW(PD_IOCTL_MAGIC, 97, struct pd_vita49_ts *)
#define  PD_IOCG_VITA49_TRIG_ADC_0_TS    _IOR(PD_IOCTL_MAGIC, 98, struct pd_vita49_ts *)
#define  PD_IOCG_VITA49_TRIG_ADC_1_TS    _IOR(PD_IOCTL_MAGIC, 99, struct pd_vita49_ts *)

#define  PD_IOCS_VITA49_TRIG_DAC_0_CTRL  _IOW(PD_IOCTL_MAGIC, 110, unsigned long)
#define  PD_IOCS_VITA49_TRIG_DAC_1_CTRL  _IOW(PD_IOCTL_MAGIC, 111, unsigned long)
#define  PD_IOCG_VITA49_TRIG_DAC_0_CTRL  _IOR(PD_IOCTL_MAGIC, 112, unsigned long *)
#define  PD_IOCG_VITA49_TRIG_DAC_1_CTRL  _IOR(PD_IOCTL_MAGIC, 113, unsigned long *)
#define  PD_IOCG_VITA49_TRIG_DAC_0_STAT  _IOR(PD_IOCTL_MAGIC, 114, unsigned long *)
#define  PD_IOCG_VITA49_TRIG_DAC_1_STAT  _IOR(PD_IOCTL_MAGIC, 115, unsigned long *)
#define  PD_IOCS_VITA49_TRIG_DAC_0_TS    _IOW(PD_IOCTL_MAGIC, 116, struct pd_vita49_ts *)
#define  PD_IOCS_VITA49_TRIG_DAC_1_TS    _IOW(PD_IOCTL_MAGIC, 117, struct pd_vita49_ts *)
#define  PD_IOCG_VITA49_TRIG_DAC_0_TS    _IOR(PD_IOCTL_MAGIC, 118, struct pd_vita49_ts *)
#define  PD_IOCG_VITA49_TRIG_DAC_1_TS    _IOR(PD_IOCTL_MAGIC, 119, struct pd_vita49_ts *)

/* SWRITE_PACK IOCTLs */
#define  PD_IOCS_SWRITE_PACK_0_CMD      _IOW(PD_IOCTL_MAGIC, 130, unsigned long)
#define  PD_IOCS_SWRITE_PACK_1_CMD      _IOW(PD_IOCTL_MAGIC, 131, unsigned long)
#define  PD_IOCG_SWRITE_PACK_0_CMD      _IOR(PD_IOCTL_MAGIC, 132, unsigned long *)
#define  PD_IOCG_SWRITE_PACK_1_CMD      _IOR(PD_IOCTL_MAGIC, 133, unsigned long *)
#define  PD_IOCS_SWRITE_PACK_0_ADDR     _IOW(PD_IOCTL_MAGIC, 134, unsigned long)
#define  PD_IOCS_SWRITE_PACK_1_ADDR     _IOW(PD_IOCTL_MAGIC, 135, unsigned long)
#define  PD_IOCG_SWRITE_PACK_0_ADDR     _IOR(PD_IOCTL_MAGIC, 136, unsigned long *)
#define  PD_IOCG_SWRITE_PACK_1_ADDR     _IOR(PD_IOCTL_MAGIC, 137, unsigned long *)
#define  PD_IOCS_SWRITE_PACK_0_SRCDEST  _IOW(PD_IOCTL_MAGIC, 138, unsigned long)
#define  PD_IOCS_SWRITE_PACK_1_SRCDEST  _IOW(PD_IOCTL_MAGIC, 139, unsigned long)
#define  PD_IOCG_SWRITE_PACK_0_SRCDEST  _IOR(PD_IOCTL_MAGIC, 140, unsigned long *)
#define  PD_IOCG_SWRITE_PACK_1_SRCDEST  _IOR(PD_IOCTL_MAGIC, 141, unsigned long *)

/* SWRITE_UNPACK IOCTLs */
#define  PD_IOCS_SWRITE_UNPACK_CMD     _IOW(PD_IOCTL_MAGIC, 150, unsigned long)
#define  PD_IOCG_SWRITE_UNPACK_CMD     _IOR(PD_IOCTL_MAGIC, 151, unsigned long *)
#define  PD_IOCS_SWRITE_UNPACK_ADDR_0  _IOW(PD_IOCTL_MAGIC, 152, unsigned long)
#define  PD_IOCS_SWRITE_UNPACK_ADDR_1  _IOW(PD_IOCTL_MAGIC, 153, unsigned long)
#define  PD_IOCG_SWRITE_UNPACK_ADDR_0  _IOR(PD_IOCTL_MAGIC, 154, unsigned long *)
#define  PD_IOCG_SWRITE_UNPACK_ADDR_1  _IOR(PD_IOCTL_MAGIC, 155, unsigned long *)

/* ROUTING_REG IOCTLs */
#define  PD_IOCS_ROUTING_REG_ADC_SW_DEST  _IOW(PD_IOCTL_MAGIC, 160, unsigned long)
#define  PD_IOCG_ROUTING_REG_ADC_SW_DEST  _IOR(PD_IOCTL_MAGIC, 161, unsigned long *)


#endif /* _INCLUDE_PD_MAIN_H */
/* Ends    : pd_main.h */
