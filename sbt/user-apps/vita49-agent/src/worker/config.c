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
#include <config/include/config.h>

#include <lib/log.h>
#include <lib/growlist.h>

#include <worker/config.h>


LOG_MODULE_STATIC("worker_config", LOG_LEVEL_WARN);


/** Global section item handler
 */
static int worker_config_cb (const char *section, const char *tag, const char *val,
                             const char *file, int line, void *data)
{
	ENTER("section %s, tag %s, val %s, file %s, line %d, data %p",
	      section, tag, val, file, line, data);
RETURN_ERRNO_VALUE(0, "%d", 0);

	if ( !tag || !val )	RETURN_ERRNO_VALUE(0, "%d", 0);

	RETURN_ERRNO_VALUE(ENOENT, "%d", -1);
}


/** Global first-pass config
 *
 *  \param path    Path to config file(s)
 *  \param section Section name to match on
 *
 *  \return 0 on success, !0 on failure
 */
int worker_config (const char *path, const char *section)
{
	ENTER("path %s, section %s", path, section);

	struct config_section_map cm[] =
	{
		// first configure sections related to overall application 
		{ section,  worker_config_cb,  NULL },
		{ NULL }
	};
	struct config_context cc =
	{
		.default_section   = "global",
		.default_function  = NULL,
		.catchall_function = NULL,
		.section_map       = cm,
		.data              = NULL,
	};

	errno = 0;
	int ret = config_parse (&cc, path);

	RETURN_VALUE("%d", ret);
}


/* Ends    : src/worker/config.c */

