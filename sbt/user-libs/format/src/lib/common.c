/** \file      src/lib/common.c
 *  \brief     I/O formatters
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


FILE *format_debug = NULL;
void format_debug_setup (FILE *fp)
{
	format_debug = fp;
}

FILE *format_error = NULL;
void format_error_setup (FILE *fp)
{
	format_error = fp;
}

static int bits_set (unsigned long  channels)
{
	int c;

	for ( c = 0; channels; channels >>= 1 )
		if ( channels & 1 )
			c++;

	return c;
}


/** Calculate payload data size from a given buffer size
 *
 *  This mostly calculates and subtracts the packet overhead according to the given
 *  options.
 *
 *  \param  opt   Format options pointer
 *  \param  buff  Size of buffer in bytes
 *
 *  \return  Size of the output data in the buffer, or 0 on error
 */
size_t format_size_data_from_buff (struct format_options *opt, size_t buff)
{
	if ( !opt || !opt->packet )
	{
		LOG_DEBUG("%s: no packet, data == buff == %zu\n", __func__, buff);
		return buff;
	}
	LOG_DEBUG("%s: packet %zu, head %zu, data %zu, foot %zu\n", __func__, 
	          opt->packet, opt->head, opt->data, opt->foot);

	/* number of full-size packets, based on packet size */
	size_t num  = buff / opt->packet;
	LOG_DEBUG("%s: buff %zu -> %zu full packets\n", __func__, buff, num);

	/* bytes of data in full-size packets, but subtract number of full packets */
	size_t data = num * opt->data;
	buff -= num * opt->packet;
	LOG_DEBUG("%s: data %zu from %zu bytes per %zu full packets, remainder %zu\n",
	          __func__, data, opt->data, num, buff);

	/* partial packet if there's enough for data */
	if ( buff && buff > (opt->head + opt->foot) )
	{
		LOG_DEBUG("%s: remainder %zu has %zu bytes data\n", __func__,
		          buff, buff - opt->head + opt->foot);
		buff -= opt->head + opt->foot;
		data += buff;
	}

	LOG_DEBUG("%s: final data %zu\n", __func__, data);
	return data;
}

/** Calculate buffer size from a given payload data size
 *
 *  This mostly calculates and adds the packet overhead according to the given options. 
 *
 *  \param  opt   Format options pointer
 *  \param  data  Size of payload data in bytes
 *
 *  \return  Size of the buffer, or 0 on error
 */
size_t format_size_buff_from_data (struct format_options *opt, size_t data)
{
	if ( !opt || !opt->packet )
	{
		LOG_DEBUG("%s: no packet, data == buff == %zu\n", __func__, data);
		return data;
	}
	LOG_DEBUG("%s: packet %zu, head %zu, data %zu, foot %zu\n", __func__, 
	          opt->packet, opt->head, opt->data, opt->foot);

	/* number of full-size packets, based on payload size */
	size_t num = data / opt->data;
	LOG_DEBUG("%s: data %zu -> %zu full packets\n", __func__, data, num);

	/* bytes of buffer for full-size packets */
	size_t buff = num * opt->packet;
	data -= (num * opt->data);
	LOG_DEBUG("%s: buff %zu from %zu bytes per %zu full packets, remainder %zu\n",
	          __func__, buff, opt->packet, num, data);

	/* partial packet */
	if ( data )
	{
		LOG_DEBUG("%s: remainder %zu bytes data\n", __func__, data);
		buff += opt->head + data + opt->foot;
	}

	LOG_DEBUG("%s: final buff %zu\n", __func__, buff);
	return buff;
}


/* Class registration struct */
static struct format_class *format_list[] =
{
	&format_bin,
	&format_bit,
	&format_dec,
	&format_hex,
	&format_iqw,
	&format_nul,
	&format_raw,
	NULL
};


/** Find a class by name
 *
 *  \param  name  Class name specified by user
 *
 *  \return  Class struct pointer, or NULL on error
 */
