/** Generic reference-counted message buffer type, built on genlist nodes
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
#include <stddef.h>
#include <stdarg.h>
#include <unistd.h>
#include <limits.h>
#include <ctype.h>
#include <errno.h>

#include <lib/log.h>
#include <lib/mbuf.h>
#include <lib/genlist.h>


LOG_MODULE_STATIC("lib_mbuf", LOG_LEVEL_WARN);



/******** Alloc / reference count / free / clone ********/


/** Allocate a mbuf and set its reference count to 1
 *
 *  Caller must free the mbuf with mbuf_free() 
 *
 *  \param data Maximum payload size in bytes 
 *  \param user Size of user structure in bytes
 *
 *  \return pointer to mbuf structure
 */
struct mbuf *_mbuf_alloc (size_t data, size_t user, const char *file, int line)
{
	ENTER("data %zu, user %zu, file %s, line %d", data, user, file, line);

	/* Total size: fixed fields, user structure, before paint, data, and after paint */
	size_t  size = offsetof(struct mbuf, usr)
	             + user
	             + sizeof(unsigned)
	             + data
	             + sizeof(unsigned);

	struct mbuf *mbuf = genlist_alloc(size);
	if ( !mbuf )
		RETURN_VALUE("%p", NULL);

	memset(mbuf, 0, size);
	mbuf->ref = 1;
	mbuf->max = data;
	mbuf->buf = &mbuf->usr[0] + user + sizeof(unsigned);

	unsigned pad = ~0;
	memcpy(mbuf->buf - sizeof(unsigned), &pad, sizeof(unsigned));
	memcpy(mbuf->buf + data,             &pad, sizeof(unsigned));

#ifdef MBUF_REF_DEBUG
	mbuf->file = file;
	mbuf->line = line;
#endif

	LOG_DEBUG("mbuf_alloc(data %zu, user %zu): return %p to %s[%d]\n",
	          data, user, mbuf, file, line);

	RETURN_ERRNO_VALUE(0, "%p", mbuf);
}


/** Increment a mbuf's reference count
 *
 *  \param mbuf mbuf to reference
 */
void _mbuf_ref (struct mbuf *mbuf, const char *file, int line)
{
	ENTER("mbuf %p, file %s, line %d", mbuf, file, line);
	if ( !mbuf )
		RETURN_ERRNO(EFAULT);

	if ( mbuf->ref < 1 )
	{
		LOG_WARN("mbuf_ref(%p): ref %d->%d, which is probably wrong\n", mbuf,
		         mbuf->ref, mbuf->ref + 1);
		LOG_WARN("  requested from %s[%d]\n", file, line);
#ifdef MBUF_REF_DEBUG
		LOG_WARN("  previous refer %s[%d]\n", mbuf->file, mbuf->line);
#endif
	}

	mbuf->ref++;

#ifdef MBUF_REF_DEBUG
	mbuf->file = file;
	mbuf->line = line;
#endif

	RETURN_ERRNO(0);
}


/** Dereference a mbuf and free if reference count is 0
 *
 *  \param mbuf mbuf to deref
 */
void _mbuf_deref (struct mbuf *mbuf, const char *file, int line)
{
	ENTER("mbuf %p, file %s, line %d", mbuf, file, line);
	if ( !mbuf )
		RETURN_ERRNO(EFAULT);

	mbuf->ref--;
	if ( mbuf->ref == 0 )
	{
		LOG_DEBUG("mbuf_deref(%p): ref 0, free\n", mbuf);
		mbuf_free(mbuf);
	}
	else if ( mbuf->ref < 0 )
	{
		LOG_WARN("mbuf_deref(%p): ref %d, not double-freeing\n", mbuf, mbuf->ref);
		LOG_WARN("  requested from %s[%d]\n", file, line);
#ifdef MBUF_REF_DEBUG
		LOG_WARN("  previous refer %s[%d]\n", mbuf->file, mbuf->line);
#endif
	}

#ifdef MBUF_REF_DEBUG
	mbuf->file = file;
	mbuf->line = line;
#endif

	RETURN_ERRNO(0);
}


/** Free a mbuf allocated with mbuf_alloc_and_ref
 *
 *  \param mbuf mbuf to be freed
 */
void _mbuf_free (struct mbuf *mbuf, const char *file, int line)
{
	ENTER("mbuf %p, file %s, line %d", mbuf, file, line);
	if ( !mbuf )
		RETURN_ERRNO(EFAULT);

	unsigned pad;
	memcpy(&pad, mbuf->buf - sizeof(unsigned), sizeof(unsigned));
	if ( pad != ~0 )
		LOG_WARN("mbuf_free(%p): lower pad corruption, want %x, have %x\n", 
		         mbuf, UINT_MAX, pad);

	memcpy(&pad, mbuf->buf + mbuf->max, sizeof(unsigned));
	if ( pad != ~0 )
		LOG_WARN("mbuf_free(%p): upper pad corruption, want %x, have %x\n", 
		         mbuf, UINT_MAX, pad);

	if ( mbuf->ext )
		LOG_WARN("mbuf_free(%p): ext pointer %p\n", mbuf, mbuf->ext);

	genlist_free(mbuf);

	RETURN_ERRNO(0);
}


static size_t mbuf_size (const struct mbuf *mbuf)
{
	/* Total size: fixed fields, user structure and before paint, data, and after paint */
	return offsetof(struct mbuf, usr)
	     + (mbuf->buf - &mbuf->usr[0])
	     + mbuf->max
	     + sizeof(unsigned);
}


/** Allocate a new mbuf with reference count 1 and clone from another mbuf
 *
 *  Caller must free the mbuf with mbuf_free() 
 *
 *  \param mbuf mbuf to clone
 *
 *  \return pointer to mbuf structure
 */
struct mbuf *_mbuf_clone (struct mbuf *mbuf, const char *file, int line)
{
	ENTER("mbuf %p, file %s, line %d", mbuf, file, line);
	if ( !mbuf )
		RETURN_ERRNO_VALUE(EFAULT, "%p", NULL);

	size_t       size  = mbuf_size(mbuf);
	struct mbuf *clone = genlist_alloc(size);
	if ( !mbuf )
		RETURN_VALUE("%p", NULL);

	memcpy(clone, mbuf, size);
	clone->ref = 1;
	clone->ext = NULL;
	clone->buf = &clone->usr[0] + (mbuf->buf - &mbuf->usr[0]);

#ifdef MBUF_REF_DEBUG
	mbuf->file = file;
	mbuf->line = line;
#endif

