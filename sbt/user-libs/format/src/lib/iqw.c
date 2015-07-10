/** \file      src/lib/iqw.c
 *  \brief     I/O formatter - IQW format
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
static size_t format_iqw_size (struct format_options *opt, FILE *fp)
{
	struct stat  sb;
	uint32_t     head;
	size_t       data;
	off_t        file;
	int          ret;

	rewind(fp);
	if ( (ret = fread(&head, 1, sizeof(head), fp)) < sizeof(head) )
	{
		LOG_DEBUG("%s: fread: %s\n", __func__, strerror(errno));
		return 0;
	}
	if ( head & 1 )
	{
		LOG_DEBUG("%s: head value %u invalid\n", __func__, head);
		errno = EINVAL;
		return 0;
	}
	
	// head counts samples
	file  = head * sizeof(float);
	file += sizeof(uint32_t);
	if ( fstat(fileno(fp), &sb) )
	{
		LOG_DEBUG("%s: stat: %s\n", __func__, strerror(errno));
		return 0;
	}
	else if ( sb.st_size != file )
	{
		LOG_DEBUG("%s: head %u -> expected %ld bytes, file is %ld\n", __func__,
		          head, file, sb.st_size);
		errno = EINVAL;
		return 0;
	}

	data  = head / 2;
	data *= opt->sample;
	LOG_DEBUG("%s: head %u -> data %zu\n", __func__, head, data);
	return data;
}


/** Read a file into a buffer according to the options given.  The buffer must be at least
 *  "size" bytes, which should preferrably have been returned from format_size() for the
 *  file and options.  Returns the number of bytes used in the buffer, or 0 on error. */
static size_t format_iqw_read_start (struct format_state *fs, FILE *fp)
{
	uint32_t  head;
	size_t    data;
	int       ret;

	rewind(fp);
	if ( (ret = fread(&head, 1, sizeof(head), fp)) < sizeof(head) )
	{
		LOG_DEBUG("%s: fread: %s\n", __func__, strerror(errno));
		return 0;
	}
	if ( head & 1 )
	{
		LOG_DEBUG("%s: head value %u invalid\n", __func__, head);
		errno = EINVAL;
		return 0;
	}

	// save length of each subsample stretch and init "left" with it
	fs->iqw.rows = (head / 2) * sizeof(float);
	fs->iqw.left = fs->iqw.rows;
	fs->iqw.clip = 0;

	data  = head / 2;
	data *= fs->opt->sample;
	LOG_DEBUG("%s: head %u -> data %zu\n", __func__, head, data);
	return data;
}


/** Read a file into a buffer according to the options given.  The buffer must be at least
 *  "size" bytes, which should preferrably have been returned from format_size() for the
 *  file and options.  Returns the number of bytes used in the buffer, or 0 on error. */
static size_t format_iqw_read_block (struct format_state *fs, void *buff, size_t size,
                                     FILE *fp)
{
	size_t    rows = size / fs->opt->sample;
	size_t    data = rows * sizeof(float);
	float    *tmp = malloc(data);
	void     *samp = buff;
	void     *load;
	size_t    want;

	float    *val1;
	long      val2;
	long      sign = 1 << (fs->opt->bits - 1);
	long      max  = sign - 1;
	long      min  = 0 - sign;
	uint16_t  val3;
	uint16_t  val4;
	uint16_t *type;

	int       ch;
	int       iq = fs->opt->flags & FO_IQ_SWAP ? 1 - fs->cur_pass : fs->cur_pass;
	int       ret;

	if ( !tmp )
	{
		LOG_DEBUG("%s: malloc(%zu) failed\n", __func__, data);
		return 0;
	}

	// reset on new pass, so we don't read the dregs of I samples as the first few Q
	if ( fs->new_pass )
		fs->iqw.left = 0;

	load = tmp;
	while ( data )
	{
		want = data;
		LOG_DEBUG("%s: read loop, want/data %zu\n", __func__, want);

		// rewind needed
		if ( !fs->iqw.left )
		{
			// all IQW files are offset by a fixed uin32_t "header"
			off_t offs = sizeof(uint32_t);

			// consecutive Q samples follow consecutive I samples:
			// H,I,I,I,I,I,Q,Q,Q,Q,Q
			// not affected by IQ swapping, this controls position within the file
			if ( fs->cur_pass )
				offs += fs->iqw.rows;

			if ( fseek(fp, offs, SEEK_SET) )
			{
				LOG_DEBUG("%s: fseek(%ld): %s\n", __func__, offs, strerror(errno));
				return 0;
			}
			fs->iqw.left = fs->iqw.rows;
			LOG_DEBUG("%s: rewound to %ld, %zu avail\n", __func__, offs, fs->iqw.left);
		}
		// partial block left, trim it
		if ( want > fs->iqw.left )
		{
			want = fs->iqw.left;
			LOG_DEBUG("%s: low left, cap want to %zu\n", __func__, fs->iqw.left);
		}

		if ( (ret = fread(load, 1, want, fp)) < 0 )
		{
			LOG_DEBUG("%s: fread: %s\n", __func__, strerror(errno));
			free(tmp);
			return 0;
		}

		LOG_DEBUG("%s: read block of %d, %zu left\n", __func__, ret, data - ret);
		load += ret;
		data -= ret;
		fs->iqw.left -= ret;
	}
	val1 = tmp;

	LOG_DEBUG("%s: read %zu rows / %zu bytes of data:\n", __func__,
	          rows, rows * sizeof(float));
	if ( format_debug )
		hexdump_buff(format_debug, tmp, rows * sizeof(float));

	while ( rows-- )
	{
		/* do float -> long -> clip -> sign -> uint16_t first, once */
		val2 = roundf(*val1 * sign);
		if ( val2 > max )
		{
			val2 = max;
			fs->iqw.clip++;
		}
		else if ( val2 < min )
		{
			val2 = min;
			fs->iqw.clip++;
		}
		val3 = (uint16_t)val2 & max;
		if ( val2 < 0 )
			val3 |= sign;

		if ( fs->opt->flags & FO_ENDIAN )
			val4 = (val3 << 8) | (val3 >> 8);
		else
			val4 = val3;

		LOG_DEBUG("%s: sample %g -> %ld -> %04x -> %04x -> ", __func__,
		          *val1, val2, val3, val4);

		type = samp;
		for ( ch = 0; (1 << ch) <= fs->opt->channels; ch++ )
			if ( (1 << ch) & fs->opt->channels )
			{
				// I/Q modified by swap flag
				LOG_DEBUG("%d.%d ", ch, iq);
				type[(ch * 2) + iq] = val4;
			}

		LOG_DEBUG("\n");
		samp += fs->opt->sample;
		val1++;
	}
	if ( format_debug )
		hexdump_buff(format_debug, buff, size);

	free(tmp);
	errno = 0;
	return samp - buff;
}