struct format_class *format_class_find (const char *name)
{
	struct format_class **fmt;

	for ( fmt = format_list; *fmt; fmt++ )
		if ( !strcasecmp((*fmt)->name, name) )
			return *fmt;

	LOG_DEBUG("%s: class '%s' not found by name\n", __func__, name);
	errno = ENOENT;
	return NULL;
}

/** Guess a class by filename
 *
 *  Currently uses the file extension (after the last dot), may in future also search a
 *  list of possible extensions.
 *
 *  \param  name  Filename specified by user
 *
 *  \return  Class struct pointer, or NULL on error
 */
struct format_class *format_class_guess (const char *name)
{
	struct format_class **fmt;
	char                 *ptr;

	if ( !(ptr = strrchr(name, '.')) )
	{
		LOG_DEBUG("%s: class '%s' not guessable\n", __func__, name);
		errno = ENOENT;
		return NULL;
	}

	ptr++;
	for ( fmt = format_list; *fmt; fmt++ )
		if ( !strcasecmp((*fmt)->name, ptr) )
			return *fmt;
		//TODO: search the exts[] array

	LOG_DEBUG("%s: class '%s' not guessed by name\n", __func__, name);
	errno = ENOENT;
	return NULL;
}

/** Print a list of compiled-in classes to the given file handle
 *
 *  The list will contain at minimum each class name and in/out/both support, and a
 *  description for classes which implement it.
 *
 *  \param  fp  Output FILE pointer
 */
void format_class_list (FILE *fp)
{
	static struct format_class **fmt;
	const char                  *desc;
	char                        *dirs;

	for ( fmt = format_list; *fmt; fmt++ )
	{
		if ( (*fmt)->read_block && (*fmt)->write_block )
			dirs = "in/out";
		else if ( (*fmt)->read_block )
			dirs = "in-only";
		else
			dirs = "out-only";

		desc = (*fmt)->desc ? (*fmt)->desc : "";

		fprintf(fp, "  %-7s  %-8s  %s\n", (*fmt)->name, dirs, desc);
	}
}

/** Return a class's printable name
 *
 *  \param  fmt  Format class pointer
 *
 *  \return  Printable string, which may be "(null)" if the class pointer is NULL
 */
const char *format_class_name (struct format_class *fmt)
{
	return fmt ? fmt->name : "(null)";
}

/** Returns a hint as to how many channels a single call of _read or _write can handle,
 *  for a given class
 *
 *  - FC_CHAN_BUFFER - operates on the entire buffer and all channels in it (ie, .bin)
 *  - FC_CHAN_SINGLE - operates on a single channel (not currently used)
 *  - FC_CHAN_MULTI  - operates on multiple channels, specified by the "channels" bitmap
 *                     in the options struct.
 *
 *  Note that some FC_CHAN_MULTI formats (ie IQW) handle inherently single-channel data
 *  files but can be loaded into multiple channels in a single pass for efficiency.  These
 *  formats may return different values for calls to format_class_read_channels() versus
 *  format_class_write_channels().
 *
 *  Note also that FC_CHAN_BUFFER formats generally work on the packed binary format in
 *  the buffer, and calling one with a channels mask which isn't the full width of the
 *  buffer will trigger a warning if debug is enabled.
 *
 *  \param  fmt  Format class pointer
 *
 *  \return  One of the FC_CHAN_* constants described above, or <0 if the class pointer is
 *           NULL (errno will be EFAULT) or the class doesn't support the relevant
 *           operation (ie _read on an output-only format, errno will be ENOSYS)
 */
int format_class_read_channels (struct format_class *fmt)
{
	if ( !fmt )
	{
		errno = EFAULT;
		return -1;
	}
	if ( !fmt->read_block )
	{
		errno = ENOSYS;
		return -1;
	}

	errno = 0;
	return fmt->read_channels;
}

