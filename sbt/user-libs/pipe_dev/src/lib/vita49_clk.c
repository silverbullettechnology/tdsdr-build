/** \file      src/lib/vita49_clk.c
 *  \brief     implementation of VITA49 CLOCK IOCTLs
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


static int VITA49_CLK_READ[2] = { PD_IOCG_VITA49_CLK_READ_0, PD_IOCG_VITA49_CLK_READ_1 };

int pipe_vita49_clk_set_ctrl (unsigned long reg)
{
	int ret;

	if ( (ret = ioctl(pipe_dev_fd, PD_IOCS_VITA49_CLK_CTRL, reg)) )
		ERROR("PD_IOCS_VITA49_CLK_CTRL, %08x: %d: %s\n", reg, ret, strerror(errno));

	return ret;
}

int pipe_vita49_clk_get_ctrl (unsigned long *reg)
{
	int ret;

	if ( (ret = ioctl(pipe_dev_fd, PD_IOCG_VITA49_CLK_CTRL, reg)) )
		ERROR("PD_IOCG_VITA49_CLK_CTRL, %08x: %d: %s\n", ret, reg, strerror(errno));

	return ret;
}

int pipe_vita49_clk_get_stat (unsigned long *reg)
{
	int ret;

	if ( (ret = ioctl(pipe_dev_fd, PD_IOCG_VITA49_CLK_STAT, reg)) )
		ERROR("PD_IOCG_VITA49_CLK_STAT: %d: %s\n", ret, strerror(errno));

	return ret;
}

int pipe_vita49_clk_set_tsi (unsigned long reg)
{
	int ret;

	if ( (ret = ioctl(pipe_dev_fd, PD_IOCS_VITA49_CLK_TSI, reg)) )
		ERROR("PD_IOCS_VITA49_CLK_TSI, %08x: %d: %s\n", reg, ret, strerror(errno));

	return ret;
}

int pipe_vita49_clk_get_tsi (unsigned long *reg)
{
	int ret;

	if ( (ret = ioctl(pipe_dev_fd, PD_IOCG_VITA49_CLK_TSI, reg)) )
		ERROR("PD_IOCG_VITA49_CLK_TSI: %d: %s\n", ret, strerror(errno));

	return ret;
}

int pipe_vita49_clk_read (int dev, struct pd_vita49_ts *ts)
{
	int ret;

	if ( (ret = ioctl(pipe_dev_fd, VITA49_CLK_READ[dev], ts)) )
		ERROR("VITA49_CLK_READ[%d]: %d: %s\n", dev, ret, strerror(errno));

	return ret;
}

