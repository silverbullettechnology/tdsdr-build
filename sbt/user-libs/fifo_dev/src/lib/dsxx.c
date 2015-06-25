/** \file      src/lib//dsxx.c
 *  \brief     implementation of data source/sink controls
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

#include <fd_main.h>

#include "fifo_dev.h"
#include "private.h"


static int DSRC_S_CTRL[2]  = { FD_IOCS_DSRC0_CTRL,  FD_IOCS_DSRC1_CTRL  };
static int DSRC_G_STAT[2]  = { FD_IOCG_DSRC0_STAT,  FD_IOCG_DSRC1_STAT  };
static int DSRC_G_BYTES[2] = { FD_IOCG_DSRC0_BYTES, FD_IOCG_DSRC1_BYTES };
static int DSRC_S_BYTES[2] = { FD_IOCS_DSRC0_BYTES, FD_IOCS_DSRC1_BYTES };
static int DSRC_G_SENT[2]  = { FD_IOCG_DSRC0_SENT,  FD_IOCG_DSRC1_SENT  };
static int DSRC_G_TYPE[2]  = { FD_IOCG_DSRC0_TYPE,  FD_IOCG_DSRC1_TYPE  };
static int DSRC_S_TYPE[2]  = { FD_IOCS_DSRC0_TYPE,  FD_IOCS_DSRC1_TYPE  };
static int DSRC_G_REPS[2]  = { FD_IOCG_DSRC0_REPS,  FD_IOCG_DSRC1_REPS  };
static int DSRC_S_REPS[2]  = { FD_IOCS_DSRC0_REPS,  FD_IOCS_DSRC1_REPS  };
static int DSRC_G_RSENT[2] = { FD_IOCG_DSRC0_RSENT, FD_IOCG_DSRC1_RSENT };

static int DSNK_S_CTRL[2]  = { FD_IOCS_DSNK0_CTRL,  FD_IOCS_DSNK1_CTRL  };
static int DSNK_G_STAT[2]  = { FD_IOCG_DSNK0_STAT,  FD_IOCG_DSNK1_STAT  };
static int DSNK_G_BYTES[2] = { FD_IOCG_DSNK0_BYTES, FD_IOCG_DSNK1_BYTES };
static int DSNK_G_SUM[2]   = { FD_IOCG_DSNK0_SUM,   FD_IOCG_DSNK1_SUM   };


int fifo_dsrc_set_ctrl (int dev, unsigned long reg)
{
	int ret;

	if ( (ret = ioctl(fifo_dev_fd, DSRC_S_CTRL[dev], reg)) )
		ERROR("DSRC_S_CTRL[%d], %08x: %d: %s\n", dev, reg, ret, strerror(errno));

	return ret;
}

int fifo_dsrc_get_stat (int dev, unsigned long *reg)
{
	int ret;

	if ( (ret = ioctl(fifo_dev_fd, DSRC_G_STAT[dev], reg)) )
		ERROR("DSRC_G_STAT[%d]: %d: %s\n", dev, ret, strerror(errno));

	return ret;
}

int fifo_dsrc_set_bytes (int dev, unsigned long reg)
{
	int ret;

	if ( (ret = ioctl(fifo_dev_fd, DSRC_S_BYTES[dev], reg)) )
		ERROR("DSRC_S_BYTES[%d], %08x: %d: %s\n", dev, reg, ret, strerror(errno));

	return ret;
}

int fifo_dsrc_get_bytes (int dev, unsigned long *reg)
{
	int ret;

	if ( (ret = ioctl(fifo_dev_fd, DSRC_G_BYTES[dev], reg)) )
		ERROR("DSRC_G_BYTES[%d]: %d: %s\n", dev, ret, strerror(errno));

	return ret;
}

int fifo_dsrc_get_sent (int dev, unsigned long *reg)
{
	int ret;

	if ( (ret = ioctl(fifo_dev_fd, DSRC_G_SENT[dev], reg)) )
		ERROR("DSRC_G_SENT[%d]: %d: %s\n", dev, ret, strerror(errno));

	return ret;
}

int fifo_dsrc_set_type (int dev, unsigned long reg)
{
	int ret;

	if ( (ret = ioctl(fifo_dev_fd, DSRC_S_TYPE[dev], reg)) )
		ERROR("DSRC_S_TYPE[%d], %08x: %d: %s\n", dev, reg, ret, strerror(errno));

	return ret;
}

int fifo_dsrc_get_type (int dev, unsigned long *reg)
{
	int ret;

	if ( (ret = ioctl(fifo_dev_fd, DSRC_G_TYPE[dev], reg)) )
		ERROR("DSRC_G_TYPE[%d]: %d: %s\n", dev, ret, strerror(errno));

	return ret;
}

int fifo_dsrc_set_reps (int dev, unsigned long reg)
{
	int ret;

	if ( (ret = ioctl(fifo_dev_fd, DSRC_S_REPS[dev], reg)) )
		ERROR("DSRC_S_REPS[%d], %08x: %d: %s\n", dev, reg, ret, strerror(errno));

	return ret;
}

int fifo_dsrc_get_reps (int dev, unsigned long *reg)
{
	int ret;

	if ( (ret = ioctl(fifo_dev_fd, DSRC_G_REPS[dev], reg)) )
		ERROR("DSRC_G_REPS[%d]: %d: %s\n", dev, ret, strerror(errno));

	return ret;
}

int fifo_dsrc_get_rsent (int dev, unsigned long *reg)
{
	int ret;

	if ( (ret = ioctl(fifo_dev_fd, DSRC_G_RSENT[dev], reg)) )
		ERROR("DSRC_G_RSENT[%d]: %d: %s\n", dev, ret, strerror(errno));

	return ret;
}


int fifo_dsnk_set_ctrl (int dev, unsigned long reg)
{
	int ret;

	if ( (ret = ioctl(fifo_dev_fd, DSNK_S_CTRL[dev], reg)) )
		ERROR("DSNK_S_CTRL[%d], %08x: %d: %s\n", dev, reg, ret, strerror(errno));

	return ret;
}

int fifo_dsnk_get_stat (int dev, unsigned long *reg)
{
	int ret;

	if ( (ret = ioctl(fifo_dev_fd, DSNK_G_STAT[dev], reg)) )
		ERROR("DSNK_G_STAT[%d]: %d: %s\n", dev, ret, strerror(errno));

	return ret;
}

int fifo_dsnk_get_bytes (int dev, unsigned long *reg)
{
	int ret;

	if ( (ret = ioctl(fifo_dev_fd, DSNK_G_BYTES[dev], reg)) )
		ERROR("DSNK_G_BYTES[%d]: %d: %s\n", dev, ret, strerror(errno));

	return ret;
}

int fifo_dsnk_get_sum (int dev, unsigned long *reg)
{
	int ret;

	if ( (ret = ioctl(fifo_dev_fd, DSNK_G_SUM[dev], reg)) )
		ERROR("DSNK_G_SUM[%d]: %d: %s\n", dev, ret, strerror(errno));

	return ret;
}


