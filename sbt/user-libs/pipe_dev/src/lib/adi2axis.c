/** \file      pipe/adi2axis.c
 *  \brief     implementation of ADI2AXIS IOCTLs
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


static int ADI2AXIS_S_CTRL[2]  = { PD_IOCS_ADI2AXIS_0_CTRL,  PD_IOCS_ADI2AXIS_1_CTRL  };
static int ADI2AXIS_G_CTRL[2]  = { PD_IOCG_ADI2AXIS_0_CTRL,  PD_IOCG_ADI2AXIS_1_CTRL  };
static int ADI2AXIS_G_STAT[2]  = { PD_IOCG_ADI2AXIS_0_STAT,  PD_IOCG_ADI2AXIS_1_STAT  };
static int ADI2AXIS_S_BYTES[2] = { PD_IOCS_ADI2AXIS_0_BYTES, PD_IOCS_ADI2AXIS_1_BYTES };
static int ADI2AXIS_G_BYTES[2] = { PD_IOCG_ADI2AXIS_0_BYTES, PD_IOCG_ADI2AXIS_1_BYTES };


int pipe_adi2axis_set_ctrl (int dev, unsigned long reg)
{
	int ret;

	if ( (ret = ioctl(pipe_dev_fd, ADI2AXIS_S_CTRL[dev], reg)) )
		printf("ADI2AXIS_S_CTRL[%d], %08x: %d: %s\n", dev, reg, ret, strerror(errno));

	return ret;
}

int pipe_adi2axis_get_ctrl (int dev, unsigned long *reg)
{
	int ret;

	if ( (ret = ioctl(pipe_dev_fd, ADI2AXIS_G_CTRL[dev], reg)) )
		printf("ADI2AXIS_G_CTRL[%d]: %d: %s\n", dev, ret, strerror(errno));

	return ret;
}

int pipe_adi2axis_get_stat (int dev, unsigned long *reg)
{
	int ret;

	if ( (ret = ioctl(pipe_dev_fd, ADI2AXIS_G_STAT[dev], reg)) )
		printf("ADI2AXIS_G_STAT[%d], %08x: %d: %s\n", dev, reg, ret, strerror(errno));

	return ret;
}

int pipe_adi2axis_set_bytes (int dev, unsigned long reg)
{
	int ret;

	if ( (ret = ioctl(pipe_dev_fd, ADI2AXIS_S_BYTES[dev], reg)) )
		printf("ADI2AXIS_S_BYTES[%d]: %d: %s\n", dev, ret, strerror(errno));

	return ret;
}

int pipe_adi2axis_get_bytes (int dev, unsigned long *reg)
{
	int ret;

	if ( (ret = ioctl(pipe_dev_fd, ADI2AXIS_G_BYTES[dev], reg)) )
		printf("ADI2AXIS_G_BYTES[%d]: %d: %s\n", dev, ret, strerror(errno));

	return ret;
}

