/** \file      src/lib/srio_dma_comb.c
 *  \brief     implementation of Access control IOCTLs
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


int pipe_srio_dma_comb_set_cmd (unsigned long reg)
{
	int ret;

	if ( (ret = ioctl(pipe_dev_fd, PD_IOCS_SRIO_DMA_COMB_CMD, reg)) )
		ERROR("PD_IOCS_SRIO_DMA_COMB_CMD: %d: %s\n", ret, strerror(errno));

	return ret;
}

int pipe_srio_dma_comb_get_cmd (unsigned long *reg)
{
	int ret;

	if ( (ret = ioctl(pipe_dev_fd, PD_IOCG_SRIO_DMA_COMB_CMD, reg)) )
		ERROR("PD_IOCG_SRIO_DMA_COMB_CMD: %d: %s\n", ret, strerror(errno));

	return ret;
}

int pipe_srio_dma_comb_get_stat (unsigned long *reg)
{
	int ret;

	if ( (ret = ioctl(pipe_dev_fd, PD_IOCG_SRIO_DMA_COMB_STAT, reg)) )
		ERROR("PD_IOCG_SRIO_DMA_COMB_STAT: %d: %s\n", ret, strerror(errno));

	return ret;
}

int pipe_srio_dma_comb_set_npkts (unsigned long reg)
{
	int ret;

	if ( (ret = ioctl(pipe_dev_fd, PD_IOCS_SRIO_DMA_COMB_NPKTS, reg)) )
		ERROR("PD_IOCS_SRIO_DMA_COMB_NPKTS: %d: %s\n", ret, strerror(errno));

	return ret;
}

int pipe_srio_dma_comb_get_npkts (unsigned long *reg)
{
	int ret;

	if ( (ret = ioctl(pipe_dev_fd, PD_IOCG_SRIO_DMA_COMB_NPKTS, reg)) )
		ERROR("PD_IOCG_SRIO_DMA_COMB_NPKTS: %d: %s\n", ret, strerror(errno));

	return ret;
}

