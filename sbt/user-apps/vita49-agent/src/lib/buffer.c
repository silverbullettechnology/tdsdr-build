/** Buffer datatype
 *
 *  \author    Morgan Hughes <morgan.hughes@silver-bullet-tech.com>
 *  \version   v0.0
 *  \copyright Copyright 2014,2015 Silver Bullet Technology
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
 *  vim:ts=4:noexpandtab
 */
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <stdarg.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>

#include <lib/log.h>
#include <lib/buffer.h>


LOG_MODULE_STATIC("lib_buffer", LOG_LEVEL_WARN);


/** Allocate and initialize a buffer 
 *
 *  The caller must free() the buffer when done with it
 *
 *  \param size Size of data buffer in bytes
 *
 *  \return Buffer pointer, or NULL on error
 */
struct buffer *buffer_alloc (size_t size)
{
	ENTER("size %zu", size);

	struct buffer *buff = malloc( offsetof(struct buffer, data) + size );
	if ( !buff )
		RETURN_VALUE("%p", NULL);

	memset(buff, 0, offsetof(struct buffer, data) + size);
	buff->size = size;

	RETURN_ERRNO_VALUE(0, "%p", buff);
}


/** Reset a buffer
 *
 *  \param buff Buffer to reset
 */
void buffer_reset (struct buffer *buff)
{
	ENTER("buff %p", buff);

	buff->used = 0;

	RETURN_ERRNO(0);
}


/** Use snprintf() to append a formatted string to the buffer 
 *
 *  \note This appends formatted data to the current contents; use buffer_reset() first to
 *        replace the contents with the formatted string.
 *  \note The length limit (second param to snprintf()) is used to avoid buffer overflow,
 *        and the terminating NUL counts against this limit, but the returned value and
 *        the increment to buff->used do not include the NUL.
 *
 *  \param buff  Buffer to add formatted string
 *  \param fmt   Format string for snprintf
 *  \param ...   Variable-length argument list
 *
 *  \return Number of printable bytes added to the buffer by this call, or <0 on error.
 */
int buffer_fmt (struct buffer *buff, const char *fmt, ...)
{
	ENTER("buff %p, fmt %s, ...", buff, fmt);
	char    *ins = buff->data + buff->used;
	char    *end = buff->data + buff->size;
	va_list  arg;
	int      ret;

	va_start(arg, fmt);
	ret = vsnprintf(ins, end - ins, fmt, arg);
	va_end(arg);

	if ( ret < 0 )
		RETURN_VALUE("%d", ret);

	buff->used += ret;
	RETURN_ERRNO_VALUE(0, "%d", ret);
}


/** Copy a string or block of bytes into the buffer
 *
 *  \note This appends the data to the current contents; use buffer_reset() first to
 *        replace the contents with the data.
 *
 *  \param buff  Buffer to add data
 *  \param src   Pointer to source data
 *  \param len   Length of binary data, or <0 to treat as string
 *
 *  \return Number of bytes added to buffer
 */
int buffer_cpy (struct buffer *buff, const char *src, int len)
{
	ENTER("buff %p, src %p, len %d", buff, src, len);

	if ( buff->used >= buff->size )
		RETURN_ERRNO_VALUE(ENOSPC, "%d", -1);

	if ( len < 0 )
		len = strlen(src);
	
	if ( len > (buff->size - buff->used) )
		len = buff->size - buff->used;

	memcpy(buff->data + buff->used, src, len);
	buff->used += len;
	RETURN_ERRNO_VALUE(0, "%d", len);
}


/** Set a block of bytes in the buffer to a fixed value
 *
 *  \note This appends the data to the current contents; use buffer_reset() first to
 *        replace the contents with the data.
 *
 *  \param buff  Buffer to add data
 *  \param val   Value to set
 *  \param len   Length to set
 *
 *  \return Number of bytes set in buffer
 */
int buffer_set (struct buffer *buff, char val, int len)
{
	ENTER("buff %p, val 0x%02x (%c), len %d", buff, val, isprint(val) ? val : '.', len);

	if ( len > (buff->size - buff->used) )
		len = buff->size - buff->used;

	memset(buff->data + buff->used, val, len);
	buff->used += len;
	RETURN_ERRNO_VALUE(0, "%d", len);
}


/** Read bytes from a file and append to buffer
 *
 *  \note This appends the data to the current contents; use buffer_reset() first to
 *        replace the contents with the data.
 *
 *  \param buff  Buffer to read into
 *  \param fd    File descriptor to read from
 *
 *  \return Number of bytes read, or <0 on error
 */
int buffer_read (struct buffer *buff, int fd)
{
	ENTER("buff %p, fd %d", buff, fd);

	if ( buff->used >= buff->size )
		RETURN_ERRNO_VALUE(ENOSPC, "%d", 0);
	
	int ret = read(fd, buff->data + buff->used, buff->size - buff->used);
	if ( ret < 0 )
		RETURN_VALUE("%d", ret);

	buff->used += ret;
	RETURN_ERRNO_VALUE(0, "%d", ret);
}


