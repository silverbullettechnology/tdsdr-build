/** Log glue
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


size_t _v49_client_log_library_reg (struct log_module_map_t *dest)
{
	/** Linker-generated symbols for modules inside the library */
	extern struct log_module_map_t __start_log_module_map;
	extern struct log_module_map_t  __stop_log_module_map;  

	size_t num = &__stop_log_module_map - &__start_log_module_map;

	if ( dest )
		memcpy(dest, &__start_log_module_map, sizeof(struct log_module_map_t) * num);

	return num;
}