/* See above */
int format_class_write_channels (struct format_class *fmt)
{
	if ( !fmt )
	{
		errno = EFAULT;
		return -1;
	}
	if ( !fmt->write_block )
	{
		errno = ENOSYS;
		return -1;
	}

	errno = 0;
	return fmt->write_channels;
}


/* Default options match the SDRDC legacy: two channels of 12-bit I/Q with 16-bit
 * alignment, packed with no gaps for packet headers */
static struct format_options  format_options =
{
	.channels = 3,
	.single   = sizeof(uint16_t) * 2,
	.sample   = sizeof(uint16_t) * 4,
	.bits     = 12,
	.packet   = 0,
	.head     = 0,
	.data     = 0,
	.foot     = 0,
};


/** Analyze an existing file and calculate buffer size
 *
 *  Reads the file as needed and calculates the number of data samples contained, and the
 *  number of buffer bytes necessary to load it according to the options given.  Note this
 *  may be larger than the file-size: if options specifies a 2-channel buffer, sizing a
 *  single-channel format will return the size of a 2-channel buffer size.
 *
 *  If opt is NULL, an internal defaults struct is used instead, which matches the SDRDC
 *  legacy: two channels of 12-bit I/Q with 16-bit alignment, packed with no gaps for
 *  packet headers.
 *
 *  \param  fmt  Format class pointer
 *  \param  opt  Format options pointer
 *  \param  fp   FILE pointer
 *
 *  \return  Buffer size in bytes, or 0 on error
 */
size_t format_size (struct format_class *fmt, struct format_options *opt, FILE *fp)
{
	if ( !fmt || !fmt->size )
	{
		errno = ENOSYS;
		return 0;
	}
	if ( !opt )
		opt = &format_options;

	size_t data = fmt->size(opt, fp);
	if ( !data )
		return 0;

	return format_size_buff_from_data(opt, data);
}

/** Read a file into a buffer
 *
 *  If the buffer is smaller than the file the data will be truncated to fit.  If the
 *  buffer is larger then the fill will be rewound and looped to fit.  Currently this
 *  means fp cannot be a stream, this restriction may be lifted in future.
 *
 *  The "packet" fields in the options if present will cause the operation to skip bytes
 *  in the buffer, so the user can pre-fill the packet headers/footers if desired. 
 *
 *  Multi-channel details, based on the return from format_class_read_channels():
 *  - FC_CHAN_BUFFER: all channels in the buffer will be loaded regardless of the
 *                    "channels" field in the options.   
 *  - FC_CHAN_MULTI:  the channel with the lowest-numbered bit set in "channels" will be
 *                    loaded with the first channel in each read sample, the next-lowest
 *                    the second channel, etc.
 *  - FC_CHAN_SINGLE: each channel with a bit set in "channels" will be loaded with the
 *                    read data. 
 *
 *  \param  fmt   Format class pointer
 *  \param  opt   Format options pointer
 *  \param  buff  Buffer to read into
 *  \param  size  Buffer size
 *  \param  fp    FILE pointer
 *
 *  \return  number of bytes used in the buffer, which should be size, or 0 on error.
 */
