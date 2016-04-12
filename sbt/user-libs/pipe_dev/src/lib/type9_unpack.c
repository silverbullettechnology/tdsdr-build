/** \file      src/lib/type9_unpack.c
 *  \brief     implementation of TYPE9_UNPACK IOCTLs
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


static int TYPE9_UNPACK_S_STRMID[2] = { PD_IOCS_TYPE9_UNPACK_STRMID_0, PD_IOCS_TYPE9_UNPACK_STRMID_1 };
static int TYPE9_UNPACK_G_STRMID[2] = { PD_IOCG_TYPE9_UNPACK_STRMID_0, PD_IOCG_TYPE9_UNPACK_STRMID_1 };


int pipe_type9_unpack_set_cmd (unsigned long reg)
{
	int ret;

	if ( (ret = ioctl(pipe_dev_fd, PD_IOCS_TYPE9_UNPACK_CMD, reg)) )
		ERROR("PD_IOCS_TYPE9_UNPACK_CMD[%d], %08x: %d: %s\n", reg, ret, strerror(errno));

	return ret;
}

int pipe_type9_unpack_get_cmd (unsigned long *reg)
{
	int ret;

	if ( (ret = ioctl(pipe_dev_fd, PD_IOCG_TYPE9_UNPACK_CMD, reg)) )
		ERROR("PD_IOCG_TYPE9_UNPACK_CMD[%d]: %d: %s\n", ret, strerror(errno));

	return ret;
}

int pipe_type9_unpack_set_strmid (int dev, unsigned long reg)
{
	int ret;

	if ( (ret = ioctl(pipe_dev_fd, TYPE9_UNPACK_S_STRMID[dev], reg)) )
		ERROR("TYPE9_UNPACK_S_STRMID[%d], %08x: %d: %s\n", dev, reg, ret, strerror(errno));

	return ret;
}

int pipe_type9_unpack_get_strmid (int dev, unsigned long *reg)
{
	int ret;

	if ( (ret = ioctl(pipe_dev_fd, TYPE9_UNPACK_G_STRMID[dev], reg)) )
		ERROR("TYPE9_UNPACK_G_STRMID[%d]: %d: %s\n", dev, ret, strerror(errno));

	return ret;
}

