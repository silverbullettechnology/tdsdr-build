/** \file      dsa_ioctl_fifo_dev.c
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
#include <sys/ioctl.h>
#include <sys/types.h>
#include <errno.h>

#include <fd_main.h>

#include "dsa_main.h"

#include "log.h"
LOG_MODULE_STATIC("ioctl_fifo_dev", LOG_LEVEL_INFO);


int dsa_ioctl_target_list (unsigned long *mask)
{
	int ret;

	errno = 0;
	if ( (ret = ioctl(dsa_fifo_dev, FD_IOCG_TARGET_LIST, mask)) )
		printf("FD_IOCG_TARGET_LIST: %d: %s", ret, strerror(errno));

	return ret;
}

