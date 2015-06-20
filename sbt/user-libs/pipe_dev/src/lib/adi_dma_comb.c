/** \file      src/lib/adi_dma_.c
 *  \brief     implementation of ADI_DMA_COMB IOCTLs
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


static int ADI_DMA_COMB_S_CMD[2]   = { PD_IOCS_ADI_DMA_COMB_0_CMD,   PD_IOCS_ADI_DMA_COMB_1_CMD   };
static int ADI_DMA_COMB_G_CMD[2]   = { PD_IOCG_ADI_DMA_COMB_0_CMD,   PD_IOCG_ADI_DMA_COMB_1_CMD   };
static int ADI_DMA_COMB_G_STAT[2]  = { PD_IOCG_ADI_DMA_COMB_0_STAT,  PD_IOCG_ADI_DMA_COMB_1_STAT  };
static int ADI_DMA_COMB_S_NPKTS[2] = { PD_IOCS_ADI_DMA_COMB_0_NPKTS, PD_IOCS_ADI_DMA_COMB_1_NPKTS };
static int ADI_DMA_COMB_G_NPKTS[2] = { PD_IOCG_ADI_DMA_COMB_0_NPKTS, PD_IOCG_ADI_DMA_COMB_1_NPKTS };


int pipe_adi_dma_comb_set_cmd (int dev, unsigned long reg)
{
	int ret;

	if ( (ret = ioctl(pipe_dev_fd, ADI_DMA_COMB_S_CMD[dev], reg)) )
		printf("ADI_DMA_COMB_S_CMD[%d], %08x: %d: %s\n", dev, reg, ret, strerror(errno));

	return ret;
}

int pipe_adi_dma_comb_get_cmd (int dev, unsigned long *reg)
{
	int ret;

	if ( (ret = ioctl(pipe_dev_fd, ADI_DMA_COMB_G_CMD[dev], reg)) )
		printf("ADI_DMA_COMB_G_CMD[%d]: %d: %s\n", dev, ret, strerror(errno));

	return ret;
}

int pipe_adi_dma_comb_get_stat (int dev, unsigned long *reg)
{
	int ret;

	if ( (ret = ioctl(pipe_dev_fd, ADI_DMA_COMB_G_STAT[dev], reg)) )
		printf("ADI_DMA_COMB_G_STAT[%d]: %d: %s\n", dev, ret, strerror(errno));

	return ret;
}

int pipe_adi_dma_comb_set_npkts (int dev, unsigned long reg)
{
	int ret;

	if ( (ret = ioctl(pipe_dev_fd, ADI_DMA_COMB_S_NPKTS[dev], reg)) )
		printf("ADI_DMA_COMB_S_NPKTS[%d], %08x: %d: %s\n", dev, reg, ret, strerror(errno));

	return ret;
}

int pipe_adi_dma_comb_get_npkts (int dev, unsigned long *reg)
{
	int ret;

	if ( (ret = ioctl(pipe_dev_fd, ADI_DMA_COMB_G_NPKTS[dev], reg)) )
		printf("ADI_DMA_COMB_G_NPKTS[%d]: %d: %s\n", dev, ret, strerror(errno));

	return ret;
}

