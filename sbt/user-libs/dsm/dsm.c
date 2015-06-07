/** \file      dsm.c
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

#include "dsm.h"


/** File handle for /dev/ node */
static int  dsm_fd = -1;


/** Limits struct read from kernel driver */
struct dsm_limits     dsm_limits;

/** List of available DMA channels read from kernel driver */
struct dsm_chan_list *dsm_channels = NULL;


/** Close DSM connection */
void dsm_close (void)
{
	if ( dsm_fd >= 0 )
		close(dsm_fd);
	dsm_fd = -1;
	free(dsm_channels);
}


/** Open or reopen DSM connection
 *
 *  The connection will be closed and dynamically-allocated objects freed before the open
 *  attempt.  The new open will also trigger a new DMA channel scan in the kernel driver,
 *  so if other drivers have been loaded/unloaded the list may change.
 *
 *  \param  node  Path to DSM module's device node
 *
 *  \return  0 on success, <0 on failure
 */
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


/** Prepare a buffer for zero-copy userspace DMA
 *
 *  The buffer will be allocated with posix_memalign() using the system page size (usually
 *  4Kbytes) and locked into memory with mlock() before a successful return.
 *
 *  \param  size  Size of buffer in words (DSM_BUS_WIDTH bytes per word)
 *
 *  \return  Buffer pointer on success, NULL on failure
 */
void *dsm_user_alloc (size_t size)
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


/** Free a buffer for zero-copy userspace DMA
 *
 *  \param  addr  Buffer allocated with dsm_user_alloc()
 *  \param  size  Size of buffer in words (DSM_BUS_WIDTH bytes per word)
 */
void dsm_user_free (void *addr, size_t size)
{
	// size specified in words, but munlock() uses bytes
	munlock(addr, size * DSM_BUS_WIDTH);
	free(addr);
}


/** Map multiple buffers for zero-copy userspace DMA
 *
 *  The DSM will map the physical pages which make up each buffer and allocate each as a
 *  scatterlist for zero-copy DMA.  Once buffers are mapped they should be unmapped with a
 *  call to dsm_cleanup() before closing the device node.
 *
 *  \note  size > 1 is currently not supported by DSM
 *
 *  \param  slot  DMA channel number, an index into dsm_chan_list->list[]
 *  \param  tx    !0 for TX, 0 for RX, must match dsm_chan_list->list[slot].flags
 *  \param  size  Number of elements in list[]
 *  \param  list  Array of xfer buffers to map
 *
 *  \return  0 on success, !0 on failure
 */
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


/** Map a buffer for zero-copy userspace DMA
 *
 *  The DSM will map the physical pages which make up the buffer and allocate a single
 *  scatterlist for zero-copy DMA.  Once a buffer is mapped it should be unmapped with a
 *  call to dsm_cleanup() before closing the device node.
 *
 *  \note  size > 1 is currently not supported by DSM
 *
 *  \param  slot  DMA channel number, an index into dsm_chan_list->list[]
 *  \param  tx    !0 for TX, 0 for RX, must match dsm_chan_list->list[slot].flags
 *  \param  addr  Page-aligned buffer allocated with dsm_user_alloc()
 *  \param  size  Size of buffer in words
 *
 *  \return  0 on success, !0 on failure
 */
int dsm_map_user (unsigned long slot, unsigned long tx, void *addr, unsigned long size)
{
	struct dsm_user_xfer xfer =
	{
		.addr = (unsigned long)addr,
		.size = size,
	};

	return dsm_map_user_array(slot, tx, 1, &xfer);
}


/** Unmap DSM-side mappings and allocations
 *
 *  The DSM will free the kernel-side structures and scatterlists for all channels setup
 *  with dsm_map_user() and dsm_map_user_array().  The user must still free buffers
 *  allocated with dsm_user_alloc().
 *
 *  \return  0 on success, !0 on failure
 */
int dsm_cleanup (void)
{
	int ret;

	errno = 0;
	if ( (ret = ioctl(dsm_fd, DSM_IOCS_CLEANUP, 0)) )
		printf("DSM_IOCS_CLEANUP: %d: %s\n", ret, strerror(errno));

	return ret;
}


