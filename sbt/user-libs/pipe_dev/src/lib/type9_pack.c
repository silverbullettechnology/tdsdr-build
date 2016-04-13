/** \file      src/lib/type9_pack.c
 *  \brief     implementation of TYPE9_PACK IOCTLs
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

#include "pipe_dev.h"
#include "private.h"


static int TYPE9_PACK_S_CMD[2]     = { PD_IOCS_TYPE9_PACK_0_CMD,     PD_IOCS_TYPE9_PACK_1_CMD     };
static int TYPE9_PACK_G_CMD[2]     = { PD_IOCG_TYPE9_PACK_0_CMD,     PD_IOCG_TYPE9_PACK_1_CMD     };
static int TYPE9_PACK_S_LENGTH[2]  = { PD_IOCS_TYPE9_PACK_0_LENGTH,  PD_IOCS_TYPE9_PACK_1_LENGTH  };
static int TYPE9_PACK_G_LENGTH[2]  = { PD_IOCG_TYPE9_PACK_0_LENGTH,  PD_IOCG_TYPE9_PACK_1_LENGTH  };
static int TYPE9_PACK_S_STRMID[2]  = { PD_IOCS_TYPE9_PACK_0_STRMID,  PD_IOCS_TYPE9_PACK_1_STRMID  };
static int TYPE9_PACK_G_STRMID[2]  = { PD_IOCG_TYPE9_PACK_0_STRMID,  PD_IOCG_TYPE9_PACK_1_STRMID  };
static int TYPE9_PACK_S_SRCDEST[2] = { PD_IOCS_TYPE9_PACK_0_SRCDEST, PD_IOCS_TYPE9_PACK_1_SRCDEST };
static int TYPE9_PACK_G_SRCDEST[2] = { PD_IOCG_TYPE9_PACK_0_SRCDEST, PD_IOCG_TYPE9_PACK_1_SRCDEST };
static int TYPE9_PACK_S_COS[2]     = { PD_IOCS_TYPE9_PACK_0_COS,     PD_IOCS_TYPE9_PACK_1_COS     };
static int TYPE9_PACK_G_COS[2]     = { PD_IOCG_TYPE9_PACK_0_COS,     PD_IOCG_TYPE9_PACK_1_COS     };


int pipe_type9_pack_set_cmd (int dev, unsigned long reg)
{
	int ret;

	if ( (ret = ioctl(pipe_dev_fd, TYPE9_PACK_S_CMD[dev], reg)) )
		ERROR("TYPE9_PACK_S_CMD[%d], %08x: %d: %s\n", dev, reg, ret, strerror(errno));

	return ret;
}

int pipe_type9_pack_get_cmd (int dev, unsigned long *reg)
{
	int ret;

	if ( (ret = ioctl(pipe_dev_fd, TYPE9_PACK_G_CMD[dev], reg)) )
		ERROR("TYPE9_PACK_G_CMD[%d]: %d: %s\n", dev, ret, strerror(errno));

	return ret;
}

int pipe_type9_pack_set_length (int dev, unsigned long reg)
{
	int ret;

	if ( (ret = ioctl(pipe_dev_fd, TYPE9_PACK_S_LENGTH[dev], reg)) )
		ERROR("TYPE9_PACK_S_LENGTH[%d], %08x: %d: %s\n", dev, reg, ret, strerror(errno));

	return ret;
}

int pipe_type9_pack_get_length (int dev, unsigned long *reg)
{
	int ret;

	if ( (ret = ioctl(pipe_dev_fd, TYPE9_PACK_G_LENGTH[dev], reg)) )
		ERROR("TYPE9_PACK_G_LENGTH[%d]: %d: %s\n", dev, ret, strerror(errno));

	return ret;
}

int pipe_type9_pack_set_strmid (int dev, unsigned long reg)
{
	int ret;

	if ( (ret = ioctl(pipe_dev_fd, TYPE9_PACK_S_STRMID[dev], reg)) )
		ERROR("TYPE9_PACK_S_STRMID[%d], %08x: %d: %s\n", dev, reg, ret, strerror(errno));

	return ret;
}

int pipe_type9_pack_get_strmid (int dev, unsigned long *reg)
{
	int ret;

	if ( (ret = ioctl(pipe_dev_fd, TYPE9_PACK_G_STRMID[dev], reg)) )
		ERROR("TYPE9_PACK_G_STRMID[%d]: %d: %s\n", dev, ret, strerror(errno));

	return ret;
}

int pipe_type9_pack_set_srcdest (int dev, unsigned long reg)
{
	int ret;

	if ( (ret = ioctl(pipe_dev_fd, TYPE9_PACK_S_SRCDEST[dev], reg)) )
		ERROR("TYPE9_PACK_S_SRCDEST[%d], %08x: %d: %s\n", dev, reg, ret, strerror(errno));

	return ret;
}

int pipe_type9_pack_get_srcdest (int dev, unsigned long *reg)
{
	int ret;

	if ( (ret = ioctl(pipe_dev_fd, TYPE9_PACK_G_SRCDEST[dev], reg)) )
		ERROR("TYPE9_PACK_G_SRCDEST[%d]: %d: %s\n", dev, ret, strerror(errno));

	return ret;
}

int pipe_type9_pack_set_cos (int dev, unsigned long reg)
{
	int ret;

	if ( (ret = ioctl(pipe_dev_fd, TYPE9_PACK_S_COS[dev], reg)) )
		ERROR("TYPE9_PACK_S_COS[%d], %08x: %d: %s\n", dev, reg, ret, strerror(errno));

	return ret;
}

int pipe_type9_pack_get_cos (int dev, unsigned long *reg)
{
	int ret;

	if ( (ret = ioctl(pipe_dev_fd, TYPE9_PACK_G_COS[dev], reg)) )
		ERROR("TYPE9_PACK_G_COS[%d]: %d: %s\n", dev, ret, strerror(errno));

	return ret;
}

