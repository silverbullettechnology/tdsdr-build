/** \file      src/lib/vita49_unpack.c
 *  \brief     implementation of VITA49_UNPACK IOCTLs
 *  \copyright Copyright 2015 Silver Bullet Technology
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

#include "pipe_dev.h"
#include "private.h"


static int VITA49_UNPACK_S_CTRL[2]    = { PD_IOCS_VITA49_UNPACK_0_CTRL,    PD_IOCS_VITA49_UNPACK_1_CTRL    };
static int VITA49_UNPACK_G_CTRL[2]    = { PD_IOCG_VITA49_UNPACK_0_CTRL,    PD_IOCG_VITA49_UNPACK_1_CTRL    };
static int VITA49_UNPACK_G_STAT[2]    = { PD_IOCG_VITA49_UNPACK_0_STAT,    PD_IOCG_VITA49_UNPACK_1_STAT    };
static int VITA49_UNPACK_G_RCVD[2]    = { PD_IOCG_VITA49_UNPACK_0_RCVD,    PD_IOCG_VITA49_UNPACK_1_RCVD    };
static int VITA49_UNPACK_S_STRM_ID[2] = { PD_IOCS_VITA49_UNPACK_0_STRM_ID, PD_IOCS_VITA49_UNPACK_1_STRM_ID };
static int VITA49_UNPACK_G_STRM_ID[2] = { PD_IOCG_VITA49_UNPACK_0_STRM_ID, PD_IOCG_VITA49_UNPACK_1_STRM_ID };
static int VITA49_UNPACK_G_COUNTS[2]  = { PD_IOCG_VITA49_UNPACK_0_COUNTS,  PD_IOCG_VITA49_UNPACK_1_COUNTS  };


int pipe_vita49_unpack_set_ctrl (int dev, unsigned long reg)
{
	int ret;

	if ( (ret = ioctl(pipe_dev_fd, VITA49_UNPACK_S_CTRL[dev], reg)) )
		printf("VITA49_UNPACK_S_CTRL[%d], %08x: %d: %s\n", dev, reg, ret, strerror(errno));

	return ret;
}

int pipe_vita49_unpack_get_ctrl (int dev, unsigned long *reg)
{
	int ret;

	if ( (ret = ioctl(pipe_dev_fd, VITA49_UNPACK_G_CTRL[dev], reg)) )
		printf("VITA49_UNPACK_G_CTRL[%d]: %d: %s\n", dev, ret, strerror(errno));

	return ret;
}

int pipe_vita49_unpack_get_stat (int dev, unsigned long *reg)
{
	int ret;

	if ( (ret = ioctl(pipe_dev_fd, VITA49_UNPACK_G_STAT[dev], reg)) )
		printf("VITA49_UNPACK_G_STAT[%d]: %d: %s\n", dev, ret, strerror(errno));

	return ret;
}

int pipe_vita49_unpack_get_rcvd (int dev, unsigned long *reg)
{
	int ret;

	if ( (ret = ioctl(pipe_dev_fd, VITA49_UNPACK_G_RCVD[dev], reg)) )
		printf("VITA49_UNPACK_G_RCVD[%d]: %d: %s\n", dev, ret, strerror(errno));

	return ret;
}

int pipe_vita49_unpack_set_strm_id (int dev, unsigned long reg)
{
	int ret;

	if ( (ret = ioctl(pipe_dev_fd, VITA49_UNPACK_S_STRM_ID[dev], reg)) )
		printf("VITA49_UNPACK_S_STRM_ID[%d], %08x: %d: %s\n", dev, reg, ret, strerror(errno));

	return ret;
}

int pipe_vita49_unpack_get_strm_id (int dev, unsigned long *reg)
{
	int ret;

	if ( (ret = ioctl(pipe_dev_fd, VITA49_UNPACK_G_STRM_ID[dev], reg)) )
		printf("VITA49_UNPACK_G_STRM_ID[%d]: %d: %s\n", dev, ret, strerror(errno));

	return ret;
}

int pipe_vita49_unpack_get_counts (int dev, struct pd_vita49_unpack *buf)
{
	int ret;

	if ( (ret = ioctl(pipe_dev_fd, VITA49_UNPACK_G_COUNTS[dev], buf)) )
		printf("VITA49_UNPACK_G_COUNTS[%d]: %d: %s\n", dev, ret, strerror(errno));

	return ret;
}

