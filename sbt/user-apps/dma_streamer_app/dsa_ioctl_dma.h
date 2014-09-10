/** \file      dsa_ioctl.h
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
#ifndef _INCLUDE_DSA_IOCTL_H_
#define _INCLUDE_DSA_IOCTL_H_

#include <dma_streamer_mod.h>

int dsa_ioctl_map (struct dsm_user_buffs *buffs);
int dsa_ioctl_unmap (void);
int dsa_ioctl_target_list (unsigned long *mask);
int dsa_ioctl_set_timeout (unsigned long timeout);
int dsa_ioctl_trigger (void);
int dsa_ioctl_get_stats (struct dsm_user_stats *sb);


#endif // _INCLUDE_DSA_IOCTL_H_
