/** \file      pipe/vita49_clk.h
 *  \brief     interface declarations for VITA49 CLOCK IOCTLs
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
#ifndef _INCLUDE_PIPE_VITA49_CLK_H_
#define _INCLUDE_PIPE_VITA49_CLK_H_
#include <pd_main.h>


int pipe_vita49_clk_set_ctrl (unsigned long  reg);
int pipe_vita49_clk_get_ctrl (unsigned long *reg);
int pipe_vita49_clk_get_stat (unsigned long *reg);
int pipe_vita49_clk_set_tsi  (unsigned long  reg);
int pipe_vita49_clk_get_tsi  (unsigned long *reg);
int pipe_vita49_clk_read     (int dev, struct pd_vita49_ts *ts);


#endif // _INCLUDE_PIPE_VITA49_CLK_H_
