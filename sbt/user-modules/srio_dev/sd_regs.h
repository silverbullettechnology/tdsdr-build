/** \file      sd_regs.h
 *  \brief     
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
#ifndef _INCLUDE_SD_REGS_H_
#define _INCLUDE_SD_REGS_H_
#include <linux/kernel.h>


/* Need to put this somewhere better */
struct sd_srio_config
{
	resource_size_t  maint;
	resource_size_t  sys_regs;
};


/* CAR: Capability Address Registers (base +0x0) */
struct sd_car_regs
{
	uint32_t  dev_id;    /* 0x00: Device Identity */
	uint32_t  dev_info;  /* 0x04: Device Information */
	uint32_t  asm_id;    /* 0x08: Assembly Identity */
	uint32_t  asm_info;  /* 0x0C: Assembly Information */
	uint32_t  pef;       /* 0x10: Processing Element Features */
	uint32_t  swp_info;  /* 0x14: Switch Port Information */
	uint32_t  src_ops;   /* 0x18: Source Operations */
	uint32_t  dst_ops;   /* 0x1c: Destination Operations */
};

/* CSR: Command and Status Registers (base +0x40) */
struct sd_csr_regs
{
	uint32_t  rsvd1[2];       /* 0x40: Reserved (..0x48) */
	uint32_t  pell_ctrl;      /* 0x4c: PE Logical layer Control */
	uint32_t  rsvd2[2];       /* 0x50: Reserved (..0x54) */
	uint32_t  lcs0_baseaddr;  /* 0x58: Local Configuration Space 0 Base Address */
	uint32_t  lcs1_baseaddr;  /* 0x5c: Local Configuration Space 1 Base Address */
	uint32_t  base_did;       /* 0x60: Base Device ID */
	uint32_t  rsvd3;          /* 0x64: Reserved */
	uint32_t  host_did_lock;  /* 0x68: Host Base Device ID Lock */
	uint32_t  component_tag;  /* 0x6c: Component Tag */
};

/* LPS-EF: LP-Serial Extended Features Registers (base +0x100) */
struct sd_lps_ef_regs
{
	uint32_t  efb_header;      /* 0x100: Extended Features Block Header */
	uint32_t  rsvd1[7];        /* 0x104: Reserved (..0x11C) */
	uint32_t  port_link_tout;  /* 0x120: Port Link Timeout */
	uint32_t  port_resp_tout;  /* 0x124: Port Response Timeout */
	uint32_t  rsvd2[5];        /* 0x128: Reserved (..0x138) */
	uint32_t  port_gen_ctl;    /* 0x13c: General Control */
	uint32_t  port_n_mnt_req;  /* 0x140: Port n Link Maintenance Request */
	uint32_t  port_n_mnt_res;  /* 0x144: Port n Maintenance Response */
	uint32_t  port_n_ackid;    /* 0x148: Port n Local Ack ID */
	uint32_t  rsvd3[2];        /* 0x14C: Reserved (..0x150) */
	uint32_t  port_n_ctl_2;    /* 0x154: Port n Control 2 */
	uint32_t  port_n_err_sts;  /* 0x158: Port n Error and Status */
	uint32_t  port_n_ctl;      /* 0x15c: Port n Control */
};

/* LPSL-EF: LP-Serial Lane Extended Features Registers (base +0x400) */
struct sd_lpsl_ef_regs
{
	uint32_t  efb_header;      /* 0x400: Extended Features Block Header */
	uint32_t  rsvd[3];         /* 0x404: Reserved (..0x40C) */
	struct
	{
		uint32_t  status0;     /* 0x410: Status 0 */
		uint32_t  status1;     /* 0x414: Status 1 */
		uint32_t  rsvd[3];     /* 0x418: Reserved (..0x42C) */
	}
	lane[4];  /* 4 lanes: 0x410, 0x430, 0x450, 0x470 */
};


#endif /* _INCLUDE_SD_REGS_H_ */
