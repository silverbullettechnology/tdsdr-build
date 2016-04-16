/** \file      src/lib/dev.c
 *  \brief     implementation of pipe-dev controls
 *  \copyright Copyright 2013,2014 Silver Bullet Technology
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
 * vim:ts=4:noexpandtab
 */
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#include <pd_main.h>

#include "pipe_dev.h"
#include "private.h"


int   pipe_dev_fd  = -1;
FILE *pipe_dev_err = NULL;


void pipe_dev_close (void)
{
	if ( pipe_dev_fd >= 0 )
		close(pipe_dev_fd);
	pipe_dev_fd = -1;
}


int pipe_dev_reopen (const char *node)
{
	pipe_dev_close();

	errno = 0;
	if ( (pipe_dev_fd = open(node, O_RDWR|O_NONBLOCK)) < 0 )
		return pipe_dev_fd;

//	fl_set = O_NONBLOCK
//	fd_set = FD_CLOEXEC

	return 0;
}

void pipe_dev_error (FILE *fp)
{
	pipe_dev_err = fp;
}