	LOG_DEBUG("mbuf_clone(mbuf %p): return %p to %s[%d]\n", mbuf, clone, file, line);

	RETURN_ERRNO_VALUE(0, "%p", clone);
}


/******** Cursor manipulation ********/


/* Manipulating the "begin" cursor */
int mbuf_beg_get (const struct mbuf *mbuf)
{
	ENTER("mbuf %p", mbuf);
	if ( !mbuf )
		RETURN_ERRNO_VALUE(EFAULT, "%d", -1);

	RETURN_ERRNO_VALUE(0, "%d", mbuf->beg);
}

int mbuf_beg_set (struct mbuf *mbuf, int beg)
{
	ENTER("mbuf %p, beg %d", mbuf, beg);
	if ( !mbuf )
		RETURN_ERRNO_VALUE(EFAULT, "%d", -1);

	if ( beg < 0 || beg >= mbuf->max )
		RETURN_ERRNO_VALUE(EINVAL, "%d", -1);

	if ( mbuf->end < beg ) mbuf->end = beg;
	if ( mbuf->cur < beg ) mbuf->cur = beg;

	mbuf->beg = beg;
	RETURN_ERRNO_VALUE(0, "%d", beg);
}


/* Manipulating the "end" cursor (see mbuf_beg_*() above) */
int mbuf_end_get (const struct mbuf *mbuf)
{
	ENTER("mbuf %p", mbuf);
	if ( !mbuf )
		RETURN_ERRNO_VALUE(EFAULT, "%d", -1);

	RETURN_ERRNO_VALUE(0, "%d", mbuf->end);
}

int mbuf_end_set (struct mbuf *mbuf, int end)
{
	ENTER("mbuf %p, end %d", mbuf, end);
	if ( !mbuf )
		RETURN_ERRNO_VALUE(EFAULT, "%d", -1);

	if ( end < 0 || end > mbuf->max )
		RETURN_ERRNO_VALUE(EINVAL, "%d", -1);

	if ( mbuf->beg > end ) mbuf->beg = end;
	if ( mbuf->cur > end ) mbuf->cur = end;

	mbuf->end = end;
	RETURN_ERRNO_VALUE(0, "%d", end);
}


/* Manipulating the "cursor" cursor (see mbuf_beg_*() above) */
int mbuf_cur_get (const struct mbuf *mbuf)
{
	ENTER("mbuf %p", mbuf);
	if ( !mbuf )
		RETURN_ERRNO_VALUE(EFAULT, "%d", -1);

	RETURN_ERRNO_VALUE(0, "%d", mbuf->cur);
}

int mbuf_cur_set (struct mbuf *mbuf, int cur)
{
	ENTER("mbuf %p, cur %d", mbuf, cur);
	if ( !mbuf )
		RETURN_ERRNO_VALUE(EFAULT, "%d", -1);

	if ( cur < 0 || cur >= mbuf->max )
		RETURN_ERRNO_VALUE(EINVAL, "%d", -1);

	if ( mbuf->beg > cur ) mbuf->beg = cur;
	if ( mbuf->end < cur ) mbuf->end = cur;

	mbuf->cur = cur;
	RETURN_ERRNO_VALUE(0, "%d", cur);
}


/* Return a pointer into the buffer at the requested offset */
void *mbuf_beg_ptr (struct mbuf *mbuf)
{
	ENTER("mbuf %p", mbuf);
	if ( !mbuf )
		RETURN_ERRNO_VALUE(EFAULT, "%p", NULL);

	if ( mbuf->beg < 0 || mbuf->beg >= mbuf->max )
		RETURN_ERRNO_VALUE(EINVAL, "%p", NULL);

	RETURN_ERRNO_VALUE(0, "%p", mbuf->buf + mbuf->beg);
}

void *mbuf_end_ptr (struct mbuf *mbuf)
{
	ENTER("mbuf %p", mbuf);
	if ( !mbuf )
		RETURN_ERRNO_VALUE(EFAULT, "%p", NULL);

	if ( mbuf->end < 0 || mbuf->end > mbuf->max )
		RETURN_ERRNO_VALUE(EINVAL, "%p", NULL);

	RETURN_ERRNO_VALUE(0, "%p", mbuf->buf + mbuf->end);
}

void *mbuf_cur_ptr (struct mbuf *mbuf)
{
	ENTER("mbuf %p", mbuf);
	if ( !mbuf )
		RETURN_ERRNO_VALUE(EFAULT, "%p", NULL);

	if ( mbuf->cur < 0 || mbuf->cur >= mbuf->max )
		RETURN_ERRNO_VALUE(EINVAL, "%p", NULL);

	RETURN_ERRNO_VALUE(0, "%p", mbuf->buf + mbuf->cur);
}

void *mbuf_max_ptr (struct mbuf *mbuf)
{
	ENTER("mbuf %p", mbuf);
	if ( !mbuf )
		RETURN_ERRNO_VALUE(EFAULT, "%p", NULL);

	RETURN_ERRNO_VALUE(0, "%p", mbuf->buf + mbuf->max);
}


/******** Get from mbuf into variables ********/


/* Return number of bytes available for read (between cur and end), or -1 if invalid */
int mbuf_get_avail (struct mbuf *mbuf)
{
	ENTER("mbuf %p", mbuf);
	if ( !mbuf )
		RETURN_ERRNO_VALUE(EFAULT, "%d", -1);

	if ( mbuf->cur >= mbuf->max || mbuf->end > mbuf->max )
		RETURN_ERRNO_VALUE(EINVAL, "%d", -1);

	RETURN_ERRNO_VALUE(0, "%d", mbuf->end - mbuf->cur);
}


/* Get numeric values from the mbuf at the cursor, which is post-incremented.  The _be and
 * _le functions consume big- and little- endian respectively, the _n functions perform no
 * endian conversion.  Return the number of bytes, or <1 on error */
int mbuf_get_8 (struct mbuf *mbuf, uint8_t *val)
{
	ENTER("mbuf %p, val %p", mbuf, val);
	if ( mbuf_get_avail(mbuf) < sizeof(uint8_t) )
		RETURN_VALUE("%d", -1);

	*val = mbuf->buf[mbuf->cur++];

	RETURN_ERRNO_VALUE(0, "%zu", sizeof(uint8_t));
}

