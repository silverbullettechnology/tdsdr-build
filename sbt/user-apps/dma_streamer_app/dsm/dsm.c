/** \file      dsm/dsm.c
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
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#include <dma_streamer_mod.h>


#include "dsm/dsm.h"


static int  dsm_fd = -1;


struct dsm_limits     dsm_limits;
struct dsm_chan_list *dsm_channels = NULL;


void dsm_close (void)
{
	if ( dsm_fd >= 0 )
		close(dsm_fd);
	dsm_fd = -1;
	free(dsm_channels);
}


int dsm_reopen (const char *node)
{
	struct dsm_chan_list  tmp;
	int                   ret;
	int                   err;

	dsm_close();

	errno = 0;
	if ( (dsm_fd = open(node, O_RDWR)) < 0 )
		return dsm_fd;

	errno = 0;
	if ( (ret = ioctl(dsm_fd, DSM_IOCG_LIMITS, &dsm_limits)) )
	{
		err = errno;
		printf("DSM_IOCG_LIMITS: %d: %s\n", ret, strerror(errno));
		dsm_close();
		errno = err;
		return -1;
	}

	// first call to size list
	errno = 0;
	memset(&tmp, 0, sizeof(tmp));
	if ( ioctl(dsm_fd, DSM_IOCG_CHANNELS, &tmp) < 0 )
	{
		err = errno;
		printf("DSM_IOCG_CHANNELS: %s", strerror(errno));
		dsm_close();
		errno = err;
		return -1;
	}

	// allocate correct size now
	errno = 0;
	dsm_channels = calloc(offsetof(struct dsm_chan_list, list) + 
	                      sizeof(struct dsm_chan_desc) * tmp.size, 1);
	if ( !dsm_channels )
		return -1;

	// second call with big-enough buffer
	errno = 0;
	dsm_channels->size = tmp.size;
	if ( ioctl(dsm_fd, DSM_IOCG_CHANNELS, dsm_channels) < 0 )
	{
		err = errno;
		printf("DSM_IOCG_CHANNELS: %s", strerror(err));
		dsm_close();
		free(dsm_channels);
		errno = err;
		return -1;
	}

	return 0;
}


void *dsm_alloc (size_t size)
{
	void  *buff;
	long   page;

	// size specified in words, but posix_memalign() and mlock() use bytes
	size *= DSM_BUS_WIDTH;

	// need page size for posix_memalign()
	errno = 0;
	if ( (page = sysconf(_SC_PAGESIZE)) < 0 )
		return NULL;

	errno = 0;
	if ( posix_memalign(&buff, page, size) )
		return NULL;

	errno = 0;
	if ( mlock(buff, size) )
	{
		int err = errno;
		free(buff);
		errno = err;
		return NULL;
	}

	return buff;
}


void dsm_free (void *addr, size_t size)
{
	// size specified in words, but munlock() uses bytes
	munlock(addr, size * DSM_BUS_WIDTH);
	free(addr);
}


int dsm_map_user_array (unsigned long slot, unsigned long tx,
                        unsigned long size, struct dsm_user_xfer *list)
{
	struct dsm_user_buffs *buffs;
	int                    ret;

	errno = 0;
	buffs = malloc(offsetof(struct dsm_user_buffs, list) +
	               sizeof(struct dsm_user_xfer) * size);
	if ( !buffs )
		return -1;

	buffs->slot = slot;
	buffs->tx   = tx;
	buffs->size = size;
	memcpy(buffs->list, list, sizeof(struct dsm_user_xfer) * size);

	if ( (ret = ioctl(dsm_fd, DSM_IOCS_MAP_USER, buffs)) )
		printf("DSM_IOCS_MAP_USER: %d: %s\n", ret, strerror(errno));

	free(buffs);
	return ret;
}


int dsm_map_user (unsigned long slot, unsigned long tx, void *addr, unsigned long size)
{
	struct dsm_user_xfer xfer =
	{
		.addr = (unsigned long)addr,
		.size = size,
	};

	return dsm_map_user_array(slot, tx, 1, &xfer);
}


int dsm_cleanup (void)
{
	int ret;

	errno = 0;
	if ( (ret = ioctl(dsm_fd, DSM_IOCS_CLEANUP, 0)) )
		printf("DSM_IOCS_CLEANUP: %d: %s\n", ret, strerror(errno));

	return ret;
}

int dsm_set_timeout (unsigned long timeout)
{
	int ret;

	errno = 0;
	if ( (ret = ioctl(dsm_fd, DSM_IOCS_TIMEOUT, timeout)) )
		printf("DSM_IOCS_TIMEOUT %d: %d: %s\n", timeout, ret, strerror(errno));

	return ret;
}

int dsm_oneshot_start (unsigned long mask)
{
	int ret;

	if ( (ret = ioctl(dsm_fd, DSM_IOCS_ONESHOT_START, mask)) )
		printf("DSM_IOCS_ONESHOT_START: %d: %s\n", ret, strerror(errno));

	return ret;
}

int dsm_oneshot_wait (unsigned long mask)
{
	int ret;

	if ( (ret = ioctl(dsm_fd, DSM_IOCS_ONESHOT_WAIT, mask)) )
		printf("DSM_IOCS_ONESHOT_WAIT: %d: %s\n", ret, strerror(errno));

	return ret;
}

struct dsm_chan_stats *dsm_get_stats (void)
{
	struct dsm_chan_stats  tmp;
	struct dsm_chan_stats *ret;

	// first call to size list
	errno = 0;
	memset(&tmp, 0, sizeof(tmp));
	if ( ioctl(dsm_fd, DSM_IOCG_STATS, &tmp) < 0 )
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
	if ( ioctl(dsm_fd, DSM_IOCG_STATS, ret) < 0 )
	{
		int err = errno;
		printf("DSM_IOCG_STATS: %s", strerror(err));
		free(ret);
		errno = err;
		return NULL;
	}

	return ret;
}

int dsm_cyclic_stop (unsigned long mask)
{
	int  ret;

	if ( (ret = ioctl(dsm_fd, DSM_IOCS_CYCLIC_STOP, mask)) )
		printf("DSM_IOCS_CYCLIC_STOP: %d: %s\n", ret, strerror(errno));

	return ret;
}

int dsm_cyclic_start (unsigned long mask)
{
	int  ret;

	if ( (ret = ioctl(dsm_fd, DSM_IOCS_CYCLIC_START, mask)) )
		printf("DSM_IOCS_CYCLIC_START: %d: %s\n", ret, strerror(errno));

	return ret;
}



