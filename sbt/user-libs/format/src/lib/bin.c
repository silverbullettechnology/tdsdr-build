/** \file      src/lib/bin.c
 *  \brief     I/O formatter - bin format
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


/** Analyze a file (possibly reading header bytes) and calculate the size of the payload
 *  data it contains.  Returns a size on success, or 0 on error */
static size_t format_bin_size (struct format_options *opt, FILE *fp)
{
	struct stat sb;
	if ( fstat(fileno(fp), &sb) )
		return 0;

	LOG_DEBUG("%s: file is %ld bytes\n", __func__, (long)sb.st_size);
	errno = 0;
	return sb.st_size;
}


/** Read a file into a buffer according to the options given.  The buffer must be at least
 *  "size" bytes, which should preferrably have been returned from format_size() for the
 *  file and options.  Returns the number of bytes used in the buffer, or 0 on error. */
static size_t format_bin_read (struct format_state *fs, void *buff, size_t size,
                               FILE *fp)
{
	void *dst = buff;
	int   ret;

	if ( (ret = fread(dst, 1, size, fp)) < 0 )
	{
		LOG_DEBUG("%s: fread: %s\n", __func__, strerror(errno));
		return 0;
	}
	dst  += ret;
	size -= ret;

	// EOF: rewind and continue if file smaller than buff, otherwise break
	while ( size )
	{
		LOG_DEBUG("EOF, need %zu more, rewind and continue\n", size);

		rewind(fp);
		if ( (ret = fread(dst, 1, size, fp)) < 0 )
		{
			LOG_DEBUG("%s: fread: %s\n", __func__, strerror(errno));
			return 0;
		}

		dst  += ret;
		size -= ret;
	}

	errno = 0;
	return dst - buff;
}


/** Write a file from a buffer according to the options given.  "size" bytes will be
 *  saved from the buffer to the file, and returned on success, or 0 for error. */
static size_t format_bin_write (struct format_state *fs, const void *buff, size_t size,
                                FILE *fp)
{
	int   ret;

	if ( (ret = fwrite(buff, 1, size, fp)) < 0 )
	{
		LOG_DEBUG("%s: fwrite: %s\n", __func__, strerror(errno));
		return 0;
	}

	errno = 0;
	return ret;
}

struct format_class format_bin =
{
	.name           = "bin",
	.desc           = "Packed binary format",
	.size           = format_bin_size,
	.read_block     = format_bin_read,
	.write_block    = format_bin_write,
	.read_channels  = FC_CHAN_BUFFER,
	.write_channels = FC_CHAN_BUFFER,
};

