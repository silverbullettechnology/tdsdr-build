/** Growable buffer
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
#ifndef INCLUDE_LIB_BUFFER_H
#define INCLUDE_LIB_BUFFER_H


struct buffer
{
	size_t  used;
	size_t  size;
	char    data[];
};


/** Allocate and initialize a buffer 
 *
 *  The caller must free() the buffer when done with it
 *
 *  \param size Size of data buffer in bytes
 *
 *  \return Buffer pointer, or NULL on error
 */
struct buffer *buffer_alloc (size_t size);


/** Reset a buffer
 *
 *  \param buff Buffer to reset
 */
void buffer_reset (struct buffer *buff);


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
int buffer_fmt (struct buffer *buff, const char *fmt, ...);


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
int buffer_cpy (struct buffer *buff, const char *src, int len);


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
int buffer_set (struct buffer *buff, char val, int len);


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
int buffer_read (struct buffer *buff, int fd);


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
void buffer_discard (struct buffer *buff, int len);


/** Write bytes from a buffer to a file
 *
 *  \note Bytes written to the file are discarded from the buffer
 *
 *  \param buff  Buffer to write from
 *  \param fd    File descriptor to write to
 *
 *  \return Number of bytes written, or <0 on error
 */
int buffer_write (struct buffer *buff, int fd);


/** Search a buffer for a character
 *
 *  \param buff Buffer to search
 *  \param ch   Character to search for
 *
 *  \return pointer to the matching character in buff, or NULL if not found
 */
char *buffer_chr (struct buffer *buff, char ch);


static inline int buffer_empty (struct buffer *buff)
{
	return buff->used == 0;
}

static inline int buffer_full (struct buffer *buff)
{
	return buff->used >= buff->size;
}

static inline int buffer_free (struct buffer *buff)
{
	return buff->size - buff->used;
}

static inline int buffer_used (struct buffer *buff)
{
	return buff->used;
}


#endif /* INCLUDE_LIB_BUFFER_H */
/* Ends   */
