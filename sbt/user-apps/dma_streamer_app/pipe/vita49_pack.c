/** \file      pipe/vita49_pack.c
 *  \brief     implementation of VITA49_PACK IOCTLs
 *  \copyright Copyright 2013,2014 Silver Bullet Technology
 *
 *             Licensed under the Apache License, Version 2.0 (the "License"); you may not
 *             use this file except in compliance with the License.  You may obtain a copy
 *             of the License at: 
 *               http://www.apache.org/licenses/LICENSE-2.0
 *
 *             Unless required by applicable law or agreed to in writing, software
 *             distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 *             WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 *             License for the specific language governing permissions and limitations
 *             under the License.
 *
 * vim:ts=4:noexpandtab
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>

#include <pd_main.h>

#include "pipe/dev.h"
#include "pipe/private.h"
#include "pipe/vita49_pack.h"


static int VITA49_PACK_S_CTRL[2]     = { PD_IOCS_VITA49_PACK_0_CTRL,     PD_IOCS_VITA49_PACK_1_CTRL     };
static int VITA49_PACK_G_CTRL[2]     = { PD_IOCG_VITA49_PACK_0_CTRL,     PD_IOCG_VITA49_PACK_1_CTRL     };
static int VITA49_PACK_G_STAT[2]     = { PD_IOCG_VITA49_PACK_0_STAT,     PD_IOCG_VITA49_PACK_1_STAT     };
static int VITA49_PACK_S_STRM_ID[2]  = { PD_IOCS_VITA49_PACK_0_STRM_ID,  PD_IOCS_VITA49_PACK_1_STRM_ID  };
static int VITA49_PACK_G_STRM_ID[2]  = { PD_IOCG_VITA49_PACK_0_STRM_ID,  PD_IOCG_VITA49_PACK_1_STRM_ID  };
static int VITA49_PACK_S_PKT_SIZE[2] = { PD_IOCS_VITA49_PACK_0_PKT_SIZE, PD_IOCS_VITA49_PACK_1_PKT_SIZE };
static int VITA49_PACK_G_PKT_SIZE[2] = { PD_IOCG_VITA49_PACK_0_PKT_SIZE, PD_IOCG_VITA49_PACK_1_PKT_SIZE };
static int VITA49_PACK_S_TRAILER[2]  = { PD_IOCS_VITA49_PACK_0_TRAILER,  PD_IOCS_VITA49_PACK_1_TRAILER  };
static int VITA49_PACK_G_TRAILER[2]  = { PD_IOCG_VITA49_PACK_0_TRAILER,  PD_IOCG_VITA49_PACK_1_TRAILER  };


int pipe_vita49_pack_set_ctrl (int dev, unsigned long reg)
{
	int ret;

	if ( (ret = ioctl(pipe_dev_fd, VITA49_PACK_S_CTRL[dev], reg)) )
		printf("VITA49_PACK_S_CTRL[%d], %08x: %d: %s\n", dev, reg, ret, strerror(errno));

	return ret;
}

int pipe_vita49_pack_get_ctrl (int dev, unsigned long *reg)
{
	int ret;

	if ( (ret = ioctl(pipe_dev_fd, VITA49_PACK_G_CTRL[dev], reg)) )
		printf("VITA49_PACK_G_CTRL[%d]: %d: %s\n", dev, ret, strerror(errno));

	return ret;
}

int pipe_vita49_pack_get_stat (int dev, unsigned long *reg)
{
	int ret;

	if ( (ret = ioctl(pipe_dev_fd, VITA49_PACK_G_STAT[dev], reg)) )
		printf("VITA49_PACK_G_STAT[%d]: %d: %s\n", dev, ret, strerror(errno));

	return ret;
}

int pipe_vita49_pack_set_strm_id (int dev, unsigned long reg)
{
	int ret;

	if ( (ret = ioctl(pipe_dev_fd, VITA49_PACK_S_STRM_ID[dev], reg)) )
		printf("VITA49_PACK_S_STRM_ID[%d], %08x: %d: %s\n", dev, reg, ret, strerror(errno));

	return ret;
}

int pipe_vita49_pack_get_strm_id (int dev, unsigned long *reg)
{
	int ret;

	if ( (ret = ioctl(pipe_dev_fd, VITA49_PACK_G_STRM_ID[dev], reg)) )
		printf("VITA49_PACK_G_STRM_ID[%d]: %d: %s\n", dev, ret, strerror(errno));

	return ret;
}

int pipe_vita49_pack_set_pkt_size (int dev, unsigned long reg)
{
	int ret;

	if ( (ret = ioctl(pipe_dev_fd, VITA49_PACK_S_PKT_SIZE[dev], reg)) )
		printf("VITA49_PACK_S_PKT_SIZE[%d], %08x: %d: %s\n", dev, reg, ret, strerror(errno));

	return ret;
}

int pipe_vita49_pack_get_pkt_size (int dev, unsigned long *reg)
{
	int ret;

	if ( (ret = ioctl(pipe_dev_fd, VITA49_PACK_G_PKT_SIZE[dev], reg)) )
		printf("VITA49_PACK_G_PKT_SIZE[%d]: %d: %s\n", dev, ret, strerror(errno));

	return ret;
}

int pipe_vita49_pack_set_trailer (int dev, unsigned long reg)
{
	int ret;

	if ( (ret = ioctl(pipe_dev_fd, VITA49_PACK_S_TRAILER[dev], reg)) )
		printf("VITA49_PACK_S_TRAILER[%d], %08x: %d: %s\n", dev, reg, ret, strerror(errno));

	return ret;
}

int pipe_vita49_pack_get_trailer (int dev, unsigned long *reg)
{
	int ret;

	if ( (ret = ioctl(pipe_dev_fd, VITA49_PACK_G_TRAILER[dev], reg)) )
		printf("VITA49_PACK_G_TRAILER[%d]: %d: %s\n", dev, ret, strerror(errno));

	return ret;
}

