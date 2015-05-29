/** \file      dsa_format.h
 *  \brief     I/O formatters
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
#ifndef _DSA_FORMAT_H_
#define _DSA_FORMAT_H_


typedef long (* format_size_fn)   (FILE *fp, int chan);
typedef int  (* format_read_fn)   (FILE *fp, void *buff, size_t size, int chan, int lsh);
typedef int  (* format_write_fn)  (FILE *fp, void *buff, size_t size, int chan);


struct format
{
	const char      *name;
	const char      *exts;
	format_size_fn   size;
	format_read_fn   read;
	format_write_fn  write;
};


struct format *format_find (const char *name);
struct format *format_guess (const char *name);


static inline int format_size (struct format *fmt, FILE *fp, int chan)
{
	if ( !fmt || !fmt->size )
	{
		errno = ENOSYS;
		return -1;
	}

	return fmt->size(fp, chan);
}

static inline int format_read (struct format *fmt, FILE *fp, void *buff, size_t size,
                               int chan, int lsh)
{
	if ( !fmt || !fmt->read )
	{
		errno = ENOSYS;
		return -1;
	}

	return fmt->read(fp, buff, size, chan, lsh);
}

static inline int format_write (struct format *fmt, FILE *fp, void *buff, size_t size,
                                int chan)
{
	if ( !fmt || !fmt->write )
	{
		errno = ENOSYS;
		return -1;
	}

	return fmt->write(fp, buff, size, chan);
}


void hexdump_line (FILE *fp, const unsigned char *ptr, int len);
void hexdump_buff (FILE *fp, const void *buf, int len);
void smpdump (FILE *fp, void *buff, size_t size);


#endif /* _DSA_FORMAT_H_ */