int mbuf_get_n16 (struct mbuf *mbuf, uint16_t *val)
{
	ENTER("mbuf %p, val %p", mbuf, val);
	if ( mbuf_get_avail(mbuf) < sizeof(uint16_t) )
		RETURN_VALUE("%d", -1);

	memcpy(val, mbuf->buf + mbuf->cur, sizeof(uint16_t));
	mbuf->cur += sizeof(uint16_t);

	RETURN_ERRNO_VALUE(0, "%zu", sizeof(uint16_t));
}

int mbuf_get_be16 (struct mbuf *mbuf, uint16_t *val)
{
	ENTER("mbuf %p, val %p", mbuf, val);
	if ( mbuf_get_avail(mbuf) < sizeof(uint16_t) )
		RETURN_VALUE("%d", -1);

	const uint8_t *ptr = mbuf->buf + mbuf->cur;

	*val  = *ptr++; *val <<= 8;
	*val |= *ptr++;

	mbuf->cur = ptr - mbuf->buf;

	RETURN_ERRNO_VALUE(0, "%zu", sizeof(uint16_t));
}

int mbuf_get_le16 (struct mbuf *mbuf, uint16_t *val)
{
	ENTER("mbuf %p, val %p", mbuf, val);
	if ( mbuf_get_avail(mbuf) < sizeof(uint16_t) )
		RETURN_VALUE("%d", -1);

	mbuf->cur += sizeof(uint16_t);
	const uint8_t *ptr = mbuf->buf + mbuf->cur - 1;

	*val  = *ptr--; *val <<= 8;
	*val |= *ptr;

	RETURN_ERRNO_VALUE(0, "%zu", sizeof(uint16_t));
}

int mbuf_get_n32 (struct mbuf *mbuf, uint32_t *val)
{
	ENTER("mbuf %p, val %p", mbuf, val);
	if ( mbuf_get_avail(mbuf) < sizeof(uint32_t) )
		RETURN_VALUE("%d", -1);

	memcpy(val, mbuf->buf + mbuf->cur, sizeof(uint32_t));
	mbuf->cur += sizeof(uint32_t);

	RETURN_ERRNO_VALUE(0, "%zu", sizeof(uint32_t));
}

int mbuf_get_be32 (struct mbuf *mbuf, uint32_t *val)
{
	ENTER("mbuf %p, val %p", mbuf, val);
	if ( mbuf_get_avail(mbuf) < sizeof(uint32_t) )
		RETURN_VALUE("%d", -1);

	const uint8_t *ptr = mbuf->buf + mbuf->cur;

	*val  = *ptr++; *val <<= 8;
	*val |= *ptr++; *val <<= 8;
	*val |= *ptr++; *val <<= 8;
	*val |= *ptr++;

	mbuf->cur = ptr - mbuf->buf;

	RETURN_ERRNO_VALUE(0, "%zu", sizeof(uint32_t));
}

int mbuf_get_le32 (struct mbuf *mbuf, uint32_t *val)
{
	ENTER("mbuf %p, val %p", mbuf, val);
	if ( mbuf_get_avail(mbuf) < sizeof(uint32_t) )
		RETURN_VALUE("%d", -1);

	mbuf->cur += sizeof(uint32_t);
	const uint8_t *ptr = mbuf->buf + mbuf->cur - 1;

	*val  = *ptr--; *val <<= 8;
	*val |= *ptr--; *val <<= 8;
	*val |= *ptr--; *val <<= 8;
	*val |= *ptr;

	RETURN_ERRNO_VALUE(0, "%zu", sizeof(uint32_t));
}

int mbuf_get_n64 (struct mbuf *mbuf, uint64_t *val)
{
	ENTER("mbuf %p, val %p", mbuf, val);
	if ( mbuf_get_avail(mbuf) < sizeof(uint64_t) )
		RETURN_VALUE("%d", -1);

	memcpy(val, mbuf->buf + mbuf->cur, sizeof(uint64_t));
	mbuf->cur += sizeof(uint64_t);

	RETURN_ERRNO_VALUE(0, "%zu", sizeof(uint64_t));
}

int mbuf_get_be64 (struct mbuf *mbuf, uint64_t *val)
{
	ENTER("mbuf %p, val %p", mbuf, val);
	if ( mbuf_get_avail(mbuf) < sizeof(uint64_t) )
		RETURN_VALUE("%d", -1);

	const uint8_t *ptr = mbuf->buf + mbuf->cur;

	*val  = *ptr++; *val <<= 8;
	*val |= *ptr++; *val <<= 8;
	*val |= *ptr++; *val <<= 8;
	*val |= *ptr++; *val <<= 8;
	*val |= *ptr++; *val <<= 8;
	*val |= *ptr++; *val <<= 8;
	*val |= *ptr++; *val <<= 8;
	*val |= *ptr++;

	mbuf->cur = ptr - mbuf->buf;

	RETURN_ERRNO_VALUE(0, "%zu", sizeof(uint64_t));
}

int mbuf_get_le64 (struct mbuf *mbuf, uint64_t *val)
{
	ENTER("mbuf %p, val %p", mbuf, val);
	if ( mbuf_get_avail(mbuf) < sizeof(uint64_t) )
		RETURN_VALUE("%d", -1);

	mbuf->cur += sizeof(uint64_t);
	const uint8_t *ptr = mbuf->buf + mbuf->cur - 1;

	*val  = *ptr--; *val <<= 8;
	*val |= *ptr--; *val <<= 8;
	*val |= *ptr--; *val <<= 8;
	*val |= *ptr--; *val <<= 8;
	*val |= *ptr--; *val <<= 8;
	*val |= *ptr--; *val <<= 8;
	*val |= *ptr--; *val <<= 8;
	*val |= *ptr;

	RETURN_ERRNO_VALUE(0, "%zu", sizeof(uint64_t));
}


/* Read from an mbuf into a user-provided buffer upto len bytes, returning the number
 * copied, which may be smaller */
int mbuf_get_mem (struct mbuf *mbuf, void *ptr, int len)
{
	ENTER("mbuf %p, ptr %p, len %d", mbuf, ptr, len);
	if ( !mbuf )
		RETURN_ERRNO_VALUE(EFAULT, "%d", -1);

	if ( mbuf->cur >= mbuf->max || mbuf->end > mbuf->max )
		RETURN_ERRNO_VALUE(EINVAL, "%d", -1);

	if ( (mbuf->end - mbuf->cur) < len )
		len = mbuf->end - mbuf->cur;
	if ( len < 1 )
		RETURN_ERRNO_VALUE(0, "%d", len);

	memcpy(ptr, mbuf->buf + mbuf->cur, len);
	mbuf->cur += len;

	RETURN_ERRNO_VALUE(0, "%d", len);
}


