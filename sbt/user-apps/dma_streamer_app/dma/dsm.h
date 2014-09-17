/** \file      dma/dsm.h
 *  \brief     interface declarations for common FIFO / DSM controls
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
#ifndef _INCLUDE_DMA_DSM_H_
#define _INCLUDE_DMA_DSM_H_

#include <dma_streamer_mod.h>


extern struct dsm_limits     dsm_limits;
extern struct dsm_chan_list *dsm_channels;


void dsm_close (void);
int dsm_reopen (const char *node);


void *dsm_alloc (size_t size);
void dsm_free (void *addr, size_t size);


int dsm_map_array (unsigned long slot, unsigned long tx,
                   unsigned long size, struct dsm_xfer_buff *list);

int dsm_map_single (unsigned long slot, unsigned long tx, void *addr,
                    unsigned long size, unsigned long reps);

struct dsm_chan_stats *dsm_get_stats (void);

int dsm_unmap            (void);
int dsm_set_timeout      (unsigned long timeout);
int dsm_oneshot_start    (unsigned long mask);
int dsm_oneshot_wait     (unsigned long mask);
int dsm_continuous_stop  (unsigned long mask);
int dsm_continuous_start (unsigned long mask);



#endif // _INCLUDE_DMA_DSM_H_
