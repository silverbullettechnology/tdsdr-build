/** \file      src/lib/swrite_unpack.c
 *  \brief     implementation of SWRITE_UNPACK IOCTLs
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


static int SWRITE_UNPACK_S_ADDR[2] = { PD_IOCS_SWRITE_UNPACK_ADDR_0, PD_IOCS_SWRITE_UNPACK_ADDR_1 };
static int SWRITE_UNPACK_G_ADDR[2] = { PD_IOCG_SWRITE_UNPACK_ADDR_0, PD_IOCG_SWRITE_UNPACK_ADDR_1 };


int pipe_swrite_unpack_set_cmd (unsigned long reg)
{
	int ret;

	if ( (ret = ioctl(pipe_dev_fd, PD_IOCS_SWRITE_UNPACK_CMD, reg)) )
		printf("PD_IOCS_SWRITE_UNPACK_CMD[%d], %08x: %d: %s\n", reg, ret, strerror(errno));

	return ret;
}

int pipe_swrite_unpack_get_cmd (unsigned long *reg)
{
	int ret;

	if ( (ret = ioctl(pipe_dev_fd, PD_IOCG_SWRITE_UNPACK_CMD, reg)) )
		printf("PD_IOCG_SWRITE_UNPACK_CMD[%d]: %d: %s\n", ret, strerror(errno));

	return ret;
}

int pipe_swrite_unpack_set_addr (int dev, unsigned long reg)
{
	int ret;

	if ( (ret = ioctl(pipe_dev_fd, SWRITE_UNPACK_S_ADDR[dev], reg)) )
		printf("SWRITE_UNPACK_S_ADDR[%d], %08x: %d: %s\n", dev, reg, ret, strerror(errno));

	return ret;
}

int pipe_swrite_unpack_get_addr (int dev, unsigned long *reg)
{
	int ret;

	if ( (ret = ioctl(pipe_dev_fd, SWRITE_UNPACK_G_ADDR[dev], reg)) )
		printf("SWRITE_UNPACK_G_ADDR[%d]: %d: %s\n", dev, ret, strerror(errno));

	return ret;
}

