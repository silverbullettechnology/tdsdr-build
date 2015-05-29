/** \file      fifo/dsxx.h
 *  \brief     interface declarations for data source/sink controls
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
#ifndef _INCLUDE_FIFO_DSXX_H_
#define _INCLUDE_FIFO_DSXX_H_


int fifo_dsrc_set_ctrl  (int dev, unsigned long  reg);
int fifo_dsrc_get_stat  (int dev, unsigned long *reg);
int fifo_dsrc_set_bytes (int dev, unsigned long  reg);
int fifo_dsrc_get_bytes (int dev, unsigned long *reg);
int fifo_dsrc_get_sent  (int dev, unsigned long *reg);
int fifo_dsrc_set_type  (int dev, unsigned long  reg);
int fifo_dsrc_get_type  (int dev, unsigned long *reg);
int fifo_dsrc_set_reps  (int dev, unsigned long  reg);
int fifo_dsrc_get_reps  (int dev, unsigned long *reg);
int fifo_dsrc_get_rsent (int dev, unsigned long *reg);

int fifo_dsnk_set_ctrl  (int dev, unsigned long  reg);
int fifo_dsnk_get_stat  (int dev, unsigned long *reg);
int fifo_dsnk_get_bytes (int dev, unsigned long *reg);
int fifo_dsnk_get_sum   (int dev, unsigned long *reg);


#endif // _INCLUDE_FIFO_DSXX_H_