/** Set software timeout for DMA transfers in jiffies
 *
 *  DSM sets up a software timeout for each DMA transfer in case the target stalls or
 *  otherwise fails without an interrupt.  Note that after a timeout the system can be
 *  unstable and require a reboot.
 *
 *  \param  timeout  Timeout in jiffies (typically 10ms each)
 *
 *  \return  0 on success, !0 on failure
 */
int dsm_set_timeout (unsigned long timeout)
{
	int ret;

	errno = 0;
	if ( (ret = ioctl(dsm_fd, DSM_IOCS_TIMEOUT, timeout)) )
		printf("DSM_IOCS_TIMEOUT %d: %d: %s\n", timeout, ret, strerror(errno));

	return ret;
}


/** Start DMA in oneshot mode with bitmask
 *
 *  This will start some or all of the active channels (setup with dsm_user_map() or
 *  dsm_user_map_array()) kernel threads.  The user should set a bit in mask for each 
 *  channel to start; ie to start channels 2 & 3 mask should be ((1 << 2) | (1 << 3)).
 *
 *  \note  DSM constrains mask to the channels which are active, so passing ~0 for mask is
 *         a convenient way to start all active channels at once.
 *
 *  \param  mask  Bitmask of DMA channels to start
 *
 *  \return  0 on success, !0 on failure
 */
int dsm_oneshot_start (unsigned long mask)
{
	int ret;

	if ( (ret = ioctl(dsm_fd, DSM_IOCS_ONESHOT_START, mask)) )
		printf("DSM_IOCS_ONESHOT_START: %d: %s\n", ret, strerror(errno));

	return ret;
}


/** Wait for oneshot DMA to complete with bitmask
 *
 *  This will block until all of the active channels (setup with dsm_user_map() or 
 *  dsm_user_map_array()) complete or timeout.  The user should set a bit in mask for each
 *  channel to wait for; ie to start channels 2 & 3 mask should be ((1 << 2) | (1 << 3)).
 *
 *  \note  DSM constrains mask to the channels which are active, so passing ~0 for mask is
 *         a convenient way to start all active channels at once.
 *  \note  If the DMA has completed before this call is made, it returns immediate success
 *
 *  \param  mask  Bitmask of DMA channels to wait for
 *
 *  \return  0 on success, !0 on failure
 */
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


/** Signal cyclic DMA threads to finish and wait for complete
 *
 *  This will set a flag on each cyclic channel (according to mask) which signals the
 *  thread to exit at the end if the current cycle, then will block until all of the
 *  channels complete or timeout.  
 *
 *  \note  DSM constrains mask to the channels which are active, so passing ~0 for mask is
 *         a convenient way to start all active channels at once.
 *  \note  If the DMA has completed before this call is made, it returns immediate success
 *
 *  \param  mask  Bitmask of DMA channels to stop and wait for
 *
 *  \return  0 on success, !0 on failure
 */
int dsm_cyclic_stop (unsigned long mask)
{
	int  ret;

	if ( (ret = ioctl(dsm_fd, DSM_IOCS_CYCLIC_STOP, mask)) )
		printf("DSM_IOCS_CYCLIC_STOP: %d: %s\n", ret, strerror(errno));

	return ret;
}


/** Start DMA in cyclic mode with bitmask
 *
 *  This will start some or all of the active channels (setup with dsm_user_map() or
 *  dsm_user_map_array()) kernel threads.  The channels will run repeatedly until a call
 *  to dsm_cyclic_stop() stops them.
 *
 *  \note  DSM constrains mask to the channels which are active, so passing ~0 for mask is
 *         a convenient way to start all active channels at once.
 *
 *  \param  mask  Bitmask of DMA channels to start
 *
 *  \return  0 on success, !0 on failure
 */
int dsm_cyclic_start (unsigned long mask)
{
	int  ret;

	if ( (ret = ioctl(dsm_fd, DSM_IOCS_CYCLIC_START, mask)) )
		printf("DSM_IOCS_CYCLIC_START: %d: %s\n", ret, strerror(errno));

	return ret;
}



