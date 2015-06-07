/** \file      pipe/vita49_pack.h
 *  \brief     interface declarations for VITA49_PACK IOCTLs
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
#ifndef _INCLUDE_PIPE_VITA49_PACK_H_
#define _INCLUDE_PIPE_VITA49_PACK_H_


int pipe_vita49_pack_set_ctrl     (int dev, unsigned long  reg);
int pipe_vita49_pack_get_ctrl     (int dev, unsigned long *reg);
int pipe_vita49_pack_get_stat     (int dev, unsigned long *reg);
int pipe_vita49_pack_set_streamid (int dev, unsigned long  reg);
int pipe_vita49_pack_get_streamid (int dev, unsigned long *reg);
int pipe_vita49_pack_set_pkt_size (int dev, unsigned long  reg);
int pipe_vita49_pack_get_pkt_size (int dev, unsigned long *reg);
int pipe_vita49_pack_set_trailer  (int dev, unsigned long  reg);
int pipe_vita49_pack_get_trailer  (int dev, unsigned long *reg);


#endif // _INCLUDE_PIPE_VITA49_PACK_H_
