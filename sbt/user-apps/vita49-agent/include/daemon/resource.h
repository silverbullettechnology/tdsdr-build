/** 
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
#ifndef INCLUDE_DAEMON_RESOURCE_H
#define INCLUDE_DAEMON_RESOURCE_H
#include <stdint.h>

#include <lib/uuid.h>
#include <lib/growlist.h>

#include <common/resource.h>


extern char *resource_config_path;

extern struct growlist  resource_list;


/** Read resource database from file
 *
 *  \param path  Path to resource config file
 *
 *  \return 0 on success, !0 on failure
 */
int resource_config (const char *path);


#endif // INCLUDE_DAEMON_RESOURCE_H

