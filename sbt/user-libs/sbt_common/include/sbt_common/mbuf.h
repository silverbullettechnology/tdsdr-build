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
#ifndef INCLUDE_LIB_MBUF_H
#define INCLUDE_LIB_MBUF_H
#include <stdint.h>


#define MBUF_REF_DEBUG 1


/** Variable-size message buffer with reference counter and cursors.  It's actually
 *  allocated as a genlist node, and queues are implemented as genlists.  When allocating,
 *  the caller may specify a user structure size which will immediately follow the fixed
 *  fields and be accessible through mbuf_user().  The payload data will be allocated
 *  after the user structure, with safety paint before and after. */
struct mbuf
{
	int       ref;     /** Internal reference counter */
	int       beg;     /** Offset of first payload byte */
	int       cur;     /** Cursor for sequential read/write */
	int       end;     /** Offset of byte past last payload byte */
	size_t    max;     /** Size of buffer, max value for end */
	void     *ext;     /** Used by mqueue to prevent double-enqueue */
#ifdef MBUF_REF_DEBUG
	const char *file;  /** Caller filename of last reference operation */
	int         line;  /** Caller line number of last reference operation */
#endif
	uint8_t  *buf;     /** Buffer space */
	uint8_t   usr[0];  /** User struct space for use with offsetof() */
};


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
#define mbuf_alloc(data, user) _mbuf_alloc(data, user, __FILE__, __LINE__)
struct mbuf *_mbuf_alloc (size_t data, size_t user, const char *file, int line);


/** Increment a mbuf's reference count
 *
 *  \param mbuf mbuf to reference
 */
#define mbuf_ref(mbuf) _mbuf_ref(mbuf, __FILE__, __LINE__)
void _mbuf_ref (struct mbuf *mbuf, const char *file, int line);


/** Dereference a mbuf and free if reference count is 0
 *
 *  \param mbuf mbuf to deref
 */
#define mbuf_deref(mbuf) _mbuf_deref(mbuf, __FILE__, __LINE__)
void _mbuf_deref (struct mbuf *mbuf, const char *file, int line);


/** Free a mbuf allocated with mbuf_alloc_and_ref
 *
 *  \param mbuf mbuf to be freed
 */
#define mbuf_free(mbuf) _mbuf_free(mbuf, __FILE__, __LINE__)
void _mbuf_free (struct mbuf *mbuf, const char *file, int line);


/** Allocate a new mbuf with reference count 1 and clone from another mbuf
 *
 *  Caller must free the mbuf with mbuf_free() 
 *
 *  \note The user section will be cloned as well, and caller is responsible for reference
 *        counting and cleanup this implies.
 *
 *  \param mbuf mbuf to clone
 *
 *  \return pointer to mbuf structure
 */
#define mbuf_clone(mbuf) _mbuf_clone(mbuf, __FILE__, __LINE__);
struct mbuf *_mbuf_clone (struct mbuf *mbuf, const char *file, int line);


/******** Cursor manipulation ********/


/** Return the "begin" cursor
 *
 *  \param  mbuf  mbuf to examine
 *
 *  \return Value of the "begin" cursor, or <0 on error
 */
int mbuf_beg_get (const struct mbuf *mbuf);

/** Set the "begin" cursor
 *
 *  \param  mbuf  mbuf to modify
 *  \param  beg   new "begin" cursor
 *
 *  \return Value of the "begin" cursor after set, or <0 on error
 */
int mbuf_beg_set (struct mbuf *mbuf, int beg);

/** Adjust the "begin" cursor by a delta value
 *
 *  \param  mbuf  mbuf to modify
 *  \param  adj   amount to add to the "begin" cursor
 *
 *  \return Value of the "begin" cursor after adjust, or <0 on error
 */
static inline int mbuf_beg_adj (struct mbuf *mbuf, int adj)
{
	return mbuf_beg_set(mbuf, mbuf->beg + adj);
}

/** Set the "begin" cursor to the "end" cursor value
 *
 *  \param  mbuf  mbuf to modify
 *
 *  \return Value of the "begin" cursor after set, or <0 on error
 */
static inline int mbuf_beg_set_end (struct mbuf *mbuf)
{
	return mbuf_beg_set(mbuf, mbuf->end);
}

/** Set the "begin" cursor to the "current" cursor value
 *
 *  \param  mbuf  mbuf to modify
 *
 *  \return Value of the "begin" cursor after set, or <0 on error
 */
static inline int mbuf_beg_set_cur (struct mbuf *mbuf)
{
	return mbuf_beg_set(mbuf, mbuf->cur);
}


