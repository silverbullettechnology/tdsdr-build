/** Worker command message handling
 *
 *  \author    Morgan Hughes <morgan.hughes@silver-bullet-tech.com>
 *  \version   v0.0
 *  \copyright Copyright 2014,2015 Silver Bullet Technology
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
 *  vim:ts=4:noexpandtab
 */
#ifndef INCLUDE_WORKER_COMMAND_H
#define INCLUDE_WORKER_COMMAND_H

#include <common/vita49/types.h>
#include <common/vita49/common.h>


/** Receive a command packet from a client
 *
 *  \param  req_v49  Parsed command structure
 */
void worker_command_recv (struct v49_common *req_v49);


#endif // INCLUDE_WORKER_COMMAND_H
