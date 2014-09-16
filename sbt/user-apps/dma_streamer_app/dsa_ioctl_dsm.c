/** \file      dsa_ioctl_dsm.c
 *  \brief     implementation of DSM controls
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
#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <errno.h>

#include <dma_streamer_mod.h>

#include "dsa_main.h"

#include "log.h"
LOG_MODULE_STATIC("ioctl", LOG_LEVEL_DEBUG);


int dsa_ioctl_dsm_limits (struct dsm_limits *limits)
{
	int ret;

	errno = 0;
	if ( (ret = ioctl(dsa_dsm_dev, DSM_IOCG_LIMITS, limits)) )
		printf("DSM_IOCG_LIMITS: %d: %s\n", ret, strerror(errno));

	return ret;
}


struct dsm_chan_list *dsa_ioctl_dsm_channels (void)
{
	struct dsm_chan_list  tmp;
	struct dsm_chan_list *ret;

	// first call to size list
	errno = 0;
	memset(&tmp, 0, sizeof(tmp));
	if ( ioctl(dsa_dsm_dev, DSM_IOCG_CHANNELS, &tmp) < 0 )
	{
		printf("DSM_IOCG_CHANNELS: %s", strerror(errno));
		return NULL;
	}

	// allocate correct size now
	ret = calloc(offsetof(struct dsm_chan_list, list) + 
	             sizeof(struct dsm_chan_desc) * tmp.size, 1);
	if ( !ret )
		return NULL;

	// second call with big-enough buffer
	errno = 0;
	ret->size = tmp.size;
	if ( ioctl(dsa_dsm_dev, DSM_IOCG_CHANNELS, ret) < 0 )
	{
		int err = errno;
		printf("DSM_IOCG_CHANNELS: %s", strerror(err));
		free(ret);
		errno = err;
		return NULL;
	}

	return ret;
}

int dsa_ioctl_dsm_map_chan (unsigned long slot, unsigned long tx,
                            unsigned long size, struct dsm_xfer_buff *list)
{
	struct dsm_chan_buffs *buffs;
	int                    ret;

	errno = 0;
	buffs = malloc(offsetof(struct dsm_chan_buffs, list) +
	               sizeof(struct dsm_xfer_buff) * size);
	if ( !buffs )
		return -1;

	buffs->slot = slot;
	buffs->tx   = tx;
	buffs->size = size;
	memcpy(buffs->list, list, sizeof(struct dsm_xfer_buff) * size);
	if ( (ret = ioctl(dsa_dsm_dev, DSM_IOCS_MAP_CHAN, buffs)) )
		printf("DSM_IOCS_MAP_CHAN: %d: %s\n", ret, strerror(errno));

	free(buffs);
	return ret;
}

int dsa_ioctl_dsm_unmap (void)
{
	int ret;

	errno = 0;
	if ( (ret = ioctl(dsa_dsm_dev, DSM_IOCS_UNMAP, 0)) )
		printf("DSM_IOCS_UNMAP: %d: %s\n", ret, strerror(errno));

	return ret;
}

int dsa_ioctl_dsm_set_timeout (unsigned long timeout)
{
	int ret;

	errno = 0;
	if ( (ret = ioctl(dsa_dsm_dev, DSM_IOCS_TIMEOUT, timeout)) )
		printf("DSM_IOCS_TIMEOUT %d: %d: %s\n", timeout, ret, strerror(errno));

	return ret;
}

int dsa_ioctl_dsm_oneshot_start (unsigned long mask)
{
	int ret;

	if ( (ret = ioctl(dsa_dsm_dev, DSM_IOCS_ONESHOT_START, mask)) )
		printf("DSM_IOCS_ONESHOT_START: %d: %s\n", ret, strerror(errno));

	return ret;
}

int dsa_ioctl_dsm_oneshot_wait (unsigned long mask)
{
	int ret;

	if ( (ret = ioctl(dsa_dsm_dev, DSM_IOCS_ONESHOT_WAIT, mask)) )
		printf("DSM_IOCS_ONESHOT_WAIT: %d: %s\n", ret, strerror(errno));

	return ret;
}

struct dsm_chan_stats *dsa_ioctl_dsm_get_stats (void)
{
	struct dsm_chan_stats  tmp;
	struct dsm_chan_stats *ret;

	// first call to size list
	errno = 0;
	memset(&tmp, 0, sizeof(tmp));
	if ( ioctl(dsa_dsm_dev, DSM_IOCG_STATS, &tmp) < 0 )
	{
		printf("DSM_IOCG_STATS: %s", strerror(errno));
		return NULL;
	}

	// allocate correct size now
	ret = malloc(offsetof(struct dsm_chan_stats, list) + 
	             sizeof(struct dsm_xfer_stats) * tmp.size);
	if ( !ret )
		return NULL;

	// second call with big-enough buffer
	errno = 0;
	ret->size = tmp.size;
	if ( ioctl(dsa_dsm_dev, DSM_IOCG_STATS, ret) < 0 )
	{
		int err = errno;
		printf("DSM_IOCG_STATS: %s", strerror(err));
		free(ret);
		errno = err;
		return NULL;
	}

	return ret;
}

int dsa_ioctl_dsm_continuous_stop (unsigned long mask)
{
	int  ret;

	if ( (ret = ioctl(dsa_fifo_dev, DSM_IOCS_CONTINUOUS_STOP, mask)) )
		printf("DSM_IOCS_CONTINUOUS_STOP: %d: %s\n", ret, strerror(errno));

	return ret;
}

int dsa_ioctl_dsm_continuous_start (unsigned long mask)
{
	int  ret;

	if ( (ret = ioctl(dsa_fifo_dev, DSM_IOCS_CONTINUOUS_START, mask)) )
		printf("DSM_IOCS_CONTINUOUS_START: %d: %s\n", ret, strerror(errno));

	return ret;
}



