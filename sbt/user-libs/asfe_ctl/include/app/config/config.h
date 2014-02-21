/** \file      config.h
 *  \brief     Headers for new config-file parser
 *  \copyright Copyright 2013,2014 Silver Bullet Technology
 *             Copyright (C) 1998,1999,2000,2010,2011 Morgan Hughes
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
#ifndef HAVE_CONFIG_H
#define HAVE_CONFIG_H
#include <stdio.h>


/* This is a pointer to a function of the prototype:
 *   int foo (const char *section, const char *tag, const char *val,
 *            const char *file, int line);
 *
 * The "section" parameter will always contain the [section] string from the input file,
 * to a maximum length of 127 bytes.  
 *
 * The "tag" and "val" parameters are used three distinct ways:
 * - When starting a new section, "tag" will be NULL and "val" will be the map pattern
 *   which matched the section name.
 * - When finishing a section, both "tag" and "val" will be NULL.
 * - While reading lines within a section, "tag" will contain the word left of the equals
 *   sign, "val" the line after.
 * 
 * The "file" and "line" parameters will give the config file name and line number being
 * parsed.
 *
 * The "data" parameter will be the "data" field in the matching entry in the section map
 *
 * If a default section name and function are given for a context, the function will be
 * called once after opening each file, for the start of the "implied" default section.
 * If the file contains an explicit [section] which matches the default, the function will
 * be * called as the parser finishes the "implied" default section and starts the expicit
 * default.
 *
 * The callback should return 0 on success, nonzero on failure.  Config parsing will be 
 * stopped and the _parse functions will return the value returned by the callback.
 */
typedef int (* config_func_t)(const char *section, const char *tag, const char *val,
                              const char *file, int line, void *data);


/* This structure maps section names to handler functions.  Section names may contain most
 * any reasonable characters, including letters, numbers, spaces, punctuation, etc.  
 *
 * If the section pattern contains a * it is interpreted as a wildcard match.  At present
 * the following forms are supported:
 *   "bob*"     matches "bob" and "bobby" but not "sirbob"
 *   "*bob"     matches "afterbob" but not "bobbit"
 *   "*bob*"    matches any of the above
 *   "*"        matches anything 
 */
struct config_section_map
{
	const char    *pattern;
	config_func_t  function;
	void          *data;
};


/* This structure defines a config context; allowing more complex programs to reuse the
 * config mechanism for datafiles etc.
 */
struct config_context
{
	// specify the default section name and callback function, if desired
	const char     *default_section;
	config_func_t   default_function;

	// mapping between section names and callback functions.  may be NULL if you're using
	// the catchall function instead
	struct config_section_map  *section_map;

	// if provided, any section which doesn't match a section will be passed to this
	// function instead
	config_func_t catchall_function;

	// caller-specified data structure, passed to the callback function
	void *data;
};


struct config_state
{
	char           section[128];
	config_func_t  function;
};

int config_parse_path (struct config_context *ctx, struct config_state *state,
                       const char *path);

int config_parse_file (struct config_context *ctx, struct config_state *state,
                       const char *file);

int config_parse_buff (struct config_context *ctx, struct config_state *state,
                       const char *file, int line, char *buff);


/* Read the config file, call any handlers, etc.  
 */
int config_parse (struct config_context *ctx, const char *path);



// TODO: add standard parsers and variable-registration stuff here



/** Structure for a growable config file line
 */
struct config_buffer
{
	int   size;  /* size of buffer allocation */
	int   used;  /* number of bytes used */
	int   grow;  /* number of bytes to grow by */
	int   line;  /* line number line begins on */
	char *buff;  /* pointer to buffer */
};


/* 
 */
#define CONFIG_BUFFER_READ_OPT_COMMENT   0x0FF
#define CONFIG_BUFFER_READ_OPT_SLASH     0x100
#define CONFIG_BUFFER_READ_OPT_QUOTE     0x200
#define CONFIG_BUFFER_READ_OPT_DEFAULT  (0x300 | '#')


/** Init a growable config_buffer with a given initial size
 *
 *  \param   size  Initial size in bytes
 *  \param   grow  Step size in bytes
 *  \return  Zero on success, nonzero on alloc failure
 */
int config_buffer_init (struct config_buffer *buff, int size, int grow);


/** Allocate and initialize a growable config_buffer with a given initial size
 *
 *  \param   size  Initial size in bytes
 *  \param   grow  Step size in bytes
 *  \return  Pointer to allocated struct, or NULL on error
 */
struct config_buffer *config_buffer_alloc (int size, int grow);


/** Grow an existing config_buffer, preserving its contents
 *
 *  \param   buff  config_buffer to grow
 *  \return  New buffer size, or <0 on error
 */
int config_buffer_grow (struct config_buffer *buff);


/** Free dynamic fields in a config_buffer 
 *
 *  \param   buff  config_buffer to finalize
 */
void config_buffer_done (struct config_buffer *buff);


/** Free a growable config_buffer allocated with config_buffer_alloc()
 *
 *  \param   buff  config_buffer to free
 */
void config_buffer_free (struct config_buffer *buff);


/** Append a fixed string to a config_buffer, growing it as necessary.
 *
 *  If len < 0, strlen() will be used to size str, which must be NUL-terminated.
 *  If len > 0, str may be binary and buff->used will be updated with the correct length.
 *  In either case a terminating NUL will be budgeted for and appended.
 *
 *  \param   buff  config_buffer to append to
 *  \param   str   string to append
 *  \param   len   length of string
 *  \return  buffer size after append, or <0 on error
 */
int config_buffer_append (struct config_buffer *buff, const char *str, int len);


/** Read a line from a file into the given config_buffer, growing it if needed
 *  and handling quotes and multi-line continuation
 *
 *  \param   buff  config_buffer to read into
 *  \param   hand  Open file handle to read lines from
 *  \return  Number of bytes placed in buff, -1 on EOF, < -1 on error
 */
int config_buffer_line (struct config_buffer *buff, FILE *hand);


/** Read a line from a file into the given config_buffer, growing it if needed
 *  and handling quotes, special characters, multi-line continuation, etc.
 *
 *  \param   buff  config_buffer to read into
 *  \param   hand  Open file handle to read lines from
 *  \param   opt   Bitmask of option bits
 *  \return  Number of bytes placed in buff, or <0 on error
 */
int config_buffer_read (struct config_buffer *buff, FILE *hand, int opt);


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
void config_buffer_discard (struct config_buffer *buff, int size);


/** Write data in the buffer to the destination file handle
 *
 *  The buffer will be adjusted with a call to config_buffer_discard()
 *
 *  \param  buff  Buffer to write from
 *  \param  hand  File handle to write to
 *  
 *  \return Number of bytes written, or <0 for error
 */
int config_buffer_write (struct config_buffer *buff, FILE *hand);


#endif
/* Ends    : config.h */
