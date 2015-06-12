/** \file      src/lib/dec.c
 *  \brief     I/O formatter - dec format
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


#define  MAX_LINE  4096


/** Analyze a file (possibly reading header bytes) and calculate the size of the buffer
 *  necessary to load it according to the options given.  Returns a size on success, or 0
 *  on error */
static size_t format_dec_size (struct format_options *opt, FILE *fp)
{
	char   *line = malloc(MAX_LINE);
	size_t  data = 0;

	if ( !line )
		return 0;

	rewind(fp);
	while ( fgets(line, MAX_LINE, fp) )
		data++;

	LOG_DEBUG("%s: %zu lines read, %zu bytes per sample -> data %zu\n", __func__,
	          data, opt->sample, data * opt->sample);
	
	data *= opt->sample;

	LOG_DEBUG("data is %zu bytes\n", data);
	free(line);
	return data;
}

/** Read a file into a buffer according to the options given.  The buffer must be at least
 *  "size" bytes, which should preferrably have been returned from format_size() for the
 *  file and options.  Returns the number of bytes used in the buffer, or 0 on error. */
static size_t format_dec_read (struct format_state *fs, void *buff, size_t size, FILE *fp)
{
	char     *line = malloc(MAX_LINE);
	size_t    rows = size / fs->opt->sample;
	void     *samp = buff;
	uint16_t *type;
	char     *ptr;
	long      val;
	long      sign = 1 << (fs->opt->bits - 1);
	long      max  = sign - 1;
	long      min  = 0 - sign;
	int       empty = 0;
	int       ch, iq;


	if ( !line )
		return 0;

	while ( rows-- )
	{
		// try to get line, on EOF reopen once
		if ( !fgets(line, MAX_LINE, fp) )
		{
			if ( empty )
			{
				LOG_DEBUG("%s: rewinding an empty file is pointless\n", __func__);
				free(line);
				return 0;
			}

			LOG_DEBUG("%s: rewind file\n", __func__);
			empty = 1;
			rewind(fp);
			continue;
		}
		empty = 0;

		type = samp;

		ch = 0;
		for ( ptr = line; isspace(*ptr); ptr++ ) ;
		while ( *ptr )
		{
			while ( (1 << ch) <= fs->opt->channels && !((1 << ch) & fs->opt->channels) )
				ch++;
			if ( (1 << ch) > fs->opt->channels )
				break;

			for ( iq = 0; iq < 2; iq++ )
			{
				LOG_DEBUG("'%s' -> ", ptr);
				val = strtol(ptr, &ptr, 0);
				if ( val < min || val > max )
				{
					LOG_DEBUG("%s: val %ld out of range %ld..%ld\n", __func__,
					          val, min, max);
					return 0;
				}
				LOG_DEBUG("%ld -> ", val);

				type[(ch * 2) + iq] = (uint16_t)val & max;
				if ( val < 0 )
					type[(ch * 2) + iq] |= sign;

				LOG_DEBUG("%x\n", type[(ch * 2) + iq]);
			}

			for ( ; isspace(*ptr); ptr++ ) ;
			ch++;
		}

		samp += fs->opt->sample;
	}

	free(line);
	return samp - buff;
}

/** Write a file from a buffer according to the options given.  "size" bytes will be
 *  saved from the buffer to the file, and returned on success, or 0 for error. */
static size_t format_dec_write (struct format_state *fs, const void *buff, size_t size,
                                FILE *fp)
{
	size_t          rows = size / fs->opt->sample;
	const void     *samp = buff;
	const uint16_t *type;
	long            val;
	long            sign = 1 << (fs->opt->bits - 1);
	long            mask = sign - 1;
	int             ch, iq;
	int             eol = '\0';

	while ( rows-- )
	{
		type = samp;

		for ( ch = 0; (1 << ch) <= fs->opt->channels; ch++ )
			if ( (1 << ch) & fs->opt->channels )
				for ( iq = 0; iq < 2; iq++ )
				{
					val = type[(ch * 2) + iq] & mask;
					if ( type[(ch * 2) + iq] & sign )
						val -= sign;

					if ( eol )
						fputc(eol, fp);
					fprintf(fp, "%6ld", val);
					eol = '\t';
				}

		eol = '\n';
		samp += fs->opt->sample;
	}
	if ( eol )
		fputc(eol, fp);

	return samp - buff;
}

struct format_class format_dec =
{
	.name           = "dec",
	.desc           = "Signed decimal format",
	.size           = format_dec_size,
	.read_block     = format_dec_read,
	.write_block    = format_dec_write,
	.read_channels  = FC_CHAN_MULTI,
	.write_channels = FC_CHAN_MULTI,
};

