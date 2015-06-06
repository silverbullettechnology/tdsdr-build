/** \file      pipe/access.h
 *  \brief     interface declarations for Access control IOCTLs
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
#ifndef _INCLUDE_PIPE_ACCESS_H_
#define _INCLUDE_PIPE_ACCESS_H_


int pipe_access_avail   (unsigned long *bits);
int pipe_access_request (unsigned long  bits);
int pipe_access_release (unsigned long  bits);


#endif // _INCLUDE_PIPE_ACCESS_H_
