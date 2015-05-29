/** \file      fifo/dev.h
 *  \brief     interface declarations for common FIFO controls
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
#ifndef _INCLUDE_FIFO_DEV_H_
#define _INCLUDE_FIFO_DEV_H_

#include <fd_main.h>


extern unsigned long  fifo_dev_target_mask;


int  fifo_dev_reopen (const char *node);
void fifo_dev_close  (void);


const char *fifo_dev_target_desc (unsigned long mask);


#endif // _INCLUDE_FIFO_DEV_H_