/******** Set into mbuf from variables ********/


/* Return number of bytes available (between cur and max), or -1 if invalid */
int mbuf_set_avail (struct mbuf *mbuf)
{
	ENTER("mbuf %p", mbuf);
	if ( !mbuf )
		RETURN_ERRNO_VALUE(EFAULT, "%d", -1);

	if ( mbuf->cur >= mbuf->max || mbuf->end > mbuf->max )
		RETURN_ERRNO_VALUE(EINVAL, "%d", -1);

	RETURN_ERRNO_VALUE(0, "%zu", mbuf->max - mbuf->cur);
}


/* Set numeric values in the mbuf at the cursor, which is post-incremented.  If the new
 * cursor value is past the end pointer, the end pointer will be set to match it.  The _be
 * and _le functions produce big- and little- endian respectively, the _n functions
 * perform no endian conversion.  Return the number of bytes, or <1 on error */
int mbuf_set_8 (struct mbuf *mbuf, uint8_t val)
{
	ENTER("mbuf %p, %02x", mbuf, val);
	if ( mbuf_set_avail(mbuf) < sizeof(uint8_t) )
		RETURN_VALUE("%d", -1);

	mbuf->buf[mbuf->cur++] = val;

	if ( mbuf->end <= mbuf->cur )
		mbuf->end = mbuf->cur;

	RETURN_ERRNO_VALUE(0, "%zu", sizeof(uint8_t));
}

int mbuf_set_n16 (struct mbuf *mbuf, uint16_t val)
{
	ENTER("mbuf %p, val %04x", mbuf, val);
	if ( mbuf_set_avail(mbuf) < sizeof(uint16_t) )
		RETURN_VALUE("%d", -1);

	memcpy(mbuf->buf + mbuf->cur, &val, sizeof(uint16_t));
	mbuf->cur += sizeof(uint16_t);

	if ( mbuf->end <= mbuf->cur )
		mbuf->end = mbuf->cur;

	RETURN_ERRNO_VALUE(0, "%zu", sizeof(uint16_t));
}

int mbuf_set_be16 (struct mbuf *mbuf, uint16_t val)
{
	ENTER("mbuf %p, val %04x", mbuf, val);
	if ( mbuf_set_avail(mbuf) < sizeof(uint16_t) )
		RETURN_VALUE("%d", -1);

	mbuf->cur += sizeof(uint16_t);
	uint8_t *ptr = mbuf->buf + mbuf->cur - 1;

	*ptr-- = val & 0xFF; val >>= 8;
	*ptr   = val;

	if ( mbuf->end <= mbuf->cur )
		mbuf->end = mbuf->cur;

	RETURN_ERRNO_VALUE(0, "%zu", sizeof(uint16_t));
}

int mbuf_set_le16 (struct mbuf *mbuf, uint16_t val)
{
	ENTER("mbuf %p, val %04x", mbuf, val);
	if ( mbuf_set_avail(mbuf) < sizeof(uint16_t) )
		RETURN_VALUE("%d", -1);

	uint8_t *ptr = mbuf->buf + mbuf->cur;

	*ptr++ = val & 0xFF; val >>= 8;
	*ptr++ = val;

	mbuf->cur = ptr - mbuf->buf;

	if ( mbuf->end <= mbuf->cur )
		mbuf->end = mbuf->cur;

	RETURN_ERRNO_VALUE(0, "%zu", sizeof(uint16_t));
}

int mbuf_set_n32 (struct mbuf *mbuf, uint32_t val)
{
	ENTER("mbuf %p, %08x", mbuf, val);
	if ( mbuf_set_avail(mbuf) < sizeof(uint32_t) )
		RETURN_VALUE("%d", -1);

	memcpy(mbuf->buf + mbuf->cur, &val, sizeof(uint32_t));
	mbuf->cur += sizeof(uint32_t);

	if ( mbuf->end <= mbuf->cur )
		mbuf->end = mbuf->cur;

	RETURN_ERRNO_VALUE(0, "%zu", sizeof(uint32_t));
}

int mbuf_set_be32 (struct mbuf *mbuf, uint32_t val)
{
	ENTER("mbuf %p, %08x", mbuf, val);
	if ( mbuf_set_avail(mbuf) < sizeof(uint32_t) )
		RETURN_VALUE("%d", -1);

	mbuf->cur += sizeof(uint32_t);
	uint8_t *ptr = mbuf->buf + mbuf->cur - 1;

	*ptr-- = val & 0xFF; val >>= 8;
	*ptr-- = val & 0xFF; val >>= 8;
	*ptr-- = val & 0xFF; val >>= 8;
	*ptr   = val;

	if ( mbuf->end <= mbuf->cur )
		mbuf->end = mbuf->cur;

	RETURN_ERRNO_VALUE(0, "%zu", sizeof(uint32_t));
}

int mbuf_set_le32 (struct mbuf *mbuf, uint32_t val)
{
	ENTER("mbuf %p, %08x", mbuf, val);
	if ( mbuf_set_avail(mbuf) < sizeof(uint32_t) )
		RETURN_VALUE("%d", -1);

	uint8_t *ptr = mbuf->buf + mbuf->cur;

	*ptr++ = val & 0xFF; val >>= 8;
	*ptr++ = val & 0xFF; val >>= 8;
	*ptr++ = val & 0xFF; val >>= 8;
	*ptr++ = val;

	mbuf->cur = ptr - mbuf->buf;

	if ( mbuf->end <= mbuf->cur )
		mbuf->end = mbuf->cur;

	RETURN_ERRNO_VALUE(0, "%zu", sizeof(uint32_t));
}

int mbuf_set_n64 (struct mbuf *mbuf, uint64_t val)
{
	ENTER("mbuf %p, val %016llx", mbuf, val);
	if ( mbuf_set_avail(mbuf) < sizeof(uint64_t) )
		RETURN_VALUE("%d", -1);

	memcpy(mbuf->buf + mbuf->cur, &val, sizeof(uint64_t));
	mbuf->cur += sizeof(uint16_t);

	if ( mbuf->end <= mbuf->cur )
		mbuf->end = mbuf->cur;

	RETURN_ERRNO_VALUE(0, "%zu", sizeof(uint64_t));
}

