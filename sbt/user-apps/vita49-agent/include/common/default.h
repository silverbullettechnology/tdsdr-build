/** Common configuration defaults
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
#ifndef INCLUDE_COMMON_DEFAULT_H
#define INCLUDE_COMMON_DEFAULT_H


/* Currently sized for a maximum-length SRIO type 11 (message) packet with sd_user header
 * size reserved */
#define DEFAULT_MBUF_SIZE  5120
#define DEFAULT_MBUF_HEAD   256


#define DEF_CONFIG_PATH            "/etc/vita49-agent/daemon.conf"
#define DEF_RESOURCE_CONFIG_PATH   "/etc/vita49-agent/resource.conf"
#define DEF_WORKER_CLASS           "child"
#define DEF_WORKER_EXEC_PATH       "/usr/bin"
#define DEF_WORKER_FILENAME        "vita49-agent-worker"


#endif /* INCLUDE_COMMON_DEFAULT_H */
/* Ends */
