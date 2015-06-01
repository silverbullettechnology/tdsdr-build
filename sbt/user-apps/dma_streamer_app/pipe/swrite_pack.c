/** \file      pipe/swrite_pack.c
 *  \brief     implementation of data source/sink controls
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
#include "pipe/swrite_pack.h"


static int SWRITE_PACK_S_CMD[2]     = { PD_IOCS_SWRITE_PACK_0_CMD,     PD_IOCS_SWRITE_PACK_1_CMD     };
static int SWRITE_PACK_G_CMD[2]     = { PD_IOCG_SWRITE_PACK_0_CMD,     PD_IOCG_SWRITE_PACK_1_CMD     };
static int SWRITE_PACK_S_ADDR[2]    = { PD_IOCS_SWRITE_PACK_0_ADDR,    PD_IOCS_SWRITE_PACK_1_ADDR    };
static int SWRITE_PACK_G_ADDR[2]    = { PD_IOCG_SWRITE_PACK_0_ADDR,    PD_IOCG_SWRITE_PACK_1_ADDR    };
static int SWRITE_PACK_S_SRCDEST[2] = { PD_IOCS_SWRITE_PACK_0_SRCDEST, PD_IOCS_SWRITE_PACK_1_SRCDEST };
static int SWRITE_PACK_G_SRCDEST[2] = { PD_IOCG_SWRITE_PACK_0_SRCDEST, PD_IOCG_SWRITE_PACK_1_SRCDEST };


int pipe_swrite_pack_set_cmd (int dev, unsigned long reg)
{
	int ret;

	if ( (ret = ioctl(pipe_dev_fd, SWRITE_PACK_S_CMD[dev], reg)) )
		printf("SWRITE_PACK_S_CMD[%d], %08x: %d: %s\n", dev, reg, ret, strerror(errno));

	return ret;
}

int pipe_swrite_pack_get_cmd (int dev, unsigned long *reg)
{
	int ret;

	if ( (ret = ioctl(pipe_dev_fd, SWRITE_PACK_G_CMD[dev], reg)) )
		printf("SWRITE_PACK_G_CMD[%d]: %d: %s\n", dev, ret, strerror(errno));

	return ret;
}

int pipe_swrite_pack_set_addr (int dev, unsigned long reg)
{
	int ret;

	if ( (ret = ioctl(pipe_dev_fd, SWRITE_PACK_S_ADDR[dev], reg)) )
		printf("SWRITE_PACK_S_ADDR[%d], %08x: %d: %s\n", dev, reg, ret, strerror(errno));

	return ret;
}

int pipe_swrite_pack_get_addr (int dev, unsigned long *reg)
{
	int ret;

	if ( (ret = ioctl(pipe_dev_fd, SWRITE_PACK_G_ADDR[dev], reg)) )
		printf("SWRITE_PACK_G_ADDR[%d]: %d: %s\n", dev, ret, strerror(errno));

	return ret;
}

int pipe_swrite_pack_set_srcdest (int dev, unsigned long reg)
{
	int ret;

	if ( (ret = ioctl(pipe_dev_fd, SWRITE_PACK_S_SRCDEST[dev], reg)) )
		printf("SWRITE_PACK_S_SRCDEST[%d], %08x: %d: %s\n", dev, reg, ret, strerror(errno));

	return ret;
}

int pipe_swrite_pack_get_srcdest (int dev, unsigned long *reg)
{
	int ret;

	if ( (ret = ioctl(pipe_dev_fd, SWRITE_PACK_G_SRCDEST[dev], reg)) )
		printf("SWRITE_PACK_G_SRCDEST[%d]: %d: %s\n", dev, ret, strerror(errno));

	return ret;
}

