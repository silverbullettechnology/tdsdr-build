/** \file      src/lib/hex.c
 *  \brief     I/O formatters - hex format
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


void hexdump_line (FILE *fp, const unsigned char *ptr, int len)
{
	int i;
	for ( i = 0; i < len; i++ )
		fprintf (fp, "%02x ", ptr[i]);

	for ( ; i < 16; i++ )
		fputs("   ", fp);

	for ( i = 0; i < len; i++ )
		fputc(isprint(ptr[i]) ? ptr[i] : '.', fp);
	fputc('\n', fp);
}
 
void hexdump_buff (FILE *fp, const void *buf, int len)
{
	const unsigned char *ptr = buf;
	while ( len >= 16 )
	{
		hexdump_line(fp, ptr, 16);
		ptr += 16;
		len -= 16;
	}
	if ( len )
		hexdump_line(fp, ptr, len);
}

static size_t format_hex_write (struct format_state *fs, const void *buff, size_t size,
                                FILE *fp)
{
	hexdump_buff(fp, buff, size);
	return size;
}

struct format_class format_hex =
{
	.name           = "hex",
	.desc           = "Canonical hexdump format",
	.write_block    = format_hex_write,
	.write_channels = FC_CHAN_BUFFER,
};

