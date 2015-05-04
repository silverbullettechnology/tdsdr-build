/** Worker configuration
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
#ifndef INCLUDE_WORKER_CONFIG_H
#define INCLUDE_WORKER_CONFIG_H
#include <lib/log.h>


/** Global first-pass config
 *
 *  \param path    Path to config file(s)
 *  \param section Section name to match on
 *
 *  \return 0 on success, !0 on failure
 */
int worker_config (const char *path, const char *section);


#endif // INCLUDE_WORKER_CONFIG_H