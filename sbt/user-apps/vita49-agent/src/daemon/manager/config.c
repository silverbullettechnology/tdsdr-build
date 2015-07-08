/** Daemon manager config callback
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
#include <sbt_common/log.h>
#include <sbt_common/mbuf.h>
#include <sbt_common/mqueue.h>
#include <daemon/worker.h>
#include <daemon/manager.h>


LOG_MODULE_IMPORT(daemon_manager_module_level);
#define module_level daemon_manager_module_level



int daemon_manager_config (const char *section, const char *tag, const char *val,
                           const char *file, int line, void *data)
{
	ENTER("section %s, tag %s, val %s, file %s, line %d, data %p",
	      section, tag, val, file, line, data);

	errno = 0;
	int ret = worker_config_common(section, tag, val, file, line, (struct worker *)data);
	RETURN_VALUE("%d", ret);
}


/* Ends    : src/daemon/worker/manager/config.c */