int mbuf_set_be64 (struct mbuf *mbuf, uint64_t val)
{
	ENTER("mbuf %p, val %016llx", mbuf, val);
	if ( mbuf_set_avail(mbuf) < sizeof(uint64_t) )
		RETURN_VALUE("%d", -1);

	mbuf->cur += sizeof(uint64_t);
	uint8_t *ptr = mbuf->buf + mbuf->cur - 1;

	*ptr-- = val & 0xFF; val >>= 8;
	*ptr-- = val & 0xFF; val >>= 8;
	*ptr-- = val & 0xFF; val >>= 8;
	*ptr-- = val & 0xFF; val >>= 8;
	*ptr-- = val & 0xFF; val >>= 8;
	*ptr-- = val & 0xFF; val >>= 8;
	*ptr-- = val & 0xFF; val >>= 8;
	*ptr   = val;

	if ( mbuf->end <= mbuf->cur )
		mbuf->end = mbuf->cur;

	RETURN_ERRNO_VALUE(0, "%zu", sizeof(uint64_t));
}

int mbuf_set_le64 (struct mbuf *mbuf, uint64_t val)
{
	ENTER("mbuf %p, val %016llx", mbuf, val);
	if ( mbuf_set_avail(mbuf) < sizeof(uint64_t) )
		RETURN_VALUE("%d", -1);

	uint8_t *ptr = mbuf->buf + mbuf->cur;

	*ptr++ = val & 0xFF; val >>= 8;
	*ptr++ = val & 0xFF; val >>= 8;
	*ptr++ = val & 0xFF; val >>= 8;
	*ptr++ = val & 0xFF; val >>= 8;
	*ptr++ = val & 0xFF; val >>= 8;
	*ptr++ = val & 0xFF; val >>= 8;
	*ptr++ = val & 0xFF; val >>= 8;
	*ptr++ = val;

	mbuf->cur = ptr - mbuf->buf;

	if ( mbuf->end <= mbuf->cur )
		mbuf->end = mbuf->cur;

	RETURN_ERRNO_VALUE(0, "%zu", sizeof(uint64_t));
}


/* Write into an mbuf from a user-provided buffer upto len bytes, returning the number
 * copied, which may be smaller, or <0 on error */
int mbuf_set_mem (struct mbuf *mbuf, void *ptr, int len)
{
	ENTER("mbuf %p, ptr %p, len %d", mbuf, ptr, len);
	if ( !mbuf || !ptr )
		RETURN_ERRNO_VALUE(EFAULT, "%d", -1);

	if ( len < 0 )
		len = strlen(ptr);

	if ( (mbuf->max - mbuf->cur) < len )
		len = mbuf->max - mbuf->cur;
	if ( len < 1 )
		RETURN_ERRNO_VALUE(ENOSPC, "%d", len);

	memcpy(mbuf->buf + mbuf->cur, ptr, len);
	mbuf->cur += len;

	if ( mbuf->end <= mbuf->cur )
		mbuf->end = mbuf->cur;

	RETURN_ERRNO_VALUE(0, "%d", len);
}


/* Use vsnprintf() to format a string into the buffer.  max gives the maximum number of
 * characters to insert, adjusted for room left in the buffer, and includes a terminating
 * NUL.  returns the number of characters written, not including the terminating NUL, or
 * <0 on error. */
int mbuf_printf (struct mbuf *mbuf, int max, const char *fmt, ...)
{
	ENTER("mbuf %p, max %d, fmt '%s', ...", mbuf, max, fmt);
	if ( !mbuf || !fmt )
		RETURN_ERRNO_VALUE(EFAULT, "%d", -1);

	if ( max > mbuf->max - mbuf->cur )
		max = mbuf->max - mbuf->cur;

	if ( max < 1 )
		RETURN_ERRNO_VALUE(ENOSPC, "%d", 0);

	va_list  ap;
	int      ret;
	int      err;

	va_start(ap, fmt);
	errno = 0;
	ret = vsnprintf((char *)(mbuf->buf + mbuf->cur), max, fmt, ap);
	err = errno;
	va_end(ap);

	// From snprintf(8): ... vsnprintf() do not write more than size bytes (including
	// the terminating null byte ('\0')).  If the output was truncated due to this limit
	// then the return value is the number of characters (excluding the terminating null
	// byte) which would have been written to the final string if enough space had been
	// available.  Thus, a return value of size or more means that the output was
	// truncated. 
	if ( ret > max )
		ret = max - 1;

	if ( ret > 0 )
		mbuf->cur += ret;

	if ( mbuf->end <= mbuf->cur )
		mbuf->end = mbuf->cur;

	RETURN_ERRNO_VALUE(err, "%d", ret);
}


/* Use memset() to set the value of len bytes in the buffer, starting at the cursor.
 * returns the number of bytes set, which may be smaller than len, or <0 on error */
int mbuf_fill (struct mbuf *mbuf, uint8_t val, int len)
{
	ENTER("mbuf %p, val %02x, len %d", mbuf, val, len);
	if ( !mbuf )
		RETURN_ERRNO_VALUE(EFAULT, "%d", -1);

	if ( (mbuf->max - mbuf->cur) < len )
		len = mbuf->max - mbuf->cur;
	if ( len < 1 )
		RETURN_ERRNO_VALUE(ENOSPC, "%d", len);

	memset(mbuf->buf + mbuf->cur, val, len);
	mbuf->cur += len;

	if ( mbuf->end <= mbuf->cur )
		mbuf->end = mbuf->cur;

	RETURN_ERRNO_VALUE(0, "%d", len);
}


/******** Read/Write mbuf from/to file descriptors ********/


/* Read upto max bytes into the mbuf from a file descriptor, limited by the number of
 * bytes of room in the buffer.  Advances the cursor by the number of bytes read, and
 * returns the number of bytes, or <0 on error. */
int mbuf_read (struct mbuf *mbuf, int fd, int max)
{
	ENTER("mbuf %p, fd %d, max %d", mbuf, fd, max);
	if ( !mbuf )
		RETURN_ERRNO_VALUE(EFAULT, "%d", -1);

	if ( (mbuf->max - mbuf->cur) < max )
		max = mbuf->max - mbuf->cur;
	if ( max < 1 )
		RETURN_ERRNO_VALUE(ENOSPC, "%d", max);

	errno = 0;
	int ret = read(fd, mbuf->buf + mbuf->cur, max);
	if ( ret < 1 )
		RETURN_VALUE("%d", ret);

	mbuf->cur += ret;
	if ( mbuf->end <= mbuf->cur )
		mbuf->end = mbuf->cur;

	RETURN_VALUE("%d", ret);
}


