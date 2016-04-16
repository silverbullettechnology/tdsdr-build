/** \file      src/lib/nul.c
 *  \brief     I/O formatters - null discard target
 *  \copyright Copyright 2013-2015 Silver Bullet Technology
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
#include <unistd.h>
#include <stdint.h>
#include <sys/stat.h>
#include <ctype.h>
#include <math.h>
#include <errno.h>

#include "format.h"
#include "private.h"


static size_t format_nul_write (struct format_state *fs, const void *buff, size_t size,
                                FILE *fp)
{
	errno = 0;
	return size;
}

struct format_class format_nul =
{
	.name           = "nul",
	.desc           = "NULL format, discards data",
	.write_block    = format_nul_write,
	.write_channels = FC_CHAN_BUFFER,
};