/** Return the "end" cursor
 *
 *  \param  mbuf  mbuf to examine
 *
 *  \return Value of the "end" cursor, or <0 on error
 */
int mbuf_end_get (const struct mbuf *mbuf);

/** Set the "end" cursor
 *
 *  \param  mbuf  mbuf to modify
 *  \param  beg   new "end" cursor
 *
 *  \return Value of the "end" cursor after set, or <0 on error
 */
int mbuf_end_set (struct mbuf *mbuf, int end);

/** Adjust the "end" cursor by a delta value
 *
 *  \param  mbuf  mbuf to modify
 *  \param  adj   amount to add to the "end" cursor
 *
 *  \return Value of the "end" cursor after adjust, or <0 on error
 */
static inline int mbuf_end_adj (struct mbuf *mbuf, int adj)
{
	return mbuf_end_set(mbuf, mbuf->end + adj);
}

/** Set the "end" cursor to the "begin" cursor value
 *
 *  \param  mbuf  mbuf to modify
 *
 *  \return Value of the "end" cursor after set, or <0 on error
 */
static inline int mbuf_end_set_beg (struct mbuf *mbuf)
{
	return mbuf_end_set(mbuf, mbuf->beg);
}

/** Set the "end" cursor to the "current" cursor value
 *
 *  \param  mbuf  mbuf to modify
 *
 *  \return Value of the "end" cursor after set, or <0 on error
 */
static inline int mbuf_end_set_cur (struct mbuf *mbuf)
{
	return mbuf_end_set(mbuf, mbuf->cur);
}


/** Return the "current" cursor
 *
 *  \param  mbuf  mbuf to examine
 *
 *  \return Value of the "current" cursor, or <0 on error
 */
int mbuf_cur_get (const struct mbuf *mbuf);

/** Set the "current" cursor
 *
 *  \param  mbuf  mbuf to modify
 *  \param  beg   new "current" cursor
 *
 *  \return Value of the "current" cursor after set, or <0 on error
 */
int mbuf_cur_set (struct mbuf *mbuf, int cur);

/** Adjust the "current" cursor by a delta value
 *
 *  \param  mbuf  mbuf to modify
 *  \param  adj   amount to add to the "current" cursor
 *
 *  \return Value of the "current" cursor after adjust, or <0 on error
 */
static inline int mbuf_cur_adj (struct mbuf *mbuf, int adj)
{
	return mbuf_cur_set(mbuf, mbuf->cur + adj);
}

/** Set the "current" cursor to the "begin" cursor value
 *
 *  \param  mbuf  mbuf to modify
 *
 *  \return Value of the "current" cursor after set, or <0 on error
 */
static inline int mbuf_cur_set_beg (struct mbuf *mbuf)
{
	return mbuf_cur_set(mbuf, mbuf->beg);
}

/** Set the "current" cursor to the "end" cursor value
 *
 *  \param  mbuf  mbuf to modify
 *
 *  \return Value of the "current" cursor after set, or <0 on error
 */
static inline int mbuf_cur_set_end (struct mbuf *mbuf)
{
	return mbuf_cur_set(mbuf, mbuf->end);
}


/** Return number of payload bytes in the mbuf (between beg and end)
 *
 *  \param  mbuf  mbuf to examine
 *
 *  \return number of payload bytes
 */
static inline int mbuf_length (struct mbuf *mbuf)
{
	return mbuf->end - mbuf->beg;
}


/** Return pointer to the user data section of the mbuf
 *
 *  \param  mbuf  mbuf to examine
 *
 *  \return pointer to the user data section
 */
static inline void *mbuf_user (struct mbuf *mbuf)
{
	return &mbuf->usr[0];
}


/** Return a pointer into the payload buffer at the "begin" cursor
 *
 *  \param  mbuf  mbuf to examine
 *
 *  \return pointer into the buffer
 */
void *mbuf_beg_ptr (struct mbuf *mbuf);

/** Return a pointer into the payload buffer at the "end" cursor
 *
 *  \param  mbuf  mbuf to examine
 *
 *  \return pointer into the buffer
 */
void *mbuf_end_ptr (struct mbuf *mbuf);

/** Return a pointer into the payload buffer at the "current" cursor
 *
 *  \param  mbuf  mbuf to examine
 *
 *  \return pointer into the buffer
 */
void *mbuf_cur_ptr (struct mbuf *mbuf);

/** Return a pointer into the payload buffer at the "max" size
 *
 *  \param  mbuf  mbuf to examine
 *
 *  \return pointer into the buffer
 */
void *mbuf_max_ptr (struct mbuf *mbuf);


/******** Get from mbuf into variables ********/


