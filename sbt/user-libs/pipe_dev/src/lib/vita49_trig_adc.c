/** \file      src/lib/vita49_trig_adc.c
 *  \brief     implementation of data source/sink controls
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


static int VITA49_TRIG_S_CTRL[2] = { PD_IOCS_VITA49_TRIG_ADC_0_CTRL, PD_IOCS_VITA49_TRIG_ADC_1_CTRL };
static int VITA49_TRIG_G_CTRL[2] = { PD_IOCG_VITA49_TRIG_ADC_0_CTRL, PD_IOCG_VITA49_TRIG_ADC_1_CTRL };
static int VITA49_TRIG_G_STAT[2] = { PD_IOCG_VITA49_TRIG_ADC_0_STAT, PD_IOCG_VITA49_TRIG_ADC_1_STAT };
static int VITA49_TRIG_S_TS[2]   = { PD_IOCS_VITA49_TRIG_ADC_0_TS,   PD_IOCS_VITA49_TRIG_ADC_1_TS   };
static int VITA49_TRIG_G_TS[2]   = { PD_IOCG_VITA49_TRIG_ADC_0_TS,   PD_IOCG_VITA49_TRIG_ADC_1_TS   };


int pipe_vita49_trig_adc_set_ctrl (int dev, unsigned long reg)
{
	int ret;

	if ( (ret = ioctl(pipe_dev_fd, VITA49_TRIG_S_CTRL[dev], reg)) )
		ERROR("VITA49_TRIG_S_CTRL[%d], %08x: %d: %s\n", dev, reg, ret, strerror(errno));

	return ret;
}

int pipe_vita49_trig_adc_get_ctrl (int dev, unsigned long *reg)
{
	int ret;

	if ( (ret = ioctl(pipe_dev_fd, VITA49_TRIG_G_CTRL[dev], reg)) )
		ERROR("VITA49_TRIG_G_CTRL[%d]: %d: %s\n", dev, ret, strerror(errno));

	return ret;
}

int pipe_vita49_trig_adc_get_stat (int dev, unsigned long *reg)
{
	int ret;

	if ( (ret = ioctl(pipe_dev_fd, VITA49_TRIG_G_STAT[dev], reg)) )
		ERROR("VITA49_TRIG_G_STAT[%d]: %d: %s\n", dev, ret, strerror(errno));

	return ret;
}

int pipe_vita49_trig_adc_set_ts (int dev, struct pd_vita49_ts *buf)
{
	int ret;

	if ( (ret = ioctl(pipe_dev_fd, VITA49_TRIG_S_TS[dev], buf)) )
		ERROR("VITA49_TRIG_S_TS[%d]: %d: %s\n", dev, ret, strerror(errno));

	return ret;
}

int pipe_vita49_trig_adc_get_ts (int dev, struct pd_vita49_ts *buf)
{
	int ret;

	if ( (ret = ioctl(pipe_dev_fd, VITA49_TRIG_G_TS[dev], buf)) )
		ERROR("VITA49_TRIG_G_TS[%d]: %d: %s\n", dev, ret, strerror(errno));

	return ret;
}