size_t format_read (struct format_class *fmt, struct format_options *opt,
                    void *buff, size_t size, FILE *fp)
{
	struct format_state  state;
	unsigned long        channels;
	long                 page = sysconf(_SC_PAGESIZE);
	size_t               step = 0;
	size_t               data;
	size_t               want;
	size_t               skip = 0;
	void                *dst;
	size_t               ret;
	int                  cycles = 5;

	if ( !fmt || !fmt->read_block )
	{
		LOG_ERROR("%s: No fmt or read_block\n", __func__);
		errno = ENOSYS;
		return 0;
	}

	if ( !opt )
		opt = &format_options;

	// restrict requested channels to actual ones; pointer math in some _block code needs
	// this for safety / accuracy
	channels = (1 << (opt->sample / opt->single)) - 1;
	if ( opt->channels & ~channels )
	{
		LOG_DEBUG("%s: channel mask 0x%lx exceeds actual 0x%lx, masked to 0x%lx\n",
		          __func__, opt->channels, channels, opt->channels & channels);
		opt->channels &= channels;
	}

	// warning on requesting a partial read using a full-buffer mode
	if ( fmt->read_channels == FC_CHAN_BUFFER && opt->channels != channels )
		LOG_DEBUG("%s: partial channel read requested with FC_CHAN_BUFFER method, which"
		          " operates on all channels implicitly; the results will probably not"
		          " be what you want.\n", __func__);

	// only 16-bit samples supported currently
	if ( opt->single != sizeof(uint16_t) * 2 )
	{
		LOG_ERROR("%s: sample size %zu not supported yet.\n", __func__, opt->single);
		errno = EINVAL;
		return 0;
	}

	// warning only, weird-size packet headers may trigger this 
	if ( (size % opt->sample) )
		LOG_DEBUG("%s: requested block %zu not a multiple of sample %zu\n", __func__,
		          size, opt->sample);

	memset(&state, 0, sizeof(state));
	state.opt  = opt;
	state.buff = size;
	state.data = format_size_data_from_buff(opt, size);

	LOG_DEBUG("%s: starting out, page %ld, size %zu\n", __func__, page, size);
	if ( page < 1024 )
	{
		page = 1024;
		LOG_DEBUG("page adjusted to %ld\n", page);
	}

	if ( opt->packet )
	{
		page = opt->data;
		if ( opt->packet > (opt->head + opt->data + opt->foot) )
			skip = opt->packet - (opt->head + opt->data + opt->foot);
		LOG_DEBUG("%s: packet %zu, head %zu, data %zu, foot %zu\n", __func__,
		          opt->packet, opt->head, opt->data, opt->foot);
	}

	rewind(fp);
	if ( fmt->read_start && !fmt->read_start(&state, fp) )
	{
		LOG_ERROR("%s: format_%s_read_start: %s\n", __func__,
		          fmt->name, strerror(errno));
		return 0;
	}

	if ( state.opt->prog_func )
	{
		step = state.opt->prog_step;
		LOG_DEBUG("initial prog_step %zu\n", step);
		LOG_DEBUG("call prog_func(%u, %zu);\n", 0, size);
		state.opt->prog_func(0, size);
	}

	// Multi-pass: treat passes 0 as 1 so single-pass classes can skip it.
	// classes which need multi-pass must set it to 2 or more
	state.cur_pass = 0;
	do
	{
		state.new_pass = 1;

		data = format_size_data_from_buff(opt, size);
		dst  = buff;

		while ( data && cycles-- )
		{
			// constrain want to size of data left
			if ( (want = page) > data )
				want = data;

			if ( opt->head )
				dst  += opt->head;

			if ( !(ret = fmt->read_block(&state, dst, want, fp)) )
			{
				LOG_ERROR("%s: format_%s_read_block: %s\n", __func__,
				          fmt->name, strerror(errno));
				if ( fmt->read_finish )
					fmt->read_finish(&state, fp);
				if ( state.opt->prog_func )
				{
					LOG_DEBUG("call prog_func(%u, %u);\n", 0, 0);
					state.opt->prog_func(0, 0);
				}
				return 0;
			}
			state.new_pass = 0;

			data -= ret;
			dst  += ret;

			if ( opt->foot )
				dst  += opt->foot;

			if ( skip )
				dst  += skip;

			if ( state.opt->prog_func )
			{
				if ( !step )
				{
					LOG_DEBUG("call prog_func(%zu, %zu);\n", dst - buff, size);
					state.opt->prog_func(dst - buff - 1, size);
				}
				else if ( (dst - buff) >= step )
				{
					LOG_DEBUG("call prog_func(%zu, %zu);\n", dst - buff, size);
					state.opt->prog_func(dst - buff - 1, size);
					step += state.opt->prog_step;
				}
			}
		}

		state.cur_pass++;
	}
	while ( state.cur_pass < fmt->passes );

	if ( fmt->read_finish && !fmt->read_finish(&state, fp) )
	{
		LOG_ERROR("%s: format_%s_read_finish: %s\n", __func__,
		          fmt->name, strerror(errno));
		if ( state.opt->prog_func )
		{
			LOG_DEBUG("call prog_func(%u, %u);\n", 0, 0);
			state.opt->prog_func(0, 0);
		}
		return 0;
	}

	LOG_DEBUG("%s: Success\n", __func__);

	if ( state.opt->prog_func )
	{
		LOG_DEBUG("call prog_func(%zu, %zu);\n", size, size);
		state.opt->prog_func(size, size);
	}

	errno = 0;
	return dst - buff;
}

