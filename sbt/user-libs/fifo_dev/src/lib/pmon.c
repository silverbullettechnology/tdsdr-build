/** \file      src/lib/pmon.c
 *  \brief     implementation of register access to performance monitor
 *  \copyright Copyright 2014 Silver Bullet Technology
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


int fifo_pmon_read (unsigned long ofs, unsigned long *val)
{
	struct fd_pmon_regs  regs;
	int                  ret;

	regs.ofs = ofs;

	// on failure print same error as kernelspace
	if ( (ret = ioctl(fifo_dev_fd, FD_IOCG_PMON_REG, &regs)) )
		printf("FD_IOCG_PMON_REG OFS %04lx READ %08lx: %d: %s\n",
		       regs.ofs, regs.val, ret, strerror(errno));

	// on success return value if user's provided a buffer
	else if ( val )
		*val = regs.val;

	return ret;
}


int fifo_pmon_write (unsigned long ofs, unsigned long val)
{
	struct fd_pmon_regs  regs;
	int                  ret;

	regs.ofs = ofs;
	regs.val = val;

	// on failure print same error as kernelspace
	if ( (ret = ioctl(fifo_dev_fd, FD_IOCS_PMON_REG_SO, &regs)) )
		printf("FD_IOCS_PMON_REG_SO AD%c %cX OFS %04lx WRITE %08lx: %d: %s\n",
		       regs.ofs, val, ret, strerror(errno));

	return ret;
}

