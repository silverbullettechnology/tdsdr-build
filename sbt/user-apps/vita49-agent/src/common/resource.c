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
#include <stdint.h>
#include <string.h>

#include <config/include/config.h>

#include <lib/log.h>
#include <lib/uuid.h>
#include <lib/growlist.h>

#include <common/resource.h>


LOG_MODULE_STATIC("resource", LOG_LEVEL_DEBUG);



void resource_dump (int level, const char *msg, struct resource_info *res)
{
	LOG_MESSAGE(level, "%suuid %s txch %u rxch %u rate %u.%03u min %u max %u\n",
	            msg, uuid_to_str(&res->uuid),
	            res->txch, res->rxch,
	            res->rate_q8 >> 8, (1000 * (res->rate_q8 & 255)) >> 8,
	            res->min, res->max);
}


/* Ends */
