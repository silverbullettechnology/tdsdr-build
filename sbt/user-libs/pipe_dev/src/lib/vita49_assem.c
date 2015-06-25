/** \file      src/lib/vita49_assem.c
 *  \brief     implementation of VITA49_ASSEM IOCTLs
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


static int VITA49_ASSEM_S_CMD[2]     = { PD_IOCS_VITA49_ASSEM_0_CMD,     PD_IOCS_VITA49_ASSEM_1_CMD     };
static int VITA49_ASSEM_G_CMD[2]     = { PD_IOCG_VITA49_ASSEM_0_CMD,     PD_IOCG_VITA49_ASSEM_1_CMD     };
static int VITA49_ASSEM_S_HDR_ERR[2] = { PD_IOCS_VITA49_ASSEM_0_HDR_ERR, PD_IOCS_VITA49_ASSEM_1_HDR_ERR };
static int VITA49_ASSEM_G_HDR_ERR[2] = { PD_IOCG_VITA49_ASSEM_0_HDR_ERR, PD_IOCG_VITA49_ASSEM_1_HDR_ERR };


int pipe_vita49_assem_set_cmd (int dev, unsigned long reg)
{
	int ret;

	if ( (ret = ioctl(pipe_dev_fd, VITA49_ASSEM_S_CMD[dev], reg)) )
		ERROR("VITA49_ASSEM_S_CMD[%d], %08x: %d: %s\n", dev, reg, ret, strerror(errno));

	return ret;
}

int pipe_vita49_assem_get_cmd (int dev, unsigned long *reg)
{
	int ret;

	if ( (ret = ioctl(pipe_dev_fd, VITA49_ASSEM_G_CMD[dev], reg)) )
		ERROR("VITA49_ASSEM_G_CMD[%d]: %d: %s\n", dev, ret, strerror(errno));

	return ret;
}

int pipe_vita49_assem_set_hdr_err (int dev, unsigned long reg)
{
	int ret;

	if ( (ret = ioctl(pipe_dev_fd, VITA49_ASSEM_S_HDR_ERR[dev], reg)) )
		ERROR("VITA49_ASSEM_S_HDR_ERR[%d], %08x: %d: %s\n", dev, reg, ret, strerror(errno));

	return ret;
}

int pipe_vita49_assem_get_hdr_err (int dev, unsigned long *reg)
{
	int ret;

	if ( (ret = ioctl(pipe_dev_fd, VITA49_ASSEM_G_HDR_ERR[dev], reg)) )
		ERROR("VITA49_ASSEM_G_HDR_ERR[%d]: %d: %s\n", dev, ret, strerror(errno));

	return ret;
}

