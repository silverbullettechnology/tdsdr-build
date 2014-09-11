/** \file      dsa_ioctl_dsm.h
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
#ifndef _INCLUDE_DSA_IOCTL_DSM_H_
#define _INCLUDE_DSA_IOCTL_DSM_H_

#include <dma_streamer_mod.h>

struct dsm_chan_list *dsa_ioctl_dsm_channels (void);

int dsa_ioctl_dsm_map              (struct dsm_chan_buffs *buffs);
int dsa_ioctl_dsm_unmap            (void);
int dsa_ioctl_dsm_set_timeout      (unsigned long timeout);
int dsa_ioctl_dsm_oneshot_start    (void);
int dsa_ioctl_dsm_oneshot_wait     (void);
int dsa_ioctl_dsm_get_stats        (struct dsm_chan_stats *sb);
int dsa_ioctl_dsm_continuous_stop  (void);
int dsa_ioctl_dsm_continuous_start (void);


#endif // _INCLUDE_DSA_IOCTL_DSM_H_
