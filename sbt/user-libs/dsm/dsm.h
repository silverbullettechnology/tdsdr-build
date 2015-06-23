/** \file      dsm.h
 *  \brief     interface declarations for DSM controls
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
#ifndef _INCLUDE_DSM_DSM_H_
#define _INCLUDE_DSM_DSM_H_

#include <dma_streamer_mod.h>


/** Limits struct read from kernel driver */
extern struct dsm_limits     dsm_limits;


/** List of available DMA channels read from kernel driver */
extern struct dsm_chan_list *dsm_channels;


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
int dsm_reopen (const char *node);


/** Close DSM connection */
void dsm_close (void);


/** Prepare a buffer for zero-copy userspace DMA
 *
 *  The buffer will be allocated with posix_memalign() using the system page size (usually
 *  4Kbytes) and locked into memory with mlock() before a successful return.
 *
 *  \param  size  Size of buffer in words (DSM_BUS_WIDTH bytes per word)
 *
 *  \return  Buffer pointer on success, NULL on failure
 */
void *dsm_user_alloc (size_t size);


/** Free a buffer for zero-copy userspace DMA
 *
 *  \param  addr  Buffer allocated with dsm_user_alloc()
 *  \param  size  Size of buffer in words (DSM_BUS_WIDTH bytes per word)
 */
void dsm_user_free (void *addr, size_t size);


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
int dsm_map_user (unsigned long slot, unsigned long tx, void *addr, unsigned long size);


/** Map multiple buffers for zero-copy userspace DMA
 *
 *  The DSM will map the physical pages which make up the buffer and allocate a
 *  scatterlist for zero-copy DMA.  Once a buffer is mapped it should be unmapped with a
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
                        unsigned long size, struct dsm_user_xfer *list);


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
int dsm_set_timeout   (unsigned long timeout);


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
int dsm_oneshot_start (unsigned long mask);


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
int dsm_oneshot_wait  (unsigned long mask);


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
int dsm_cyclic_stop   (unsigned long mask);


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
int dsm_cyclic_start  (unsigned long mask);


/** Allocate and return a structure with statistics from the most recent DMA transfers
 *
 *  The structure will be sized appropriately to the number of channels available
 *
 *  \note  The user is responsible for freeing the returned value with free()
 *
 *  \return  statistics structure on success, NULL on failure
 */
struct dsm_chan_stats *dsm_get_stats (void);


/** Unmap DSM-side mappings and allocations
 *
 *  The DSM will free the kernel-side structures and scatterlists for all channels setup
 *  with dsm_map_user() and dsm_map_user_array().  The user must still free buffers
 *  allocated with dsm_user_alloc().
 *
 *  \return  0 on success, !0 on failure
 */
int dsm_cleanup (void);


#endif /* _INCLUDE_DSM_DSM_H_ */
