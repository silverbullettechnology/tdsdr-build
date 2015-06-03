/** \file      pd_regs.h
 *  \brief     Register definitions
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
#ifndef _INCLUDE_PD_REGS_H
#define _INCLUDE_PD_REGS_H
#include <linux/kernel.h>


/* ADI2AXIS - This block converts 64-bit data coming from the AD9361 module into
 * AXI-Streaming format.
 *
 * There are two modes in which this block can run in:
 * 1) legacy mode which transfers BYTES of data once a trigger signal is recieved
 * 2) trigger controlled mode which transfers data starting from when trigger is enabled
 * until the trigger signal is disabled.
 *
 * When enabled, the block will start transferring data when an external trigger signal is
 * enabled.  No AXI stream data is generated when the block is not enabled.  */
struct pd_adi2axis_regs
{
	uint32_t  ctrl;   /* 0x00  RW  one of PD_ADI2AXIS_CTRL_* */
	uint32_t  bytes;  /* 0x04  RW  number of bytes of data to transfer once module is
	                               triggered (ignored in trigger controlled mode) */
	uint32_t  stat;   /* 0x08  R   bitmap of PD_ADI2AXIS_STAT_* */
};

/* VITA49 CLOCK - This block generates two different output reference clocks.  The integer
 * part of the clock is common to both references and is incremented by an external 1pps
 * signal.  The fractional part of the clock is incremented by the sample rate clock
 * coming from each of the ADI chips.  The output clock value is used as a timestamp for
 * triggering the data streams. */
struct pd_vita49_clk_regs
{
	uint32_t  ctrl;      /* 0x00  RW  bitmap of PD_VITA49_CLK_CTRL_*           */
	uint32_t  stat;      /* 0x04  R   ???                                      */
	uint32_t  tsi_prog;  /* 0x08  RW  integer seconds counter value to program */
	uint32_t  tsi_0;     /* 0x0C  R   channel 0 integer counter                */
	uint32_t  tsf_0_hi;  /* 0x10  R   channel 0 msw of fractional counter      */
	uint32_t  tsf_0_lo;  /* 0x14  R   channel 0 lsw of fractional counter      */
	uint32_t  tsi_1;     /* 0x0C  R   channel 1 integer counter                */
	uint32_t  tsf_1_hi;  /* 0x10  R   channel 1 msw of fractional counter      */
	uint32_t  tsf_1_lo;  /* 0x14  R   channel 1 lsw of fractional counter      */
};

/* VITA49_PACK - This module packages the incoming data stream into a VITA49 format.  The
 * user is able to program a packet size to package the incoming data to.  An incoming
 * TLAST signal designates the end of the data stream.
 *
 * A output TLAST signal is generated that end of each packet.  If the data stream ends in
 * the middle of a partially completed packet, the module will fill the remaining data
 * with zeros up to the packet size.
 *
 * In addition to the packet header, the module transmits the following fields:  Stream
 * ID, Integer-seconds timestamp, Fractional-seconds timestamp.  The module is optionally
 * able to transmit a trailer word.  In pass-through mode, the incoming AXI-S stream is
 * passed immediately with a single clock cycle of delay. */
struct pd_vita49_pack_regs
{
	uint32_t  ctrl;      /* 0x00  RW  bitmap of PD_VITA49_PACK_CTRL_*      */
	uint32_t  stat;      /* 0x04  R   ???                                  */
	uint32_t  streamid;  /* 0x08  RW  stream id value                      */
	uint32_t  pkt_size;  /* 0x0C  RW  vita49 packet size (in 32-bit words) */
	uint32_t  trailer;   /* 0x10  RW  32-bit word trailer                  */
};


/* VITA49_UNPACK - This module unpacks incoming VITA49 data packets into a data stream to
 * be transmitted.  The module assumes correctly formatted VITA49 packets and also packets
 * are received in order.  The trailer feature for VITA49 packet is not supported.
 *
 * Counters are available to count the number of packets recieved, the number dropped, and
 * the number of errors detected.  There is an error counter for each type of error
 * detected. The following errors are detected:
 *
 * PKT_SIZE_ERR:  The packet size specified in the packet header does not match the number
 *                of words in the actual packet (as marked by TLAST signal).  When
 *                detected, the error counter is incremented, but the full packet payload
 *                is transmitted up to TLAST.
 * PKT_TYPE_ERR:  Only type 0001 packets are support.  Any other packet types are dropped.
 * PKT_ORDER_ERR: The Packet Count field in the header must count from 0000 and increment
 *                on consecutive packets.  If this is not the case, error counter is
 *                incremented but payload is transmitted.
 * TS_ORDER_ERR:  Timestamp of the current packet is earlier than the previous packet.  If
 *                this is not the case, error counter is incremented but payload is
 *                transmitted.
 * STRM_ID_ERR:   The stream id of vita packet does not match the expected value.  Packet
 *                is dropped.
 * TRAILER_ERR:   Trailer field is of viat packet is not supported.  Packet payload is
 *                transmitted.
 *
 * The module must be reset to clear the counters.  In pass-through mode, the incoming
 * AXI-S stream is passed immediately with a single clock cycle of delay. */