/* Write upto max bytes from the mbuf to a file descriptor, limited by the number of bytes
 * present in the buffer.  Advances the cursor by the number of bytes written, and returns
 * the number of bytes, or <0 on error. */
int mbuf_write (struct mbuf *mbuf, int fd, int max)
{
	ENTER("mbuf %p, fd %d, max %d", mbuf, fd, max);
	if ( !mbuf )
		RETURN_ERRNO_VALUE(EFAULT, "%d", -1);

	if ( mbuf->cur >= mbuf->max || mbuf->end > mbuf->max )
		RETURN_ERRNO_VALUE(EINVAL, "%d", -1);

	if ( (mbuf->end - mbuf->cur) < max )
		max = mbuf->end - mbuf->cur;
	if ( max < 1 )
		RETURN_ERRNO_VALUE(0, "%d", max);

	errno = 0;
	int ret = write(fd, mbuf->buf + mbuf->cur, max);
	if ( ret < 1 )
		RETURN_VALUE("%d", ret);

	mbuf->cur += ret;
	if ( mbuf->end <= mbuf->cur )
		mbuf->end = mbuf->cur;

	RETURN_VALUE("%d", ret);
}


/******** Debug: dump mbuf to log ********/


void _mbuf_dump (int level, const struct mbuf *mbuf, const char *file, int line,
                 const char *var)
{
	if ( level < LOG_LEVEL_DEBUG )
		return;

	if ( !mbuf )
	{
		log_printf(level, module_level, file, line, "calling mbuf_dump(NULL)\n");
		return;
	}

	if ( var && *var )
		log_printf(level, module_level, file, line,
		           "mbuf %p('%s'), ref %d, max %zu, beg %d%s, cur %d%s, end %d%s:\n",
		           mbuf, var, mbuf->ref, mbuf->max,
		           mbuf->beg, (mbuf->beg < 0 || mbuf->beg >= mbuf->max) ? "!" : "",
		           mbuf->cur, (mbuf->cur < 0 || mbuf->cur >  mbuf->max) ? "!" : "",
		           mbuf->end, (mbuf->end < 0 || mbuf->end >  mbuf->max) ? "!" : "");
	else
		log_printf(level, module_level, file, line,
		           "mbuf %p(\?\?\?), ref %d, max %zu, beg %d%s, cur %d%s, end %d%s:\n",
		           mbuf, mbuf->ref, mbuf->max,
		           mbuf->beg, (mbuf->beg < 0 || mbuf->beg >= mbuf->max) ? "!" : "",
		           mbuf->cur, (mbuf->cur < 0 || mbuf->cur >  mbuf->max) ? "!" : "",
		           mbuf->end, (mbuf->end < 0 || mbuf->end >  mbuf->max) ? "!" : "");

	const uint8_t *ptr = mbuf->buf;
	size_t         ofs = mbuf->max;
	uint8_t        dup[16];
	int            sup = 0;

	dup[0] = ~ *ptr;
	while ( ofs >= 16 )
	{
		if ( memcmp(dup, ptr, 16) )
		{
			if ( sup )
			{
				log_printf(level, module_level, file, line, "* (%d duplicates)\n", sup);
				sup = 0;
			}
			log_hexdump_line(ptr, mbuf->buf, 16);
			memcpy(dup, ptr, 16);
		}
		else
			sup++;

		ofs -= 16;
		ptr += 16;
	}
	if ( sup )
		log_printf(level, module_level, file, line, "* (%d duplicates)\n", sup);

	if ( ofs )
		log_hexdump_line(ptr, mbuf->buf, ofs);

	unsigned lpad, upad;
	memcpy(&lpad, mbuf->buf - sizeof(unsigned), sizeof(unsigned));
	memcpy(&upad, mbuf->buf + mbuf->max,        sizeof(unsigned));
	log_printf(level, module_level, file, line,
	           "mbuf pads: lower %x, upper %x\n",
	           lpad, upad);
}


