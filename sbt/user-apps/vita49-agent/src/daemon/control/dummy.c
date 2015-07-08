/** Dummy control
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
#include <sys/select.h>

#include <sbt_common/log.h>
#include <daemon/control.h>


LOG_MODULE_STATIC("control_dummy", LOG_LEVEL_WARN);


static struct control *control_dummy_alloc (void)
{
	ENTER("");
	struct control *ctrl = malloc(sizeof(struct control));
	if ( !ctrl )
		RETURN_VALUE("%p", NULL);

	memset (ctrl, 0, sizeof(struct control));

	RETURN_ERRNO_VALUE(0, "%p", ctrl);
}

static int control_dummy_config (const char *section, const char *tag, const char *val,
                               const char *file, int line, void *data)
{
	ENTER("section %s, tag %s, val %s, file %s, line %d, data %p",
	      section, tag, val, file, line, data);

	errno = 0;
	int ret = control_config_common(section, tag, val, file, line, (struct control *)data);
	RETURN_VALUE("%d", ret);
}

static int control_dummy_check (struct control *ctrl)
{
	ENTER("ctrl %p", ctrl);
	RETURN_VALUE("%d", 0);
}

static void control_dummy_fd_set (struct control *ctrl,
                                fd_set *rfds, fd_set *wfds, int *nfds)
{
	ENTER("ctrl %p, rfds %p, wfds %p, nfds %d", ctrl, rfds, wfds, *nfds);
	RETURN_ERRNO(ENOSYS);
}

static int control_dummy_fd_is_set (struct control *ctrl, fd_set *rfds, fd_set *wfds)
{
	ENTER("ctrl %p, rfds %p, wfds %p", ctrl, rfds, wfds);
	RETURN_ERRNO_VALUE(ENOSYS, "%d", 0);
}

static int control_dummy_read (struct control *ctrl, fd_set *rfds)
{
	ENTER("ctrl %p, rfds %p", ctrl, rfds);
	RETURN_ERRNO_VALUE(0, "%d", 0);
}

static int control_dummy_write (struct control *ctrl, fd_set *wfds)
{
	ENTER("ctrl %p, wfds %p", ctrl, wfds);
	RETURN_ERRNO_VALUE(ENOSYS, "%d", 0);
}

static void control_dummy_close (struct control *ctrl)
{
	ENTER("ctrl %p", ctrl);
	RETURN_ERRNO(ENOSYS);
}

static void control_dummy_free (struct control *ctrl)
{
	ENTER("ctrl %p", ctrl);
	free(ctrl);
	RETURN_ERRNO(0);
}


/** Operation pointers. */
static struct control_ops control_dummy_ops =
{
	alloc_fn:      control_dummy_alloc,
	config_fn:     control_dummy_config,
	check_fn:      control_dummy_check,
	fd_set_fn:     control_dummy_fd_set,
	fd_is_set_fn:  control_dummy_fd_is_set,
	read_fn:       control_dummy_read,
	write_fn:      control_dummy_write,
	close_fn:      control_dummy_close,
	free_fn:       control_dummy_free,
};
CONTROL_CLASS(dummy, control_dummy_ops);


/* Ends    : src/control/dummy.c */
