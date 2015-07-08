/*
 * File    : config_buffer.c
 * Descript: Variable type and file reader for config-file lines.  Handles:
 *           - backslash escape for non-unicode cases in printf(1):
 *             \" \\ \a \b \c \f \n \r \t \v 
 *             \NNN   1..3 digit octal character
 *             \xHH   1..2 digit hex character
 *           - EOL ignored following backslash for line continuation
 *           - EOL ignored inside quoted string for line continuation
 *           - comment character treated as EOL outside quoted string
 *
 * Version : 3.0
 * Author  : Morgan Hughes (morgan.hughes@silver-bullet-tech.com)
 * Copying : Copyright (C) 1998,1999,2000,2010,2011 Morgan Hughes
 *           All rights reserved.  For more info contact the address above.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#include "config.h"

#ifdef NDEBUG
#define dprintf(...) do{ }while(0)
#else
#define dprintf(fmt, ...) \
	do{ \
		fprintf(stderr, "%s[%d]: " fmt, __func__, __LINE__, ##__VA_ARGS__); \
	}while(0)
#endif



/** Init a growable config_buffer with a given initial size
 *
 *  \param   size  Initial size in bytes
 *  \param   grow  Step size in bytes
 *  \return  Zero on success, nonzero on alloc failure
 */
int config_buffer_init (struct config_buffer *buff, int size, int grow)
{
	if ( !(buff->buff = calloc(1, size)) )
	{
		free (buff);
		return -1;
	}

	buff->size = size;
	buff->grow = grow;
	buff->used = 0;
	buff->line = 0;

	errno = 0;
	return 0;
}


/** Allocate and initialize a growable config_buffer with a given initial size
 *
 *  \param   size  Initial size in bytes
 *  \param   grow  Step size in bytes
 *  \return  Pointer to allocated struct, or NULL on error
 */
struct config_buffer *config_buffer_alloc (int size, int grow)
{
	struct config_buffer *buff = calloc(1, sizeof(struct config_buffer));
	if ( !buff )
		return NULL;

	if ( config_buffer_init(buff, size, grow) )
	{
		free(buff);
		return NULL;
	}

	return buff;
}


/** Grow an existing config_buffer, preserving its contents
 *
 *  \param   buff  config_buffer to grow
 *  \return  New buffer size, or <0 on error
 */
int config_buffer_grow (struct config_buffer *buff)
{
	char *tmp;

	errno = EINVAL;
	if ( buff->grow < 1 )
		return -1;

	errno = ENOMEM;
	if ( !(tmp = calloc(1, buff->size + buff->grow)) )
		return -1;

	memcpy (tmp, buff->buff, buff->used);
	free (buff->buff);
	buff->buff  = tmp;
	buff->size += buff->grow;

	errno = 0;
	return buff->size;
}


/** Free dynamic fields in a config_buffer 
 *
 *  \param   buff  config_buffer to finalize
 */
void config_buffer_done (struct config_buffer *buff)
{
	free (buff->buff);
}


/** Free a growable config_buffer allocated with config_buffer_alloc()
 *
 *  \param   buff  config_buffer to free
 */
void config_buffer_free (struct config_buffer *buff)
{
	config_buffer_done(buff);
	free (buff);
}


/** Read a line from a file into the given config_buffer, growing it if needed
 *  and handling quotes, special characters, multi-line continuation, etc.
 *
 *  \param   buff  config_buffer to read into
 *  \param   hand  Open file handle to read lines from
 *  \param   opt   Bitmask of option bits
 *  \return  Number of bytes placed in buff, or <0 on error
 */
int config_buffer_read (struct config_buffer *buff, FILE *hand, int opt)
{
	char  line[64], *s, *d, *e, *l;
	char  conv[4];
	int   i;
	int   slash = 0;
	int   quote = 0;
	char  comment = opt & CONFIG_BUFFER_READ_OPT_COMMENT;
	int   c = 0, a = 0;

	buff->used = 0;
	d = buff->buff;
	l = line + sizeof(line) - 2;

	*l = '\0';
	while ( fgets(line, sizeof(line), hand) )
	{
		if ( !c++ )
			buff->line++;
		dprintf ("%d: raw read (%s)\n", buff->line, line);

		/* always skip leading spaces */
		for ( s = line; *s && isspace(*s); s++) ;

		d = buff->buff + buff->used;
		e = buff->buff + buff->size;
		while ( *s )
		{
			/* grow buffer, abort on failure */
			if ( d == e )
			{
				if ( config_buffer_grow(buff) < 0 )
					return -1;
				e = buff->buff + buff->size;
			}

			/* handling an escaped character */
			if ( slash )
			{
				/* convert 1-3 octal digits */
				if ( *s >= '0' && *s <= '7' )
				{
					for ( i = 0; s[i] >= '0' && s[i] <= '7' && i < 3; i++ )
						conv[i] = s[i];
					conv[i] = '\0';
					*d++ = strtoul(conv, NULL, 8) & 0xFF;
					s += i;
				}
				else switch ( *s )
				{
					/* convert special character */
					case 'a':  *d++ = '\a'; s++;  break;
					case 'b':  *d++ = '\b'; s++;  break;
					case 'f':  *d++ = '\f'; s++;  break;
					case 'n':  *d++ = '\n'; s++;  break;
					case 'r':  *d++ = '\r'; s++;  break;
					case 't':  *d++ = '\t'; s++;  break;
					case 'v':  *d++ = '\v'; s++;  break;
					case '\\': *d++ = '\\'; s++;  break;
					case '\'': *d++ = '\''; s++;  break;
					case '"':  *d++ = '"';  s++;  break;
					case '`':  *d++ = '`';  s++;  break;

					case '\n':
						*d++ = '\n';
						s++;
						a++;
						break;

 					/* convert one or more hex digits */
					case 'x':
						if ( isxdigit(s[1]) )
						{
							for ( i = 1; isxdigit(s[i]) && i < 3; i++ )
								conv[i] = s[i];
							conv[i] = '\0';
							*d++ = strtoul(conv + 1, NULL, 16) & 0xFF;
							s += i;
							break;
						}
						/* fallthrough for x not followed by an xdigit */

					/* copy regular character */
					default:
						*d++ = '\\';
						*d++ = *s++;
						break;
				}

				slash = 0;
			}

			/* escape character itself */
			else if ( *s == '\\' )
			{
				if ( opt & CONFIG_BUFFER_READ_OPT_SLASH )
					slash = 1;
				else
					*d++ = *s;
				s++;
			}

			/* already in a quoted string */
			else if ( quote )
			{
				/* exiting a quoted string: terminate there and return */
				if ( *s == quote )
					goto ret_success;
				*d++ = *s++;
				if ( *s == '\n' )
					a++;
			}

			/* entering a double-quoted string */
			else if ( *s == '"' && (opt & CONFIG_BUFFER_READ_OPT_QUOTE) )
				quote = *s++;

			/* unescaped newline or comment: set length, terminate string, and return */
			else if ( *s == '\n' || *s == '\r' || *s == comment )
			{
				/* special case for a long commented line: read the rest of the line
				 * until it's either less than full, or the last char is a newline */
				if ( *s == comment )
					while ( strlen(line) == (sizeof(line) - 1) &&
					        *l != '\n' &&
					         fgets(line, sizeof(line), hand) ) ;

				goto ret_success;
			}

			/* other unescaped character: just copy */
			else 
				*d++ = *s++;
		}
		buff->used = d - buff->buff;
		*l = '\0';
	}

	/* EOF: return -1 */
	if ( d == buff->buff )
		return -1;

ret_success:
	while ( d > buff->buff && isspace(*(d-1)) )
		d--;
	*d = '\0';
	buff->used = d - buff->buff;
	errno = 0;
	buff->line += a;
	return buff->used;
}


