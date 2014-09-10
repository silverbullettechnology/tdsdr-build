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
#include <sys/ioctl.h>
#include <sys/types.h>
#include <errno.h>

#include <dma_streamer_mod.h>

#include "dsa_main.h"

#include "log.h"
LOG_MODULE_STATIC("ioctl", LOG_LEVEL_INFO);


int dsa_ioctl_dsm_map (struct dsm_user_buffs *buffs)
{
	int ret;

	errno = 0;
	if ( (ret = ioctl(dsa_dsm_dev, DSM_IOCS_MAP, buffs)) )
		printf("DSM_IOCS_MAP: %d: %s\n", ret, strerror(errno));

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

#if 0
int dsa_ioctl_target_list (unsigned long *mask)
{
	int ret;

	errno = 0;
	if ( (ret = ioctl(dsa_dsm_dev, DSM_IOCG_TARGET_LIST, mask)) )
		printf("DSM_IOCG_TARGET_LIST: %d: %s", ret, strerror(errno));

	return ret;
}
#endif

int dsa_ioctl_dsm_set_timeout (unsigned long timeout)
{
	int ret;

	errno = 0;
	if ( (ret = ioctl(dsa_dsm_dev, DSM_IOCS_TIMEOUT, timeout)) )
		printf("DSM_IOCS_TIMEOUT %d: %d: %s\n", timeout, ret, strerror(errno));

	return ret;
}

int dsa_ioctl_dsm_oneshot_start (void)
{
	int ret;

	if ( (ret = ioctl(dsa_dsm_dev, DSM_IOCS_ONESHOT_START, 0)) )
		printf("DSM_IOCS_ONESHOT_START: %d: %s\n", ret, strerror(errno));

	return ret;
}

int dsa_ioctl_dsm_oneshot_wait (void)
{
	int ret;

	if ( (ret = ioctl(dsa_dsm_dev, DSM_IOCS_ONESHOT_WAIT, 0)) )
		printf("DSM_IOCS_ONESHOT_WAIT: %d: %s\n", ret, strerror(errno));

	return ret;
}

int dsa_ioctl_dsm_get_stats (struct dsm_user_stats *sb)
{
	int ret;

	if ( (ret = ioctl(dsa_dsm_dev, DSM_IOCG_STATS, sb)) )
		printf("DSM_IOCG_STATS: %d: %s\n", ret, strerror(errno));

	return ret;
}

int dsa_ioctl_dsm_continuous_stop (void)
{
	int  ret;

	if ( (ret = ioctl(dsa_fifo_dev, DSM_IOCS_CONTINUOUS_STOP, 0)) )
		printf("DSM_IOCS_CONTINUOUS_STOP: %d: %s\n", ret, strerror(errno));

	return ret;
}

int dsa_ioctl_dsm_continuous_start (void)
{
	int  ret;

	if ( (ret = ioctl(dsa_fifo_dev, DSM_IOCS_CONTINUOUS_START, 0)) )
		printf("DSM_IOCS_CONTINUOUS_START: %d: %s\n", ret, strerror(errno));

	return ret;
}



