/** \file      src/lib/raw.c
 *  \brief     I/O formatter - raw format
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


/** Analyze a file (possibly reading header bytes) and calculate the size of the buffer
 *  necessary to load it according to the options given.  Returns a size on success, or 0
 *  on error */
static size_t format_raw_size (struct format_options *opt, FILE *fp)
{
	struct stat sb;
	if ( fstat(fileno(fp), &sb) )
		return 0;

	LOG_DEBUG("fp is %ld bytes\n", (long)sb.st_size);
	return (size_t)sb.st_size;
}


/** Read a file into a buffer according to the options given.  The buffer must be at least
 *  "size" bytes, which should preferrably have been returned from format_size() for the
 *  file and options.  Returns the number of bytes used in the buffer, or 0 on error. */
static size_t format_raw_read (struct format_state *fs, void *buff, size_t size, FILE *fp)
{
	long    page = sysconf(_SC_PAGESIZE);
	int     file = 0;
	int     offs = 0;
	size_t  want;
	size_t  left = size;
	void   *dst = buff;
	int     ret;

	if ( page < 1024 )
	{
		page = 1024;
		LOG_DEBUG("page adjusted to %ld\n", page);
	}

	if ( (file = fseek(fp, 0, SEEK_END)) < 0 )
	{
		LOG_DEBUG("%s: fseek: %s\n", __func__, strerror(errno));
		return 0;
	}

	LOG_DEBUG("file is %d bytes\n", file);
	rewind(fp);

	while ( left )
	{
		if ( (want = page) > left )
			want = left;
		if ( file && want > (file - offs) )
			want = file - offs;
		LOG_DEBUG("page %ld, file %d, offs %d -> want %zu\n", page, file, offs, want);

		if ( (ret = fread(dst, 1, want, fp)) < 0 )
		{
			LOG_DEBUG("%s: fread: %s\n", __func__, strerror(errno));
			return 0;
		}

		// EOF handling
		if ( ret == 0 )
		{
			// more buffer to fill from file: rewind to 0 and repeat
			if ( file && left > 0 )
			{
				LOG_DEBUG("file > left, rewind and reload\n");
				fseek(fp, 0, SEEK_SET);
				offs = 0;
				continue;
			}
			// stdin or when done: return success
			else
				return 0;
		}

		LOG_DEBUG("read block of %d at %d, %zu left\n", ret, offs, left - ret);
		dst  += ret;
		left -= ret;
		offs += ret;
	}

	LOG_DEBUG("success.\n");
	return dst - buff;
}


/** Write a file from a buffer according to the options given.  "size" bytes will be
 *  saved from the buffer to the file, and returned on success, or 0 for error. */
static size_t format_raw_write (struct format_state *fs, const void *buff, size_t size,
                                FILE *fp)
{
	const void *src  = buff;
	long        page = sysconf(_SC_PAGESIZE);
	int         ret;

	while ( size )
	{
		if ( page > size )
			page = size;

		if ( (ret = fwrite(src, 1, page, fp)) < 0 )
		{
			LOG_DEBUG("%s: fwrite: %s\n", __func__, strerror(errno));
			return 0;
		}

		LOG_DEBUG("wrote block of %d, %zu left\n", ret, size - ret);
		src  += ret;
		size -= ret;
	}

	LOG_DEBUG("success.\n");
	return src - buff;
}

struct format_class format_raw =
{
	.name           = "raw",
	.desc           = "Packed binary format with no packetization",
	.size           = format_raw_size,
	.read_block     = format_raw_read,
	.write_block    = format_raw_write,
	.read_channels  = FC_CHAN_BUFFER,
	.write_channels = FC_CHAN_BUFFER,
};