struct pd_vita49_unpack_regs
{
	uint32_t  ctrl;           /* 0x00  RW  bitmap of PD_VITA49_UNPACK_CTRL_*         */
	uint32_t  stat;           /* 0x04  R   ???                                       */
	uint32_t  streamid;       /* 0x08  RW  stream id value                           */
	uint32_t  pkt_rcv_cnt;    /* 0x0C  R   packets received counter                  */
	uint32_t  pkt_drop_cnt;   /* 0x10  R   packets dropped counter                   */
	uint32_t  pkt_size_err;   /* 0x14  R   incorrect packet size counter             */
	uint32_t  pkt_type_err;   /* 0x18  R   unsupport VITA49 packet type counter      */
	uint32_t  pkt_order_err;  /* 0x1C  R   incorrect packet number order counter     */
	uint32_t  ts_order_err;   /* 0x20  R   incorrect packet timestamp order counter  */
	uint32_t  strm_id_err;    /* 0x24  R   non-matching stream id counter            */
	uint32_t  trailer_err;    /* 0x28  R   packet with trailer (unsupported) counter */
};

/* VITA49_ASSEM - Takes a look at incoming data stream and regenerates TLAST signal based
 * on VITA49 header.  Packets with invalid headers are discarded and a counter increments.
 */
struct pd_vita49_assem_regs
{
	uint32_t cmd;          /* 0x00 RW bitmap of PD_VITA49_ASSEM_CMD_*       */
	uint32_t hdr_err_cnt;  /* 0x04 RW counts number of packet header errors */
};

/* VITA49_TRIGGER - This block pauses the incoming AXI-S data stream until the programmed
 * triggered time is reached by the vita49 clock module.  There are two programmable
 * trigger times: one to turn on the data stream and one to turn off.  In pass-through
 * mode, the incoming AXI-S stream is passed immediately with a single clock cycle of
 * delay. */
struct pd_vita49_trig_regs
{
	uint32_t  ctrl;         /* 0x00  RW  bitmap of PD_VITA49_TRIG_CTRL_*        */
	uint32_t  stat;         /* 0x04  R   ???                                    */
	uint32_t  tsi_trig;     /* 0x08  RW  integer clock trigger value            */
	uint32_t  tsf_hi_trig;  /* 0x0C  RW  msw of fractional clock trigger value  */
	uint32_t  tsf_lo_trig;  /* 0x10  RW  lsw of fractional clock trigger value  */
};

/* SRIO_SWRITE_PACK - Takes the incoming data stream and packages into SRIO Type 6
 * (SWRITE) packets in HELLO format. */
struct pd_swrite_pack_regs
{
	uint32_t  cmd;      /* 0x00  RW  bitmap of PD_SRIO_SWRITE_PACK_CMD_*               */
	uint32_t  addr;     /* 0x04  RW  address in target device to write to              */
	uint32_t  srcdest;  /* 0x08  RW  source and destination device id for SRIO routing */
};

/* SRIO_SWRITE_UNPACK - Takes incoming SRIO Type 6 packets (SWRITE) removes the header and
 * transfers the payload.  TDEST is generated based on the address field of SRIO HELLO
 * header.  The SRIO address refers to the ad9361 branch in TD-SDR board and may or may
 * not be the same as the VITA stream ID value. */
struct pd_swrite_unpack_regs
{
	uint32_t  cmd;     /* 0x00  RW  bitmap of PD_SRIO_SWRITE_UNPACK_CTRL_START  */
	uint32_t  addr_0;  /* 0x04  RW  streamID of data to be sent to AD9361_0     */
	uint32_t  addr_1;  /* 0x08  RW  streamID of data to be sent to AD9361_1     */
};


/* ROUTING_REG */
struct pd_routing_reg
{
	uint32_t  adc_sw_dest;  /* 0x0  RW  bitmap of PD_ROUTING_REG_* */
};


#define REG_WRITE(addr, val) \
	do{ \
		pr_debug("REG_WRITE(%p, %08lx)\n", addr, (unsigned long)val); \
		iowrite32(val, addr); \
	}while(0)
#define REG_READ(addr)        (ioread32(addr))

#define REG_RMW(addr, clr, set) \
	do{ \
		uint32_t reg = ioread32(addr); \
		reg &= ~(clr); \
		reg |= (set); \
		iowrite32(reg, (addr)); \
	}while(0)


#endif // _INCLUDE_PD_REGS_H