/** Discard bytes in the buffer
 *
 *  How this is handled internally depends on both size and buff->used
 *  - If size >= buff->used, the buffer is reset cheaply.  This should be the most common
 *    case when the caller writes out the entire buffer to a file, etc.
 *  - If size >= (buff->used - size), the remaining data does not overlap with the data
 *    being discarded, and a single memcpy() is used to move it to the buffer base
 *  - Otherwise the remaining data is moved with memmove()
 *
 *  \param  buff  Buffer to discard from
 *  \param  size  Number of bytes to discard
 */
void config_buffer_discard (struct config_buffer *buff, int size)
{
	if ( size >= buff->used )
	{
		buff->used = 0;
		return;
	}

	if ( size >= (buff->used - size) )
	{
		memcpy(buff->buff, buff->buff + size, buff->used - size);
		buff->used -= size;
		return;
	}

	memmove(buff->buff, buff->buff + size, buff->used - size);
	buff->used -= size;
}


/** Write data in the buffer to the destination file handle
 *
 *  The buffer will be adjusted with a call to config_buffer_discard()
 *
 *  \param  buff  Buffer to write from
 *  \param  hand  File handle to write to
 *  
 *  \return Number of bytes written, or <0 for error
 */
int config_buffer_write (struct config_buffer *buff, FILE *hand)
{
	int ret = fwrite(buff->buff, sizeof(char), buff->used, hand);
	if ( ret > 0 )
		config_buffer_discard(buff, ret);

	return ret;
}


#ifdef UNIT_TEST
#include <getopt.h>

static void proc (struct config_buffer *buff, FILE *ifp, int flags)
{
	int ret;
	while ( (ret = config_buffer_read(buff, ifp, flags)) >= 0 )
	{
		printf ("ret %d: line %d: used %d: ", ret, buff->line, buff->used);
		for ( ret = 0; ret < buff->used; ret++ )
			if ( isprint(buff->buff[ret]) )
				putc(buff->buff[ret], stdout);
			else
				printf ("\\x%02X", buff->buff[ret]);
		putc('\n', stdout);
		putc('\n', stdout);
	}
}

int main (int argc, char **argv)
{
	struct config_buffer *buff;
	int                   opt;
	int                   flag = CONFIG_BUFFER_READ_OPT_DEFAULT;
	int                   init = 256;
	int                   grow = 1024;
	FILE                 *ifp;

	while ( (opt = getopt(argc, argv, "i:g:o:c:")) != -1 )
		switch ( opt )
		{
			case 'i': init = strtoul(optarg, NULL, 0); break;
			case 'g': grow = strtoul(optarg, NULL, 0); break;
			case 'o': flag = (flag &  CONFIG_BUFFER_READ_OPT_COMMENT) | (strtoul(optarg, NULL, 0) & ~CONFIG_BUFFER_READ_OPT_COMMENT); break;
			case 'c': flag = (flag & ~CONFIG_BUFFER_READ_OPT_COMMENT) | (*optarg & ~CONFIG_BUFFER_READ_OPT_COMMENT); break;
				break;
		}

	if ( ! (buff = config_buffer_alloc(init)) )
	{
		printf ("config_buffer_alloc(%d) failed: %s\n", init, strerror(errno));
		return 1;
	}

	if ( optind == argc )
		proc (buff, stdin, flag);

	for ( ; optind < argc; optind++ )
		if ( (ifp = fopen(argv[optind], "r")) )
		{
			proc (buff, ifp, flag);
			fclose (ifp);
		}
		else
			printf ("%s: %s\n", argv[optind], strerror(errno));

	return 0;
}
#endif


/* Ends    : config_buffer.c */
