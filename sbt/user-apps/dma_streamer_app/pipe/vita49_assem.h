/** \file      pipe/vita49_assem.h
 *  \brief     interface declarations for VITA49_ASSEM IOCTLs
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
#ifndef _INCLUDE_PIPE_VITA49_ASSEM_H_
#define _INCLUDE_PIPE_VITA49_ASSEM_H_


int pipe_vita49_assem_set_cmd     (int dev, unsigned long reg);
int pipe_vita49_assem_get_cmd     (int dev, unsigned long *reg);
int pipe_vita49_assem_set_hdr_err (int dev, unsigned long reg);
int pipe_vita49_assem_get_hdr_err (int dev, unsigned long *reg);


#endif // _INCLUDE_PIPE_VITA49_ASSEM_H_
