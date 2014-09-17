/** \file      fifo/dev.c
 *  \brief     implementation of FIFO controls
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

#include <fd_main.h>

#include "fifo/dev.h"
#include "fifo/private.h"


int            fifo_dev_fd = -1;
unsigned long  fifo_dev_target_mask = 0;


void fifo_dev_close (void)
{
	if ( fifo_dev_fd >= 0 )
		close(fifo_dev_fd);
	fifo_dev_fd = -1;
}


int fifo_dev_reopen (const char *node)
{
	int  ret;

	fifo_dev_close();

	errno = 0;
	if ( (fifo_dev_fd = open(node, O_RDWR)) < 0 )
		return fifo_dev_fd;

	if ( (ret = ioctl(fifo_dev_fd, FD_IOCG_TARGET_LIST, &fifo_dev_target_mask)) )
	{
		printf("FD_IOCG_TARGET_LIST: %d: %s", ret, strerror(errno));
		fifo_dev_close();
		return -1;
	}

	return 0;
}


const char *fifo_dev_target_desc (unsigned long mask)
{
	static char buff[64];

	snprintf(buff, sizeof(buff), "{ %s%s%s%s%s}",
		     mask & FD_TARGT_ADI1 ? "adi1 " : "",
		     mask & FD_TARGT_ADI2 ? "adi2 " : "",
		     mask & FD_TARGT_NEW  ? "new: " : "",
		     mask & FD_TARGT_DSX0 ? "dsx0 " : "",
		     mask & FD_TARGT_DSX1 ? "dsx1 " : "");

	return buff;
}