/** Discard bytes from the beginning of the buffer
 *
 *  \note If discarding fewer bytes than are in the buffer, the remaining bytes are copied
 *        downwards, so that data[len] becomes data[0].  If discarding buff->used or more
 *        bytes, the buffer is simply reset
 *
 *  \param buff  Buffer to discard from
 *  \param len   Number of bytes to discard
 *
 *  \return Number of bytes added to buffer
 */
void buffer_discard (struct buffer *buff, int len)
{
	ENTER("buff %p, len %d, ...", buff, len);

	if ( len >= buff->used )
	{
		buffer_reset(buff);
		RETURN_ERRNO(0);
	}

	char *end = buff->data + buff->used;
	char *src = buff->data + len;
	char *dst = buff->data;

	while ( src < end )
		*dst++ = *src++;

	buff->used = dst - buff->data;
	RETURN_ERRNO(0);
}


/** Write bytes from a buffer to a file
 *
 *  \note Bytes written to the file are discarded from the buffer
 *
 *  \param buff  Buffer to write from
 *  \param fd    File descriptor to write to
 *
 *  \return Number of bytes written, or <0 on error
 */
int buffer_write (struct buffer *buff, int fd)
{
	ENTER("buff %p, fd %d", buff, fd);

	if ( !buff->used )
		RETURN_ERRNO_VALUE(0, "%d", 0);
	
	int ret = write(fd, buff->data, buff->used);
	if ( ret < 0 )
		RETURN_VALUE("%d", ret);

	buffer_discard(buff, ret);
	RETURN_ERRNO_VALUE(0, "%d", ret);
}


/** Search a buffer for a character
 *
 *  \param buff Buffer to search
 *  \param ch   Character to search for
 *
 *  \return pointer to the matching character in buff, or NULL if not found
 */
char *buffer_chr (struct buffer *buff, char ch)
{
	ENTER("buff %p, c 0x%02x (%c)", buff, ch, isprint(ch) ? ch : '.');

	if ( !buff->used )
		RETURN_ERRNO_VALUE(0, "%p", NULL);

	char *ret = memchr(buff->data, ch, buff->used);
	RETURN_ERRNO_VALUE(0, "%p", ret);
}


#ifdef UNIT_TEST
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

void stop (const char *fmt, ...)
{
	int err = errno;

	va_list ap;
	va_start (ap, fmt);
	vfprintf (stderr, fmt, ap);
	va_end   (ap);

	fprintf (stderr, ": %s\n", strerror(err));
	exit    (1);
}

int main (int argc, char **argv)
{
	log_dupe(stdout);
	module_level = LOG_LEVEL_TRACE;

	struct buffer *buff = buffer_alloc(48);
	if ( !buff )
		stop("buffer_alloc()");

	buff->size = 32;
	LOG_HEXDUMP(buff, 64);

	int ret = buffer_set(buff, 'X', 256);
	LOG_DEBUG("buffer_set(buff, 'X', 256): %d (%s)\n", ret, strerror(errno));
	LOG_HEXDUMP(buff, 64);

	buffer_reset(buff); memset(buff->data, 0xFF, 48);
	ret = buffer_cpy(buff, "xxxxxxxxxxxxyyyy", -1);
	LOG_DEBUG("buffer_cpy(buff, \"xxxxxxxxxxxxyyyy\", -1): %d (%s)\n", ret, strerror(errno));
	ret = buffer_cpy(buff, "zzzzzzzzzzzzzzzz", -1);
	LOG_DEBUG("buffer_cpy(buff, \"zzzzzzzzzzzzzzzz\", -1): %d (%s)\n", ret, strerror(errno));
	LOG_HEXDUMP(buff, 64);

	buffer_discard(buff, 12);
	LOG_DEBUG("buffer_discard(buff, 12): (%s)\n", strerror(errno));
	LOG_HEXDUMP(buff, 64);

	buffer_reset(buff); memset(buff->data, 0xFF, 48);
	ret = buffer_fmt(buff, "foo\tbar\tbaz");
	LOG_DEBUG("buffer_fmt(buff, \"foo\\tbar\\tbaz\"): %d (%s)\n", ret, strerror(errno));
	LOG_HEXDUMP(buff, 64);

	char *ptr = buffer_chr(buff, '\t');
	LOG_DEBUG("buffer_chr(buff, '\\t': +%d (%s)\n",
	          ptr ? (int)(ptr - buff->data) : -1,
	          strerror(errno));

	int fd = open("buffer-test", O_WRONLY|O_CREAT|O_TRUNC, 0644);
	if ( fd < 0 )
		stop("open(buffer-test)");
	int len = buffer_write(buff, fd);
	LOG_DEBUG("buffer_write(buff, fd): %d (%s)\n", len, strerror(errno));
	close(fd);

	buffer_reset(buff); memset(buff->data, 0xFF, 48);
	fd = open("buffer-test", O_RDONLY);
	if ( fd < 0 )
		stop("open(buffer-test)");
	while ( (ret = buffer_read(buff, fd)) > 0 )
	{
		LOG_DEBUG("buffer_read(buff, fd): %d (%s)\n", ret, strerror(errno));
		LOG_HEXDUMP(buff, 64);
		lseek(fd, 0, SEEK_SET);
	}
	close(fd);

//	unlink("buffer-test");

	free(buff);
	return 0;
}
#endif
/* Ends    : src/lib/buffer.c */
