/** Local daemon configuration
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
#include <sbt_common/growlist.h>

#include <common/default.h>
#include <vita49/resource.h>

#include <daemon/resource.h>
#include <daemon/worker.h>
#include <daemon/control.h>
#include <daemon/config.h>
#include <daemon/manager.h>


LOG_MODULE_STATIC("daemon_config", LOG_LEVEL_WARN);



/** Global section item handler
 */
static int daemon_config_global (const char *section, const char *tag, const char *val,
                                const char *file, int line, void *data)
{
	ENTER("section %s, tag %s, val %s, file %s, line %d, data %p",
	      section, tag, val, file, line, data);
RETURN_ERRNO_VALUE(0, "%d", 0);

	if ( !tag || !val )	RETURN_ERRNO_VALUE(0, "%d", 0);

	RETURN_ERRNO_VALUE(ENOENT, "%d", -1);
}


/** Paths section item handler
 */
static int daemon_config_paths (const char *section, const char *tag, const char *val,
                                const char *file, int line, void *data)
{
	ENTER("section %s, tag %s, val %s, file %s, line %d, data %p",
	      section, tag, val, file, line, data);

	if ( !tag || !val )
		RETURN_ERRNO_VALUE(0, "%d", 0);
	
	if ( !strcmp(tag, "resource") )
	{
		free(resource_config_path);
		resource_config_path = strdup(val);
		RETURN_ERRNO_VALUE(0, "%d", 0);
	}

	if ( !strcmp(tag, "worker") )
	{
		free(worker_exec_path);
		worker_exec_path = strdup(val);
		RETURN_ERRNO_VALUE(0, "%d", 0);
	}


	RETURN_ERRNO_VALUE(ENOENT, "%d", -1);
}


/* State struct used in daemon_config_section(); accumulates section entries which will be
 * either control or worker instances. */
struct daemon_config_section_state
{
	char *name;
	char *worker;
	char *control;
};


/** Wildcard section handler for both controls and workers.  Section specifies the
 *  instance name, while either a "control" or "worker" specifies the class name.
 */
static int daemon_config_section (const char *section, const char *tag, const char *val,
                                 const char *file, int line, void *data)
{
	ENTER("section %s, tag %s, val %s, file %s, line %d, data %p",
	      section, tag, val, file, line, data);

	// section entry/exit is where we 
	struct daemon_config_section_state *ss = (struct daemon_config_section_state *)data;
	if ( !tag )
	{
		// entering section: latch instance name
		if ( val )
		{
			free (ss->name);
			ss->name = strdup(section);
		}
		// leaving section: 
		else 
		{
			// instance name and control class specified: try to allocate
			if ( ss->name && ss->control )
			{
				// note: control_alloc adds the control to its control_list internally
				struct control *ctrl = control_alloc(ss->control);
				if ( !ctrl )
				{
					LOG_ERROR("%s[%d]: Failed to allocate control %s: %s\n", file, line,
					          section, strerror(errno));
					RETURN_VALUE("%d", -1);
				}

				// success: detach name from scan state and attach to new instance
				ctrl->name = ss->name;
				ss->name = NULL;
				LOG_DEBUG("%s[%d]: name %s control %s allocated %p\n", file, line,
				          control_name(ctrl), ctrl->class->name, ctrl);
			}

			// instance name and control class specified: try to allocate
			else if ( ss->name && ss->worker )
			{
				// note: worker_alloc adds the worker to its worker_list internally
				struct worker *worker = worker_alloc(ss->worker, 0);
				if ( !worker )
				{
					LOG_ERROR("%s[%d]: Failed to allocate worker %s: %s\n", file, line,
					          section, strerror(errno));
					RETURN_VALUE("%d", -1);
				}

				// success: detach name from scan state and attach to new instance
				worker->name = ss->name;
				ss->name = NULL;
				LOG_DEBUG("%s[%d]: name %s worker %s allocated %p\n", file, line,
				          worker_name(worker), worker->class->name, worker);
			}

			// ignore incomplete section
			else 
			{
				LOG_WARN("%s[%d]: Incomplete section '%s' ignored\n", file, line,
				         section);
				RETURN_ERRNO_VALUE(0, "%d", 0);
			}

			// reset scan state
			free (ss->worker);
			free (ss->control);
			ss->worker  = NULL;
			ss->control = NULL;
		}
		RETURN_ERRNO_VALUE(0, "%d", 0);
	}

	// got a control= tag: latch value, try as a control class
	if ( !strcmp(tag, "control") )
	{
		ss->control = strdup(val);
		free (ss->worker);
		ss->worker = NULL;
		RETURN_ERRNO_VALUE(0, "%d", 0);
	}

	// got a worker= tag: latch value, try as a worker class
	if ( !strcmp(tag, "worker") )
	{
		ss->worker = strdup(val);
		free (ss->control);
		ss->control = NULL;
		RETURN_ERRNO_VALUE(0, "%d", 0);
	}

	RETURN_ERRNO_VALUE(0, "%d", 0);
}


/** Global first-pass config
 *
 *  \param path  Path to config file(s)
 *
 *  \return 0 on success, !0 on failure
 */
int daemon_config (const char *path)
{
	ENTER("path %s", path);

	// setting default values for items which can be changed with libconfig 
	resource_config_path = strdup(DEF_RESOURCE_CONFIG_PATH);
	worker_exec_path     = strdup(DEF_WORKER_EXEC_PATH);

	static struct config_section_map cm[] =
	{
		// first configure sections related to overall application 
		{ "global",  daemon_config_global,   NULL },
		{ "paths",   daemon_config_paths,    NULL },
		{ "log",     log_config,             NULL },
		{ "manager", daemon_manager_config,  NULL },

		// catchall configures both local workers and controls
		{ "*",      daemon_config_section, NULL },
		{ NULL }
	};
	static struct daemon_config_section_state ss;
	static struct config_context cc =
	{
		.default_section   = "global",
		.default_function  = daemon_config_global,
		.catchall_function = NULL,
		.section_map       = cm,
		.data              = &ss 
	};

	// setup section state for locating control/worker sections
	ss.name    = NULL;
	ss.worker  = NULL;
	ss.control = NULL;
	growlist_init (&worker_list,  4, 4);
	growlist_init (&control_list, 4, 4);

	errno = 0;
	int ret = config_parse (&cc, path);

	RETURN_VALUE("%d", ret);
}


/* Ends    : src/daemon/config.c */
