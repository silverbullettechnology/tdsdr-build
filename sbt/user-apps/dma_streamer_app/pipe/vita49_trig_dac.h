/** \file      pipe/vita49_trig_dac.h
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
#ifndef _INCLUDE_PIPE_VITA49_TRIG_DAC_H_
#define _INCLUDE_PIPE_VITA49_TRIG_DAC_H_
#include <pd_main.h>


int pipe_vita49_trig_dac_set_ctrl (int dev, unsigned long  reg);
int pipe_vita49_trig_dac_get_ctrl (int dev, unsigned long *reg);
int pipe_vita49_trig_dac_get_stat (int dev, unsigned long *reg);
int pipe_vita49_trig_dac_set_ts   (int dev, struct pd_vita49_ts *buf);
int pipe_vita49_trig_dac_get_ts   (int dev, struct pd_vita49_ts *buf);


#endif // _INCLUDE_PIPE_VITA49_TRIG_DAC_H_
