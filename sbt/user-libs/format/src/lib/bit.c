/** \file      src/lib/bit.c
 *  \brief     I/O formatters - bit format
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


static size_t format_bit_write (struct format_state *fs, const void *buff, size_t size,
                                FILE *fp)
{
	size_t          rows = size / fs->opt->sample;
	const void     *samp = buff;
	const uint16_t *type;
	long            val;
	long            mask = (1 << fs->opt->bits) - 1;
	long            bit;
	int             ch, iq;
	int             digs = (fs->opt->bits + 3) / 4;
	int             eol = '\0';

	while ( rows-- )
	{
		type = samp;

		for ( ch = 0; (1 << ch) <= fs->opt->channels; ch++ )
			if ( (1 << ch) & fs->opt->channels )
				for ( iq = 0; iq < 2; iq++ )
				{
					val = type[(ch * 2) + iq] & mask;

					if ( eol )
						fputc(eol, fp);

					fprintf(fp, "%0*lx:", digs, val);
					for ( bit = 1 << (fs->opt->bits - 1); bit; bit >>= 1 )
						fputc(val & bit ? '1' : '0', fp);

					eol = '\t';
				}

		eol = '\n';
		samp += fs->opt->sample;
	}

	return samp - buff;
}

struct format_class format_bit =
{
	.name           = "bit",
	.desc           = "ASCII bit format",
	.write_block    = format_bit_write,
	.write_channels = FC_CHAN_MULTI,
};
