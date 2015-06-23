/** \file      private.h
 *  \brief     I/O formatters - private definitions
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
#ifndef _INCLUDE_FORMAT_PRIVATE_H_
#define _INCLUDE_FORMAT_PRIVATE_H_
#include <stdio.h>


struct format_options;
struct format_state
{
	/* Pointer to user-supplied or default options struct, guaranteed non-NULL */
	struct format_options *opt;

	/* Size of data in buffer and buffer overall, in bytes */
	size_t  data;
	size_t  buff;

	/* Multi-pass counter: cur_pass will be 0 on the first pass, 1 on the second, up to
	 * format_class.passes - 1.  On the first call to a _block function after the pass
	 * number changes, new_pass will be nonzero, on subsequent calls it will be zero.  */
	int  cur_pass;
	int  new_pass;

	union
	{
		struct
		{
			size_t  rows;
			size_t  left;
			size_t  clip;
		} iqw;
	};
};


/** Analyze a file (possibly reading header bytes) and calculate the size of the buffer
 *  necessary to load it according to the options given.  Returns a size on success, or 0
 *  on error */
typedef size_t (* format_size_fn) (struct format_options *opt, FILE *fp);


/** Begin a file read, possibly validating header data, and return nonzero on success. */
typedef size_t (* format_read_start_fn) (struct format_state *fs, FILE *fp);

/** Read a chunk into a buffer according to the options given.  The buffer must be at least
 *  "size" bytes, and the caller manages pointer math for packets.  File formats requiring
 *  multiple passes will be called multiple times, with the "pass" field in the options
 *  struct climbing per pass, from 0 to passes - 1.  Returns the number of bytes used in
 *  the buffer, or 0 on error. */
typedef size_t (* format_read_block_fn) (struct format_state *fs, void *buff, size_t size,
                                         FILE *fp);

/** End a file read, and return nonzero on success.  Any temporary memory allocated during
 *  the read can be freed here. */
typedef size_t (* format_read_finish_fn) (struct format_state *fs, FILE *fp);


/** Begin a file write, initializing state data as needed, and return nonzero on success.
 */
typedef size_t (* format_write_start_fn) (struct format_state *fs, FILE *fp);

/** Write a chunk from a buffer according to the options given.  "size" bytes will be
 *  saved from the buffer to the file.  File formats requiring multiple passes will be
 *  called multiple times, with the "pass" field in the options struct climbing per pass,
 *  from 0 to passes - 1.  Return value is the number of bytes written, or 0 on error. */
typedef size_t (* format_write_block_fn) (struct format_state *fs, const void *buff,
                                          size_t size, FILE *fp);

/** End a file read, and return nonzero on success.  Any header fixups needed can be done
 *  here, and any temporary memory allocated during the write freed. */
typedef size_t (* format_write_finish_fn) (struct format_state *fs, FILE *fp);


struct format_class
{
	const char             *name;
	const char             *desc;
	const char             *exts;
	format_size_fn          size;
	format_read_start_fn    read_start;
	format_read_block_fn    read_block;
	format_read_finish_fn   read_finish;
	format_write_start_fn   write_start;
	format_write_block_fn   write_block;
	format_write_finish_fn  write_finish;
	int                     passes;
	int                     read_channels;
	int                     write_channels;
};


extern FILE *format_debug;
#define LOG_DEBUG(...) \
	do{ \
		if ( format_debug ) \
			fprintf(format_debug, ##__VA_ARGS__); \
	}while(0)


extern FILE *format_error;
#define LOG_ERROR(...) \
	do{ \
		if ( !format_error ) \
			format_error = stderr; \
		if ( format_error ) \
			fprintf(format_error, ##__VA_ARGS__); \
	}while(0)


/* class registration structs */
extern struct format_class format_raw;
extern struct format_class format_bin;
extern struct format_class format_hex;
extern struct format_class format_dec;
extern struct format_class format_iqw;
extern struct format_class format_bit;
extern struct format_class format_nul;


/* Internals of some formats, exposed for debugging. */
void hexdump_line (FILE *fp, const unsigned char *ptr, int len);
void hexdump_buff (FILE *fp, const void *buf, int len);
void smpdump (FILE *fp, void *buff, size_t size);


#endif /* _INCLUDE_FORMAT_PRIVATE_H_ */