/** Write a file from a buffer
 *
 *  The "packet" fields in the options if present will cause the operation to skip bytes
 *  in the buffer, so the user can pre-fill the packet headers/footers if desired. 
 *
 *  Multi-channel details, based on the return from format_class_write_channels():
 *  - FC_CHAN_BUFFER: all channels in the buffer will be written regardless of the
 *                    "channels" field in the options.   
 *  - FC_CHAN_MULTI:  the channel with the lowest-numbered bit set in "channels" will be
 *                    the first channel written in each output sample, the next-lowest the
 *                    second channel, etc.
 *  - FC_CHAN_SINGLE: the channel with the only bit set in "channels" will be written; a
 *                    single bit must be set before writing - otherwise is an error.
 *
 *  \param  fmt   Format class pointer
 *  \param  opt   Format options pointer
 *  \param  buff  Buffer to write from
 *  \param  size  Buffer size
 *  \param  fp    FILE pointer
 *
 *  \return  number of bytes used in the buffer, which should be size, or 0 on error.
 */
size_t format_write (struct format_class *fmt, struct format_options *opt,
                     const void *buff, size_t size, FILE *fp)
{
	struct format_state  state;
	unsigned long        channels;
	const void          *src = buff;
	long                 page = sysconf(_SC_PAGESIZE);
	size_t               step = 0;
	size_t               data;
	size_t               want;
	size_t               skip = 0;
	size_t               ret;

	if ( !fmt || !fmt->write_block )
	{
		LOG_ERROR("%s: No fmt or write_block\n", __func__);
		errno = ENOSYS;
		return 0;
	}

	if ( !opt )
		opt = &format_options;

	// restrict requested channels to actual ones; pointer math in some _block code needs
	// this for safety / accuracy
	channels = (1 << (opt->sample / opt->single)) - 1;
	if ( opt->channels & ~channels )
	{
		LOG_DEBUG("%s: channel mask 0x%lx exceeds actual 0x%lx, masked to 0x%lx\n",
		          __func__, opt->channels, channels, opt->channels & channels);
		opt->channels &= channels;
	}

	switch ( fmt->write_channels )
	{
		// warning on requesting a partial write using a full-buffer mode
		case FC_CHAN_BUFFER:
			if ( opt->channels != channels )
				LOG_DEBUG("%s: partial channel write requested with FC_CHAN_BUFFER method"
				          ", which operates on all channels implicitly; the results will"
				          " probably not be what you want.\n", __func__);
			break;

		// fail on requesting an impossible operation
		case FC_CHAN_SINGLE:
			if ( bits_set(opt->channels) != 1 )
			{
				LOG_ERROR("%s: FC_CHAN_SINGLE must have a single channel bit set\n",
				          __func__);
				errno = EINVAL;
				return 0;
			}
			break;
	}

	// only 16-bit samples supported currently
	if ( opt->single != sizeof(uint16_t) * 2 )
	{
		LOG_ERROR("%s: sample size %zu not supported yet.\n", __func__, opt->single);
		errno = EINVAL;
		return 0;
	}

	// warning only, weird-size packet headers may trigger this 
	if ( (size % opt->sample) )
		LOG_DEBUG("%s: requested block %zu not a multiple of sample %zu\n", __func__,
		          size, opt->sample);

	memset(&state, 0, sizeof(state));
	state.opt  = opt;
	state.buff = size;
	state.data = format_size_data_from_buff(opt, size);

	if ( opt->packet )
	{
		page = opt->data;
		if ( opt->packet > (opt->head + opt->data + opt->foot) )
			skip = opt->packet - (opt->head + opt->data + opt->foot);
		LOG_DEBUG("%s: packet %zu, head %zu, data %zu, foot %zu\n", __func__,
		          opt->packet, opt->head, opt->data, opt->foot);
	}

	if ( state.opt->prog_func )
	{
		step = state.opt->prog_step;
		LOG_DEBUG("initial prog_step %zu\n", step);
		LOG_DEBUG("call prog_func(%u, %zu);\n", 0, size);
		state.opt->prog_func(0, size);
	}

	if ( fmt->write_start && !fmt->write_start(&state, fp) )
	{
		LOG_ERROR("%s: format_%s_write_start: %s\n", __func__,
		          fmt->name, strerror(errno));
		if ( state.opt->prog_func )
		{
			LOG_DEBUG("call prog_func(%u, %u);\n", 0, 0);
			state.opt->prog_func(0, 0);
		}
		return 0;
	}

	// Multi-pass: treat passes 0 as 1 so single-pass classes can skip it.
	// classes which need multi-pass must set it to 2 or more
	state.cur_pass = 0;
	do
	{
		state.new_pass = 1;

		data = format_size_data_from_buff(opt, size);
		src = buff;

		while ( data )
		{
			// constrain want to size of data left
			if ( (want = page) > data )
				want = data;

			if ( opt->head )
				src  += opt->head;

			if ( !(ret = fmt->write_block(&state, src, want, fp)) )
			{
				LOG_ERROR("%s: format_%s_write_block: %s\n", __func__,
				          fmt->name, strerror(errno));
				if ( fmt->write_finish )
					fmt->write_finish(&state, fp);
				if ( state.opt->prog_func )
				{
					LOG_DEBUG("call prog_func(%u, %u);\n", 0, 0);
					state.opt->prog_func(0, 0);
				}
				return 0;
			}
			state.new_pass = 0;

			data -= ret;
			src  += ret;

			if ( opt->foot )
				src += opt->foot;

			if ( skip )
				src += skip;

			if ( state.opt->prog_func )
			{
				if ( !step )
				{
					LOG_DEBUG("call prog_func(%zu, %zu);\n", src - buff, size);
					state.opt->prog_func(src - buff - 1, size);
				}
				else if ( (src - buff) >= step )
				{
					LOG_DEBUG("call prog_func(%zu, %zu);\n", src - buff, size);
					state.opt->prog_func(src - buff - 1, size);
					step += state.opt->prog_step;
				}
			}
		}

		state.cur_pass++;
	}
	while ( state.cur_pass < fmt->passes );

	if ( fmt->write_finish && !fmt->write_finish(&state, fp) )
	{
		LOG_ERROR("%s: format_%s_write_finish: %s\n", __func__,
		          fmt->name, strerror(errno));
		if ( state.opt->prog_func )
		{
			LOG_DEBUG("call prog_func(%u, %u);\n", 0, 0);
			state.opt->prog_func(0, 0);
		}
		return 0;
	}

	LOG_DEBUG("%s: Success\n", __func__);

	if ( state.opt->prog_func )
	{
		LOG_DEBUG("call prog_func(%zu, %zu);\n", size, size);
		state.opt->prog_func(size, size);
	}

	errno = 0;
	return src - buff;
}

