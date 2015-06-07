/** \file      src/lib/adi_old.c
 *  \brief     implementation for legacy ADI FIFO controls
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
#include <string.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <errno.h>

#include <fd_main.h>

#include "fifo_dev.h"
#include "private.h"


static int  ADI_S_CTRL[2]   = { FD_IOCS_ADI1_OLD_CTRL,   FD_IOCS_ADI2_OLD_CTRL   };
static int  ADI_G_CTRL[2]   = { FD_IOCG_ADI1_OLD_CTRL,   FD_IOCG_ADI2_OLD_CTRL   };
static int  ADI_S_TX_CNT[2] = { FD_IOCS_ADI1_OLD_TX_CNT, FD_IOCS_ADI2_OLD_TX_CNT };
static int  ADI_G_TX_CNT[2] = { FD_IOCG_ADI1_OLD_TX_CNT, FD_IOCG_ADI2_OLD_TX_CNT };
static int  ADI_S_RX_CNT[2] = { FD_IOCS_ADI1_OLD_RX_CNT, FD_IOCS_ADI2_OLD_RX_CNT };
static int  ADI_G_RX_CNT[2] = { FD_IOCG_ADI1_OLD_RX_CNT, FD_IOCG_ADI2_OLD_RX_CNT };
static int  ADI_G_SUM[2]    = { FD_IOCG_ADI1_OLD_SUM,    FD_IOCG_ADI2_OLD_SUM    };
static int  ADI_G_LAST[2]   = { FD_IOCG_ADI1_OLD_LAST,   FD_IOCG_ADI2_OLD_LAST   };
static int  ADI_S_CS_RST[2] = { FD_IOCS_ADI1_OLD_CS_RST, FD_IOCS_ADI2_OLD_CS_RST };


int fifo_adi_old_set_ctrl (int dev, unsigned long reg)
{
	int ret;

	if ( (ret = ioctl(fifo_dev_fd, ADI_S_CTRL[dev], reg)) )
		printf("ADI_S_CTRL[%d], %08x: %d: %s\n", dev, reg, ret, strerror(errno));

	return ret;
}

int fifo_adi_old_get_ctrl (int dev, unsigned long *reg)
{
	int ret;

	if ( (ret = ioctl(fifo_dev_fd, ADI_G_CTRL[dev], reg)) )
		printf("ADI_G_CTRL[%d]: %d: %s\n", dev, ret, strerror(errno));

	return ret;
}


int fifo_adi_old_set_tx_cnt (int dev, unsigned long len, unsigned long reps)
{
	unsigned long long  words = len;
	int                 ret;

	words *= reps;
	if ( words >= 0x100000000ULL )
	{
		printf("Specified %lu TX reps exceeds limits...\n", reps);
		words  = 0xFFFFFFFFULL;
		words /= len;
		reps   = words;
		printf("Adjusted to %lu TX reps.\n", reps);
	}

	if ( (ret = ioctl(fifo_dev_fd, ADI_S_TX_CNT[dev], len * reps)) )
		printf("ADI_S_TX_CNT[%d], %08x: %d: %s\n", dev, len, ret, strerror(errno));

	return ret;
}

int fifo_adi_old_get_tx_cnt (int dev, unsigned long *reg)
{
	int ret;

	if ( (ret = ioctl(fifo_dev_fd, ADI_G_TX_CNT[dev], reg)) )
		printf("ADI_G_TX_CNT[%d]: %d: %s\n", dev, ret, strerror(errno));

	return ret;
}


int fifo_adi_old_set_rx_cnt (int dev, unsigned long len, unsigned long reps)
{
	int ret;

	if ( (ret = ioctl(fifo_dev_fd, ADI_S_RX_CNT[dev], len)) )
		printf("ADI_S_RX_CNT[%d], %08x: %d: %s\n", dev, len, ret, strerror(errno));

	return ret;
}

int fifo_adi_old_get_rx_cnt (int dev, unsigned long *reg)
{
	int ret;

	if ( (ret = ioctl(fifo_dev_fd, ADI_G_RX_CNT[dev], reg)) )
		printf("ADI_G_RX_CNT[%d]: %d: %s\n", dev, ret, strerror(errno));

	return ret;
}


int fifo_adi_old_get_sum (int dev, unsigned long *sum)
{
	int ret;

	if ( (ret = ioctl(fifo_dev_fd, ADI_G_SUM[dev], sum)) )
		printf("ADI_G_SUM[%d]: %d: %08lx.%08lx: %s\n",
		       dev, ret, sum[0], sum[1], strerror(errno));

	return ret;
}

int fifo_adi_old_get_last (int dev, unsigned long *last)
{
	int ret;

	if ( (ret = ioctl(fifo_dev_fd, ADI_G_LAST[dev], last)) )
		printf("ADI_G_LAST[%d]: %d: %08lx.%08lx: %s\n",
		       dev, ret, last[0], last[1], strerror(errno));

	return ret;
}

int fifo_adi_old_chksum_reset (int dev)
{
	int ret;

	if ( (ret = ioctl(fifo_dev_fd, ADI_S_CS_RST[dev], 0xFFFFFFFF)) )
		printf("ADI_S_CS_RST[%d]: %d: %s\n", dev, ret, strerror(errno));

	return ret;
}


int fifo_adi_old_get_fifo_cnt (struct fd_fifo_counts *fb)
{
	int ret;

	if ( (ret = ioctl(fifo_dev_fd, FD_IOCG_FIFO_CNT, fb)) )
		printf("FD_IOCG_FIFO_CNT: %d: %s\n", ret, strerror(errno));

	return ret;
}
