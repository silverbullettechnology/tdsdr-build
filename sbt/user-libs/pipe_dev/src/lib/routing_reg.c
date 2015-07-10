/** \file      src/lib/routing_reg.c
 *  \brief     implementation of ROUTING_REG IOCTLs
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


int pipe_routing_reg_get_adc_sw_dest (unsigned long *reg)
{
	int ret;

	if ( (ret = ioctl(pipe_dev_fd, PD_IOCG_ROUTING_REG_ADC_SW_DEST, reg)) )
		ERROR("PD_IOCG_ROUTING_REG_ADC_SW_DEST: %d: %s\n", ret, strerror(errno));

	return ret;
}

int pipe_routing_reg_set_adc_sw_dest (unsigned long reg)
{
	int ret;

	if ( (ret = ioctl(pipe_dev_fd, PD_IOCS_ROUTING_REG_ADC_SW_DEST, reg)) )
		ERROR("PD_IOCS_ROUTING_REG_ADC_SW_DEST: %d: %s\n", ret, strerror(errno));

	return ret;
}