/** Return number of bytes available to "get" between "current" and "end" cursors
 *
 *  \param  mbuf  mbuf to examine
 *
 *  \return number of bytes available or <0 on error
 */
int mbuf_get_avail (struct mbuf *mbuf);

/* Get numeric values from the mbuf at the cursor, which is post-incremented.  The _be and
 * _le functions consume big- and little- endian respectively, the _n functions perform no
 * endian conversion.  Return the number of bytes, or <1 on error */
int mbuf_get_8     (struct mbuf *mbuf, uint8_t  *val);
int mbuf_get_n16   (struct mbuf *mbuf, uint16_t *val);
int mbuf_get_be16  (struct mbuf *mbuf, uint16_t *val);
int mbuf_get_le16  (struct mbuf *mbuf, uint16_t *val);
int mbuf_get_n32   (struct mbuf *mbuf, uint32_t *val);
int mbuf_get_be32  (struct mbuf *mbuf, uint32_t *val);
int mbuf_get_le32  (struct mbuf *mbuf, uint32_t *val);
int mbuf_get_n64   (struct mbuf *mbuf, uint64_t *val);
int mbuf_get_be64  (struct mbuf *mbuf, uint64_t *val);
int mbuf_get_le64  (struct mbuf *mbuf, uint64_t *val);

/* Read from an mbuf into a user-provided buffer upto len bytes, returning the number
 * copied, which may be smaller */
int mbuf_get_mem (struct mbuf *mbuf, void *ptr, int len);


/******** Set into mbuf from variables ********/


/* Return number of bytes available (between cur and max), or -1 if invalid */
int mbuf_set_avail (struct mbuf *mbuf);

/* Set numeric values in the mbuf at the cursor, which is post-incremented.  If the new
 * cursor value is past the end pointer, the end pointer will be set to match it.  The _be
 * and _le functions produce big- and little- endian respectively, the _n functions
 * perform no endian conversion.  Return the number of bytes, or <1 on error */
int mbuf_set_8    (struct mbuf *mbuf, uint8_t   val);
int mbuf_set_n16  (struct mbuf *mbuf, uint16_t  val);
int mbuf_set_be16 (struct mbuf *mbuf, uint16_t  val);
int mbuf_set_le16 (struct mbuf *mbuf, uint16_t  val);
int mbuf_set_n32  (struct mbuf *mbuf, uint32_t  val);
int mbuf_set_be32 (struct mbuf *mbuf, uint32_t  val);
int mbuf_set_le32 (struct mbuf *mbuf, uint32_t  val);
int mbuf_set_n64  (struct mbuf *mbuf, uint64_t  val);
int mbuf_set_be64 (struct mbuf *mbuf, uint64_t  val);
int mbuf_set_le64 (struct mbuf *mbuf, uint64_t  val);

/* Write into an mbuf from a user-provided buffer upto len bytes, returning the number
 * copied, which may be smaller */
int mbuf_set_mem (struct mbuf *mbuf, void *ptr, int len);

/* Use vsnprintf() to format a string into the buffer.  max gives the maximum number of
 * characters to insert, adjusted for room left in the buffer, and includes a terminating
 * NUL.  returns the number of characters written, not including the terminating NUL, or
 * <0 on error. */
int mbuf_printf (struct mbuf *mbuf, int max, const char *fmt, ...);

/* Use memset() to set the value of len bytes in the buffer, starting at the cursor.
 * returns the number of bytes set, which may be smaller than len, or <0 on error */
int mbuf_fill (struct mbuf *mbuf, uint8_t val, int len);


/******** Read/Write mbuf from/to file descriptors ********/


/* Read upto max bytes into the mbuf from a file descriptor, limited by the number of
 * bytes of room in the buffer.  Advances the cursor by the number of bytes read, and
 * returns the number of bytes, or <0 on error. */
int mbuf_read (struct mbuf *mbuf, int fd, int max);

/* Write upto max bytes from the mbuf to a file descriptor, limited by the number of bytes
 * present in the buffer.  Advances the cursor by the number of bytes written, and returns
 * the number of bytes, or <0 on error. */
int mbuf_write (struct mbuf *mbuf, int fd, int max);


/******** Debug: dump mbuf to log ********/


/* Dump an mbuf at LOG_LEVEL_DEBUG if the module's level is set at least that high */
#define mbuf_dump(mbuf) _mbuf_dump(module_level, mbuf, __FILE__, __LINE__, #mbuf)
void _mbuf_dump (int level, const struct mbuf *mbuf, const char *file, int line,
                 const char *var);


#endif /* INCLUDE_LIB_MBUF_H */
/* Ends */