#ifdef UNIT_TEST
#define TEST(test,want) \
	do{ \
		int tmp = (test); \
		printf("%d: %s: %d: ", __LINE__, #test, tmp); \
		if ( tmp != want ) { \
			printf("failed, %s\n", strerror(errno)); \
			return 1; \
		} \
		printf("pass\n"); \
	}while(0)

#define CHECK(test,want) \
	do{ \
		unsigned long long tmp = (unsigned long long)(test); \
		printf("%d: (%s) == (%s): ", __LINE__, #test, #want); \
		if ( tmp != want ) { \
			printf("have 0x%llx/%llu, want 0x%llx/%llu: fail\n", tmp, tmp, \
			       (unsigned long long)(want), (unsigned long long)(want)); \
			return 1; \
		} \
		printf("0x%llx/%llu: pass\n", tmp, tmp); \
	}while(0)


int main (int argc, char **argv)
{
	module_level = LOG_LEVEL_DEBUG;
	log_dupe (stderr);
	setbuf   (stderr, NULL);
	setbuf   (stdout, NULL);

	printf("Alloc(136, 8) test: ");
	struct mbuf *mbuf = mbuf_alloc(136, 8);
	if ( !mbuf )
	{
		perror("fail: mbuf_alloc");
		return 1;
	}
	printf("%p, pass\n", mbuf);

	printf("Size test: ");
	size_t size = mbuf_size(mbuf);
	if ( size != offsetof(struct mbuf, usr) + 8 + mbuf->max + (2 * sizeof(unsigned)) )
	{
		printf("fail: calc %zu, should be %zu\n", size,
		       offsetof(struct mbuf, usr) + 8 + mbuf->max + (2 * sizeof(unsigned)));
		return 1;
	}
	printf("%zu, pass\n", size);

	// paint "user" portion with test pattern
	printf("User test: ");
	char *user = "<-user->";
	memcpy(mbuf_user(mbuf), user, 8);
	TEST(memcmp(mbuf_user(mbuf), user, 8), 0);
	printf("pass\n");

	// printf tests: format 7 digits plus NUL at offset 0.  mbuf_printf() returns the
	// number of non-NUL characters inserted, though it does put the NUL in at position 8,
	// so adjust the cursor up by 1 for the following test.
	printf("\nmbuf_printf test\n");
	TEST(mbuf_cur_set(mbuf, 0), 0);
	TEST(mbuf_printf(mbuf, 8, "%ld", 1234567890), 7);
	TEST(mbuf_cur_adj(mbuf, 1), 8);
	mbuf_dump(mbuf);

	// readback test: depends on previous test, re-read into a painted buffer
	printf("\nmbuf_get_mem test\n");
	uint8_t have[16];
	memset(have, 0xFF, sizeof(have));
	TEST(mbuf_cur_set(mbuf, 0), 0);
	TEST(mbuf_get_mem(mbuf, have + 4, 8), 8);

	// depends on previous test: compare painted buffer with expectation
	printf("\ncompare readback test\n");
	uint8_t want[16] = 
	{
		0xFF, 0xFF, 0xFF, 0xFF, 
		'1', '2', '3', '4', '5', '6', '7', '\0',
		0xFF, 0xFF, 0xFF, 0xFF
	};
	log_hexdump_line(have, have, sizeof(have));
	log_hexdump_line(want, want, sizeof(want));
	if ( memcmp(have, want, 16) )
	{
		printf("fail\n");
		return 1;
	}
	printf("pass\n");

	// string/binary write test with readback: 
	char *string = "badgers! badgers!";
	printf("\nmbuf_set_mem test\n");
	TEST(mbuf_cur_set(mbuf, 0), 0);
	TEST(mbuf_set_mem(mbuf, string, 8), 8);

	printf("\nmbuf_get_mem test\n");
	memset(have, 0xFF, sizeof(have));
	TEST(mbuf_cur_set(mbuf, 0), 0);
	TEST(mbuf_get_mem(mbuf, have + 4, 8), 8);

	printf("\ncompare readback test\n");
	memcpy(want + 4, string, 8);
	log_hexdump_line(have, have, sizeof(have));
	log_hexdump_line(want, want, sizeof(want));
	if ( memcmp(have, want, 16) )
	{
		printf("fail\n");
		return 1;
	}
	printf("pass\n");

	// write tests: depends on previous test
	printf("\nmbuf_write_* tests\n");
	TEST(mbuf_cur_set(mbuf, 8), 8);

	TEST(mbuf_set_8(mbuf, 0x01), 1);
	TEST(mbuf_set_8(mbuf, 0x02), 1);
	TEST(mbuf_set_8(mbuf, 0x03), 1);
	TEST(mbuf_set_8(mbuf, 0x00), 1);

	TEST(mbuf_cur_set(mbuf, 16), 16);
	mbuf_dump(mbuf);

	TEST(mbuf_set_n16(mbuf, 0x1112),             2);
	TEST(mbuf_set_8(mbuf,   0x00),               1);
	TEST(mbuf_set_n32(mbuf, 0x21222324),         4);
	TEST(mbuf_set_8(mbuf,   0x00),               1);
	TEST(mbuf_set_n64(mbuf, 0x3132333435363738), 8);

	TEST(mbuf_cur_set(mbuf, 48), 48);
	mbuf_dump(mbuf);

	TEST(mbuf_set_be16(mbuf, 0x4142),             2);
	TEST(mbuf_set_8(mbuf,    0x00),               1);
	TEST(mbuf_set_be32(mbuf, 0x51525354),         4);
	TEST(mbuf_set_8(mbuf,    0x00),               1);
	TEST(mbuf_set_be64(mbuf, 0x6162636465666768), 8);

	TEST(mbuf_cur_set(mbuf, 80), 80);
	mbuf_dump(mbuf);

	TEST(mbuf_set_le16(mbuf, 0x7172),             2);
	TEST(mbuf_set_8(mbuf,    0x00),               1);
	TEST(mbuf_set_le32(mbuf, 0x81828384),         4);
	TEST(mbuf_set_8(mbuf,    0x00),               1);
	TEST(mbuf_set_le64(mbuf, 0x9192939495969798), 8);
	mbuf_dump(mbuf);

	TEST(mbuf_cur_set(mbuf, 120), 120);
	TEST(mbuf_set_le64(mbuf, 0xa1a2a3a4a5a6a7a8), 8);
	mbuf_dump(mbuf);

	uint8_t   u8;
	uint16_t  u16;
	uint32_t  u32;
	uint64_t  u64;

	// read tests: depends on previous test
	printf("\nmbuf_read_* tests\n");
	TEST(mbuf_cur_set(mbuf, 8), 8);

	TEST(mbuf_get_8(mbuf, &u8), 1);   CHECK(u8, 0x01);
	TEST(mbuf_get_8(mbuf, &u8), 1);   CHECK(u8, 0x02);
	TEST(mbuf_get_8(mbuf, &u8), 1);   CHECK(u8, 0x03);
	TEST(mbuf_get_8(mbuf, &u8), 1);   CHECK(u8, 0x00);

	TEST(mbuf_cur_set(mbuf, 16), 16);

	TEST(mbuf_get_n16(mbuf, &u16), 2);  CHECK(u16, 0x1112);
	TEST(mbuf_get_8(mbuf, &u8),    1);  CHECK(u8,  0x00);
	TEST(mbuf_get_n32(mbuf, &u32), 4);  CHECK(u32, 0x21222324);
	TEST(mbuf_get_8(mbuf, &u8),    1);  CHECK(u8,  0x00);
	TEST(mbuf_get_n64(mbuf, &u64), 8);  CHECK(u64, 0x3132333435363738);

	TEST(mbuf_cur_set(mbuf, 48), 48);

	TEST(mbuf_get_be16(mbuf, &u16), 2);  CHECK(u16, 0x4142);
	TEST(mbuf_get_8(mbuf, &u8),     1);  CHECK(u8,  0x00);
	TEST(mbuf_get_be32(mbuf, &u32), 4);  CHECK(u32, 0x51525354);
	TEST(mbuf_get_8(mbuf, &u8),     1);  CHECK(u8,  0x00);
	TEST(mbuf_get_be64(mbuf, &u64), 8);  CHECK(u64, 0x6162636465666768);

	TEST(mbuf_cur_set(mbuf, 80), 80);

	TEST(mbuf_get_le16(mbuf, &u16), 2);  CHECK(u16, 0x7172);
	TEST(mbuf_get_8(mbuf, &u8),     1);  CHECK(u8,  0x00);
	TEST(mbuf_get_le32(mbuf, &u32), 4);  CHECK(u32, 0x81828384);
	TEST(mbuf_get_8(mbuf, &u8),     1);  CHECK(u8,  0x00);
	TEST(mbuf_get_le64(mbuf, &u64), 8);  CHECK(u64, 0x9192939495969798);

	TEST(mbuf_cur_set(mbuf, 120), 120);
	TEST(mbuf_get_le64(mbuf, &u64), 8);  CHECK(u64, 0xa1a2a3a4a5a6a7a8);

	// byte fill test: should produce a buffer full of 0xee
	printf("\nmbuf_fill test\n");
	TEST(mbuf_cur_set(mbuf, 0), 0);
	TEST(mbuf_fill(mbuf, 0xee, mbuf->max), mbuf->max);
	mbuf_dump(mbuf);

	// test cursor sets: should be 10/50/120
	printf("\nCursor set test\n");
	TEST(mbuf_beg_set(mbuf, 10),   10);
	TEST(mbuf_cur_set(mbuf, 50),   50);
	TEST(mbuf_end_set(mbuf, 120), 120);
	printf("beg %d, cur %d, end %d, max %zu\n", mbuf->beg, mbuf->cur, mbuf->end, mbuf->max);

	// test cursor adjusts: depends on previous test. should all be += 5, so 15/55/125
	printf("\nCursor adjust test\n");
	TEST(mbuf_beg_adj(mbuf, -5),   5);
	TEST(mbuf_beg_adj(mbuf, 10),  15);
	TEST(mbuf_cur_adj(mbuf, -5),  45);
	TEST(mbuf_cur_adj(mbuf, 10),  55);
	TEST(mbuf_end_adj(mbuf, -5), 115);
	TEST(mbuf_end_adj(mbuf, 10), 125);
	printf("beg %d, cur %d, end %d, max %zu\n", mbuf->beg, mbuf->cur, mbuf->end, mbuf->max);

	// test cursor gets: depends on previous test
	printf("\nCursor get test\n");
	CHECK(mbuf_beg_get(mbuf), 15);
	CHECK(mbuf_cur_get(mbuf), 55);
	CHECK(mbuf_end_get(mbuf), 125);

	// test access: depends on previous test
	printf("\nCursor access test\n");
	CHECK((uint8_t *)mbuf_beg_ptr(mbuf) - mbuf->buf, 15);
	CHECK((uint8_t *)mbuf_cur_ptr(mbuf) - mbuf->buf, 55);
	CHECK((uint8_t *)mbuf_end_ptr(mbuf) - mbuf->buf, 125);
	CHECK((uint8_t *)mbuf_max_ptr(mbuf) - mbuf->buf, mbuf->max);

	// test avail space: zero counters, check zero read and max write. write some bytes,
	// recheck zero read and decreased write. reset cur to read, verify read avail and max
	// write
	printf("\nAvail space test\n");
	TEST(mbuf_beg_set(mbuf, 0), 0);
	TEST(mbuf_cur_set(mbuf, 0), 0);
	TEST(mbuf_end_set(mbuf, 0), 0);
	CHECK(mbuf_get_avail(mbuf), 0);
	CHECK(mbuf_set_avail(mbuf), mbuf->max);
	CHECK(mbuf_length(mbuf), 0);
	TEST(mbuf_set_le64(mbuf, 0xb1b2b3b4b5b6b7b8), 8);
	CHECK(mbuf_get_avail(mbuf), 0);
	CHECK(mbuf_set_avail(mbuf), mbuf->max - 8);
	CHECK(mbuf_length(mbuf), 8);
	TEST(mbuf_cur_set(mbuf, 0), 0);
	CHECK(mbuf_get_avail(mbuf), 8);
	CHECK(mbuf_set_avail(mbuf), mbuf->max);

	// test clone/copy to new mbuf
	printf("\nClone test:\n");
	mbuf->ext = mbuf;
	struct mbuf *clone = mbuf_clone(mbuf);
	if ( !clone )
	{
		perror("mbuf_clone");
		return 1;
	}
	LOG_HEXDUMP(clone, mbuf_size(clone));
	CHECK(clone->ref, 1);
	CHECK(mbuf_beg_get(clone), mbuf_beg_get(mbuf));
	CHECK(mbuf_cur_get(clone), mbuf_cur_get(mbuf));
	CHECK(mbuf_end_get(clone), mbuf_end_get(mbuf));
	CHECK(clone->max, mbuf->max);
	CHECK(clone->ext, (unsigned long long)NULL);
	CHECK(memcmp(mbuf_user(mbuf), mbuf_user(clone), 8), 0);
	CHECK(mbuf_length(clone), mbuf_length(mbuf));
	CHECK(memcmp(mbuf_beg_ptr(mbuf), mbuf_beg_ptr(clone), mbuf_length(clone)), 0);
	printf("%p: pass\n", clone);

	// get a temporary file
	printf("\nFile descriptor read/write test:\n");
	FILE *fp = tmpfile();
	if ( !fp )
	{
		perror("tmpfile");
		return 1;
	}
	int fd = fileno(fp);

	// write out mbuf to temporary file
	CHECK(mbuf_cur_set_beg(mbuf),  0);
	CHECK(mbuf_write(mbuf, fd, mbuf->max), 8);

	// seek back to beginning and read into clone
	CHECK(lseek(fd, 0, SEEK_SET), 0);
	CHECK(mbuf_cur_set_beg(clone), 0);
	CHECK(mbuf_read(clone, fd, clone->max), 8);

	// compare 
	CHECK(mbuf_length(clone), mbuf_length(mbuf));
	CHECK(memcmp(mbuf_beg_ptr(mbuf), mbuf_beg_ptr(clone), mbuf_length(clone)), 0);
	mbuf_deref(clone);

	// test upper/lower pads and corruption check in mbuf_free()
	printf("\nPaint / corruption detect test\n");
	uint8_t *p = mbuf->buf - 1;
	*p-- = 0x77;
	*p-- = 0x66;
	*p-- = 0x55;

	p = mbuf->buf + mbuf->max;
	*p++ = 0x77;
	*p++ = 0x88;
	*p++ = 0x99;

	// check "user" portion for corruption
	printf("\nUser corruption test\n");
	TEST(memcmp(mbuf_user(mbuf), user, 8), 0);

	printf("\nDump entire buffer:\n");
	LOG_HEXDUMP(mbuf, mbuf_size(mbuf));

	// test ref/deref and free
	printf("\nReference test:\n %d -> ", mbuf->ref);
	mbuf_ref(mbuf);
	printf("%d -> ", mbuf->ref);
	mbuf_deref(mbuf);
	printf("%d -> ", mbuf->ref);
	mbuf_deref(mbuf);
	printf("free\n");

	return 0;
}
#endif


/* Ends    : src/lib/mbuf.c */
