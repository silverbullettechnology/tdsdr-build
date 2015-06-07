/** \file      src/lib/access.c
 *  \brief     implementation of Access control IOCTLs
 *  \copyright Copyright 2015 Silver Bullet Technology
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
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>

#include <fd_main.h>

#include "fifo_dev.h"
#include "private.h"


int fifo_access_avail (unsigned long *bits)
{
	int ret;

	if ( (ret = ioctl(fifo_dev_fd, FD_IOCG_ACCESS_AVAIL, bits)) )
		printf("FD_IOCG_ACCESS_AVAIL: %d: %s\n", ret, strerror(errno));

	return ret;
}

int fifo_access_request (unsigned long bits)
{
	int ret;

	if ( (ret = ioctl(fifo_dev_fd, FD_IOCS_ACCESS_REQUEST, bits)) )
		printf("FD_IOCS_ACCESS_REQUEST: %d: %s\n", ret, strerror(errno));

	return ret;
}

int fifo_access_release (unsigned long bits)
{
	int ret;

	if ( (ret = ioctl(fifo_dev_fd, FD_IOCS_ACCESS_RELEASE, bits)) )
		printf("FD_IOCS_ACCESS_RELEASE: %d: %s\n", ret, strerror(errno));

	return ret;
}

