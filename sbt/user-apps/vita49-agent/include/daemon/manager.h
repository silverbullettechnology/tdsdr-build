/** Daemon manager prototypes
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
#ifndef INCLUDE_DAEMON_MANAGER_H
#define INCLUDE_DAEMON_MANAGER_H

#include <vita49/types.h>
#include <vita49/common.h>
#include <vita49/command.h>
#include <vita49/context.h>
#include <vita49/control.h>

#include <daemon/message.h>
#include <daemon/worker.h>


void daemon_manager_control_recv (struct v49_control *ctrl);

void daemon_manager_command_recv (struct v49_common *req_v49, struct message *req_user,
                                  struct worker *worker);

int daemon_manager_config (const char *section, const char *tag, const char *val,
                           const char *file, int line, void *data);


#endif /* INCLUDE_DAEMON_MANAGER_H */
/* Ends */
