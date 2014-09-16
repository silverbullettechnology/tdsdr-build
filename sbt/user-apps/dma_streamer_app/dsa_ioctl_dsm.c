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
	ret = calloc(offsetof(struct dsm_chan_list, chan_lst) + 
	             sizeof(struct dsm_chan_desc) * tmp.chan_cnt, 1);
	if ( !ret )
		return NULL;

	// second call with big-enough buffer
	errno = 0;
	ret->chan_cnt = tmp.chan_cnt;
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

int dsa_ioctl_dsm_limits (struct dsm_limits *limits)
{
	int ret;

	errno = 0;
	if ( (ret = ioctl(dsa_dsm_dev, DSM_IOCG_LIMITS, limits)) )
		printf("DSM_IOCG_LIMITS: %d: %s\n", ret, strerror(errno));

	return ret;
}

int dsa_ioctl_dsm_map_chan (unsigned long ident,    unsigned long tx,
                            unsigned long buff_cnt, struct dsm_xfer_buff *buff_lst)
{
	struct dsm_chan_buffs *buffs;
	int                    ret;

	errno = 0;
	buffs = malloc(offsetof(struct dsm_chan_buffs, buff_lst) +
	               sizeof(struct dsm_xfer_buff) * buff_cnt);
	if ( !buffs )
		return -1;

	buffs->ident    = ident;
	buffs->tx       = tx;
	buffs->buff_cnt = buff_cnt;
	memcpy(buffs->buff_lst, buff_lst, sizeof(struct dsm_xfer_buff) * buff_cnt);
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

int dsa_ioctl_dsm_get_stats (struct dsm_chan_stats *sb)
{
	int ret;

	if ( (ret = ioctl(dsa_dsm_dev, DSM_IOCG_STATS, sb)) )
		printf("DSM_IOCG_STATS: %d: %s\n", ret, strerror(errno));

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



