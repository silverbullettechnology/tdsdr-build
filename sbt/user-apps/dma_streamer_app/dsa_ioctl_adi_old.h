/** \file      dsa_ioctl_adi_old.h
 *  \brief     interface declarations for legacy ADI FIFO controls
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
#ifndef _INCLUDE_DSA_IOCTL_ADI_OLD_H_
#define _INCLUDE_DSA_IOCTL_ADI_OLD_H_

#include <dma_streamer_mod.h>


int dsa_ioctl_adi_old_set_ctrl     (int dev, unsigned long reg);
int dsa_ioctl_adi_old_get_ctrl     (int dev, unsigned long *reg);
int dsa_ioctl_adi_old_set_tx_cnt   (int dev, unsigned long len, unsigned long reps);
int dsa_ioctl_adi_old_get_tx_cnt   (int dev, unsigned long *reg);
int dsa_ioctl_adi_old_set_rx_cnt   (int dev, unsigned long len, unsigned long reps);
int dsa_ioctl_adi_old_get_rx_cnt   (int dev, unsigned long *reg);
int dsa_ioctl_adi_old_get_sum      (int dev, unsigned long *sum);
int dsa_ioctl_adi_old_get_last     (int dev, unsigned long *last);
int dsa_ioctl_adi_old_get_fifo_cnt (struct dsm_fifo_counts *fb);


#endif // _INCLUDE_DSA_IOCTL_ADI_OLD_H_
