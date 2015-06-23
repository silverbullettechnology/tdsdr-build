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

#include <config/config.h>

#include <sbt_common/log.h>
#include <sbt_common/uuid.h>
#include <sbt_common/growlist.h>

#include <dsa_util.h>

#include <v49_message/resource.h>


LOG_MODULE_STATIC("resource", LOG_LEVEL_DEBUG);


char            *resource_config_path;
struct growlist  resource_list = GROWLIST_INIT;


/** Wildcard section handler for both controls and workers.  Section specifies the
 *  instance name, while either a "control" or "worker" specifies the class name.
 */
static int resource_config_line (const char *section, const char *tag, const char *val,
                                 const char *file, int line, void *data)
{
	ENTER("section %s, tag %s, val %s, file %s, line %d, data %p",
	      section, tag, val, file, line, data);

	struct resource_info *tmp = (struct resource_info*)data;
	struct resource_info *ptr;

	// section entry/exit is where we allocate and insert into list
	if ( !tag )
	{
		// entering section: parse UUID into temp struct
		if ( val )
		{
			// parse UUID hex and 
			if ( uuid_from_str(&tmp->uuid, section) )
			{
				LOG_ERROR("%s[%d]: UUID '%s' invalid\n", file, line, section);
				RETURN_ERRNO_VALUE(EINVAL, "%d", 1);
			}

			// check for duplicates early on
			growlist_reset(&resource_list);
			if ( growlist_search(&resource_list, uuid_cmp, &tmp->uuid) )
			{
				LOG_ERROR("%s[%d]: duplicate section\n", file, line);
				RETURN_ERRNO_VALUE(EEXIST, "%d", 1);
			}

			LOG_DEBUG("Enter section: %s / %s\n", section, uuid_to_str(&tmp->uuid));
		}

		// leaving section: alloc and insert into list
		else 
		{
			LOG_DEBUG("Leave section: \n");
			resource_dump(LOG_LEVEL_DEBUG, "  ", tmp);

			if ( !(ptr = malloc(sizeof(*ptr))) )
			{
				LOG_ERROR("Failed to alloc for resource %s\n", uuid_to_str(&tmp->uuid));
				RETURN_VALUE("%d", 1);
			}
			memcpy(ptr, tmp, sizeof(*ptr));

			if ( growlist_append(&resource_list, ptr) < 0 )
			{
				LOG_ERROR("Failed to insert new resource %s\n", uuid_to_str(&tmp->uuid));
				free(ptr);
				RETURN_VALUE("%d", 1);
			}
			memset(tmp, 0, sizeof(*tmp));
		}
		RETURN_ERRNO_VALUE(0, "%d", 0);
	}

	if ( !strcmp(tag, "copy") )
	{
		uuid_t uuid;
		if ( uuid_from_str(&uuid, val) )
		{
			LOG_ERROR("%s[%d]: UUID '%s' invalid\n", file, line, val);
			RETURN_ERRNO_VALUE(EINVAL, "%d", 1);
		}

		growlist_reset(&resource_list);
		if ( !(ptr = growlist_search(&resource_list, uuid_cmp, &uuid)) )
		{
			LOG_ERROR("%s[%d]: UUID '%s' not found\n", file, line, val);
			RETURN_ERRNO_VALUE(ENOENT, "%d", 1);
		}
		LOG_DEBUG("%s[%d]: Template %s copied\n", file, line, uuid_to_str(&uuid));

		// save the tmp uuid locally since it'll get wiped out by the template
		memcpy(&uuid, &tmp->uuid, sizeof(uuid));
		memcpy(tmp, ptr, sizeof(*tmp));
		memcpy(&tmp->uuid, &uuid, sizeof(uuid));

		LOG_DEBUG("%s[%d]: Resource %s cloned\n", file, line, uuid_to_str(&uuid));
		RETURN_ERRNO_VALUE(0, "%d", 0);
	}

	if ( !strcmp(tag, "name") )
	{
		memset(tmp->name, 0, sizeof(tmp->name));
		strncpy(tmp->name, val, 16);
		LOG_DEBUG("%s[%d]: Resource %s name '%s'\n", file, line,
		          uuid_to_str(&tmp->uuid), tmp->name);
		RETURN_ERRNO_VALUE(0, "%d", 0);
	}

	if ( !strcmp(tag, "txch") )
	{
		tmp->txch = strtoul(val, NULL, 0);
		LOG_DEBUG("%s[%d]: Resource %s txch %u\n", file, line,
		          uuid_to_str(&tmp->uuid), tmp->txch);
		RETURN_ERRNO_VALUE(0, "%d", 0);
	}

	if ( !strcmp(tag, "rxch") )
	{
		tmp->rxch = strtoul(val, NULL, 0);
		LOG_DEBUG("%s[%d]: Resource %s rxch %u\n", file, line,
		          uuid_to_str(&tmp->uuid), tmp->rxch);
		RETURN_ERRNO_VALUE(0, "%d", 0);
	}

	if ( !strcmp(tag, "rate") )
	{
		tmp->rate_q8 = strtoul(val, NULL, 0) << 8;
		LOG_DEBUG("%s[%d]: Resource %s rate %u.0\n", file, line,
		          uuid_to_str(&tmp->uuid), tmp->rate_q8 >> 8);
		RETURN_ERRNO_VALUE(0, "%d", 0);
	}

	if ( !strcmp(tag, "min") )
	{
		tmp->min = strtoul(val, NULL, 0);
		LOG_DEBUG("%s[%d]: Resource %s min %u\n", file, line,
		          uuid_to_str(&tmp->uuid), tmp->min);
		RETURN_ERRNO_VALUE(0, "%d", 0);
	}

	if ( !strcmp(tag, "max") )
	{
		tmp->max = strtoul(val, NULL, 0);
		LOG_DEBUG("%s[%d]: Resource %s max %u\n", file, line,
		          uuid_to_str(&tmp->uuid), tmp->max);
		RETURN_ERRNO_VALUE(0, "%d", 0);
	}

	if ( !strcmp(tag, "spec") )
	{
		int ident = dsa_util_spec_parse(val);
		if ( ident < 0 )
		{
			LOG_ERROR("%s[%d]: spec '%s' invalid\n", file, line, val);
			RETURN_ERRNO_VALUE(EINVAL, "%d", 1);
		}

		tmp->dc_bits = ident;
		tmp->fd_bits = dsa_util_fd_access(ident);

		LOG_DEBUG("%s[%d]: Resource %s spec '%s' -> %s, fd_bits 0x%lx\n", file, line,
		          uuid_to_str(&tmp->uuid), val, dsa_util_chan_desc(ident), tmp->fd_bits);
		RETURN_ERRNO_VALUE(0, "%d", 0);
	}

	RETURN_ERRNO_VALUE(0, "%d", 0);
}


/** Read resource database from file
 *
 *  \param path  Path to resource config file
 *
 *  \return 0 on success, !0 on failure
 */
int resource_config (const char *path)
{
	static struct resource_info   tmp;
	static struct config_context  cc =
	{
		.default_section   = NULL,
		.default_function  = NULL,
		.catchall_function = resource_config_line,
		.section_map       = NULL,
		.data              = &tmp,
	};

	ENTER("path %s", path);

	// setup section state for locating control/worker sections
	growlist_init (&resource_list,  4, 4);

	errno = 0;
	int ret = config_parse (&cc, path);

	RETURN_VALUE("%d", ret);
}


void resource_dump (int level, const char *msg, struct resource_info *res)
{
	LOG_MESSAGE(level, "%s%s: uuid %s txch %u rxch %u rate %u.%03u min %u max %u\n",
	            msg, res->name, uuid_to_str(&res->uuid),
	            res->txch, res->rxch,
	            res->rate_q8 >> 8, (1000 * (res->rate_q8 & 255)) >> 8,
	            res->min, res->max);
}


/* Ends */
