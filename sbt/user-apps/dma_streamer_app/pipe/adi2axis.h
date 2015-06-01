/** \file      pipe/adi2axis.h
 *  \brief     interface declarations for ADI2AXIS IOCTLs
 *  \copyright Copyright 2015 Silver Bullet Technology
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
#ifndef _INCLUDE_PIPE_ADI2AXIS_H_
#define _INCLUDE_PIPE_ADI2AXIS_H_


int pipe_adi2axis_set_ctrl  (int dev, unsigned long  reg);
int pipe_adi2axis_get_ctrl  (int dev, unsigned long *reg);
int pipe_adi2axis_get_stat  (int dev, unsigned long *reg);
int pipe_adi2axis_set_bytes (int dev, unsigned long  reg);
int pipe_adi2axis_get_bytes (int dev, unsigned long *reg);


#endif // _INCLUDE_PIPE_ADI2AXIS_H_