/** Read a file into a buffer according to the options given.  The buffer must be at least
 *  "size" bytes, which should preferrably have been returned from format_size() for the
 *  file and options.  Returns the number of bytes used in the buffer, or 0 on error. */
static size_t format_iqw_read_finish (struct format_state *fs, FILE *fp)
{
	LOG_DEBUG("%s: %zu subsamples clipped on read\n", __func__, fs->iqw.clip);
	return 1;
}


/** Read a file into a buffer according to the options given.  The buffer must be at least
 *  "size" bytes, which should preferrably have been returned from format_size() for the
 *  file and options.  Returns the number of bytes used in the buffer, or 0 on error. */
static size_t format_iqw_write_start (struct format_state *fs, FILE *fp)
{
	uint32_t  head = fs->data;
	int       ret;

	head /= fs->opt->sample;
	head *= 2;

	rewind(fp);
	if ( (ret = fwrite(&head, 1, sizeof(head), fp)) < sizeof(head) )
	{
		LOG_DEBUG("%s: fwrite: %s\n", __func__, strerror(errno));
		return 0;
	}

	return fs->buff;
}


/** Write a file from a buffer according to the options given.  "size" bytes will be
 *  saved from the buffer to the file, and returned on success, or 0 for error. */
static size_t format_iqw_write_block (struct format_state *fs, const void *buff,
                                      size_t size, FILE *fp)
{
	size_t          rows = size / fs->opt->sample;
	const void     *samp = buff;
	const uint16_t *type;
	uint16_t        val2;
	long            val3;
	float           val4;
	long            sign = 1 << (fs->opt->bits - 1);
	long            mask = sign - 1;
	int             ch;
	int             iq = fs->opt->flags & FO_IQ_SWAP ? 1 - fs->cur_pass : fs->cur_pass;
	int             ret;

	if ( format_debug )
		hexdump_buff(format_debug, buff, size);

	while ( rows-- )
	{
		for ( ch = 0; (1 << ch) <= fs->opt->channels; ch++ )
			if ( (1 << ch) & fs->opt->channels )
			{
				type  = samp;
				type += (ch * 2) + iq;
				LOG_DEBUG("%04x -> ", *type);

				val2 = *type;
				if ( fs->opt->flags & FO_ENDIAN )
					val2 = (val2 << 8) | (val2 >> 8);

				val3 = val2 & mask;
				LOG_DEBUG("%04x -> %04lx -> ", val2, val3);
				if ( val2 & sign )
					val3 -= sign;
				LOG_DEBUG("%ld -> ", val3);

				val4  = val3;
				val4 /= sign;
				LOG_DEBUG("%g ", val4);

				if ( (ret = fwrite(&val4, 1, sizeof(val4), fp)) < sizeof(val4) )
				{
					LOG_DEBUG("%s: fwrite: %s\n", __func__, strerror(errno));
					return 0;
				}
				LOG_DEBUG("(%d)\n", ret);
				break;
			}

		samp += fs->opt->sample;
	}

	errno = 0;
	return samp - buff;
}

struct format_class format_iqw =
{ 
	.name           = "iqw",
	.desc           = "32-bit float I/Q format",
	.size           = format_iqw_size,
	.read_start     = format_iqw_read_start,
	.read_block     = format_iqw_read_block,
	.read_finish    = format_iqw_read_finish,
	.write_start    = format_iqw_write_start,
	.write_block    = format_iqw_write_block,
	.passes         = 2,
	.read_channels  = FC_CHAN_MULTI,
	.write_channels = FC_CHAN_SINGLE,
};

