/** Debug log code
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
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>


/* include our logging first here to get the various things it defines, then undefine the
 * macros it defines for the application to use, which conflict with syslog.h */
#include <sbt_common/log.h> 
#undef LOG_INFO             
#undef LOG_DEBUG            

/* include the actual syslog.h after undef'ing conflicting macros */
#include <syslog.h>

#include <sbt_common/util.h>
#include <sbt_common/clocks.h>
#include <sbt_common/descript.h>
#include <sbt_common/packlist.h>


LOG_MODULE_STATIC("log_core", LOG_LEVEL_DEBUG);


static struct descript log_level_descript[] =
{
	{ "focus", LOG_LEVEL_FOCUS },
	{ "error", LOG_LEVEL_ERROR },
	{ "warn",  LOG_LEVEL_WARN  },
	{ "info",  LOG_LEVEL_INFO  },
	{ "debug", LOG_LEVEL_DEBUG },
	{ "trace", LOG_LEVEL_TRACE },
	{ NULL }
};

/** Return label/level lookups
 */
const char *log_label_for_level (int level)
{
	return descript_label_for_value(log_level_descript, level);
}
int log_level_for_label (const char *label)
{
	return descript_value_for_label(log_level_descript, label);
}


#ifndef NDEBUG
static FILE *log_own_handle = NULL;
static FILE *log_dup_handle = NULL;
static int   log_syslog = 0;
static int   log_print_trace = 0;
#endif


#define LOG_FUNC_SIZE 8
static const char *log_func_list[LOG_FUNC_SIZE];
static signed      log_func_curs = 0;


/** Open a logfile with fopen() and use it for logging.
 *
 *  The file WILL be closed by log_close(), it is separate from the handle opened by log_dupe()
 *
 *  \param name  Filename to open
 *  \param mode  Mode string
 *
 *  \return 0 on success, <0 on error
 */
int log_fopen (const char *name, const char *mode)
{
#ifdef NDEBUG
	errno = ENOSYS;
	return -1;
#else
	if ( log_own_handle )
		fclose (log_own_handle);

	if ( (log_own_handle = fopen(name, mode)) )
	{
		setbuf(log_own_handle, NULL);

		// make it close-on-exec
		int flags = fcntl(fileno(log_own_handle), F_GETFD, 0);
		if ( flags < -1 )
			fcntl(fileno(log_own_handle), F_SETFD, flags | FD_CLOEXEC);
	}

	return log_own_handle == NULL;
#endif
}


/** Set the logging file to one already open, or set it NULL 
 *
 *  The handle will NOT be closed by log_close(), it is separate from the handle opened by log_fopen()
 *  The handle must stay open until log_close(), or the caller must set it NULL with another call 
 *
 *  \param handle  FILE pointer already opened by caller
 */
void log_dupe (FILE *handle)
{
#ifdef NDEBUG
	errno = ENOSYS;
#else
	log_dup_handle = handle;
	errno = 0;
#endif
}


/** Open a syslog connection for logging messages
 *
 *  The handle will NOT be closed by log_close(), it is separate from the handle opened by log_fopen()
 *  The handle must stay open until log_close(), or the caller must set it NULL with another call 
 *
 *  \param ident     Identity for syslog messages, traditionally basename(argv[0])
 *  \param option    Bitmask of options; see syslog(3) for details
 *  \param facility  Facility argument; see syslog(3) for details
 */
void log_openlog (const char *ident, int option, int facility)
{
#ifdef NDEBUG
	errno = ENOSYS;
#else
	openlog (ident, option, facility);
	log_syslog = 1;
#endif
}


/** Set the trace flag
 *
 *  After a call with trace set nonzero, the LOG_*() calls below will log the calling filename and line number
 *  in each message for debugging.  This has no effect on the LOG_CALL_TRACE() macro, which always logs this.
 *
 *  \param trace  nonzero to enable file name and line numbers in debug messages.
 */
void log_trace (int trace)
{
#ifdef NDEBUG
	errno = ENOSYS;
#else
	log_print_trace = trace;
	errno = 0;
#endif
}


/** Close all logging output handles
 */
void log_close (void)
{
#ifdef NDEBUG
	errno = ENOSYS;
#else
	if ( log_own_handle )
		fclose (log_own_handle);
	log_own_handle = NULL;
	log_dup_handle = NULL;
	if ( log_syslog )
		closelog();
	log_syslog = 0;
	errno = 0;
#endif
}


#ifndef NDEBUG

/** Print a message to the debug file handle, if open and message_level <= module_level.  
 *
 *  \param message_level
 *  \param module_level
 *  \param file
 *  \param line
 *  \param format
 *  \param ...
 */
void log_printf (int message_level, int module_level, const char *file, int line, const char *format, ...)
{
	int err = errno;

	static char tb[64] = "";

	if ( message_level > module_level )
		return;

#ifdef LOG_TIMESTAMP
	int t1 = clocks_to_ms(get_clocks());
	snprintf(tb, sizeof(tb), "[%5u.%03u] ", (t1 / 1000) % 100000, t1 % 1000);
#endif

	va_list ap;
	if ( log_own_handle )
	{
		if ( log_print_trace )
			fprintf (log_own_handle, "%s%s[%d]: ", tb, file, line);

		va_start (ap, format);
		vfprintf (log_own_handle, format, ap);
		va_end   (ap);
	}

	if ( log_dup_handle )
	{
		if ( log_print_trace )
			fprintf (log_dup_handle, "%s%s[%d]: ", tb, file, line);

		va_start (ap, format);
		vfprintf (log_dup_handle, format, ap);
		va_end   (ap);
	}

	if ( log_syslog )
	{
		int priority = LOG_INFO;
		switch ( message_level )
		{
			case LOG_LEVEL_FOCUS:  priority = LOG_CRIT;     break;
			case LOG_LEVEL_ERROR:  priority = LOG_ERR;      break;
			case LOG_LEVEL_WARN:   priority = LOG_WARNING;  break;
			case LOG_LEVEL_INFO:   priority = LOG_INFO;     break;
			case LOG_LEVEL_DEBUG:  priority = LOG_DEBUG;    break;
			case LOG_LEVEL_TRACE:  priority = LOG_DEBUG;    break;
		}
		va_start (ap, format);
		vsyslog  (priority, format, ap);
		va_end   (ap);
	}

	errno = err;
}


/** Hexdump one line of 1-16 bytes
 *
 *  \param ptr Pointer to line's worth of data to dump
 *  \param ptr Pointer to first line's data
 *  \param len Length of line's worth of data to dump
 */
void log_hexdump_line (const unsigned char *ptr, const unsigned char *org, int len)
{
	char buff[80];
	char *p = buff;
	p += sprintf (p, "%04x: ", (unsigned)(ptr - org));

	int i;
	for ( i = 0; i < len; i++ )
		p += sprintf (p, "%02x ", ptr[i]);

	for ( ; i < 16; i++ )
	{
		strcpy (p, "   ");
		p += 3;
	}

	for ( i = 0; i < len; i++ )
		*p++ = isprint(ptr[i]) ? ptr[i] : '.';
	*p = '\0';

	if ( log_own_handle ) fprintf (log_own_handle, "%s\n", buff);
	if ( log_dup_handle ) fprintf (log_dup_handle, "%s\n", buff);
	if ( log_syslog )     syslog  (LOG_DEBUG,      "%s",   buff); 
}


/** Hexdump a buffer
 *
 *  \parma level Module debug level via macro
 *  \param file  File called from, from __FILE__ via macro
 *  \param line  Line called from, from __LINE__ via macro
 *  \param var   Name of variable, via macro
 *  \param buf   Pointer to buffer to hexdump
 *  \param len   Length of buffer to hexdump
 */
void log_hexdump (int module_level, const char *file, int line, const char *var,
                  const void *buf, int len)
{
	int err = errno;

	if ( LOG_LEVEL_DEBUG > module_level )
		return;

	if ( var && *var )
		log_printf (LOG_LEVEL_DEBUG, module_level, file, line,
		            "%d bytes from %p('%s'):\n", len, buf, var);
	else
		log_printf (LOG_LEVEL_DEBUG, module_level, file, line,
		            "%d bytes from %p(\?\?\?):\n", len, buf);

	const unsigned char *ptr = (const unsigned char *)buf;
	unsigned char        dup[16];
	int                  sup = 0;

	dup[0] = ~ *ptr;
	while ( len >= 16 )
	{
		if ( memcmp(dup, ptr, 16) )
		{
			if ( sup )
			{
				log_printf(LOG_LEVEL_DEBUG, module_level, file, line,
				           "* (%d duplicates)\n", sup);
				sup = 0;
			}
			log_hexdump_line(ptr, (const unsigned char *)buf, 16);
			memcpy(dup, ptr, 16);
		}
		else
			sup++;

		ptr += 16;
		len -= 16;
	}
	if ( sup )
		log_printf(LOG_LEVEL_DEBUG, module_level, file, line,
		           "* (%d duplicates)\n", sup);

	if ( len )
		log_hexdump_line(ptr, (const unsigned char *)buf, len);

	errno = err;
}

#endif // !NDEBUG


enum
{
	LOG_CONFIG_LEVEL,
	LOG_CONFIG_APPEND,
	LOG_CONFIG_OVERWRITE,
	LOG_CONFIG_SYSLOG,
	LOG_CONFIG_TRACE,
	LOG_CONFIG_DUPE,
	LOG_CONFIG_DUPE_STDERR,
	LOG_CONFIG_DUPE_STDOUT,
};

static struct descript verb_lut[] =
{
	{ "level",  LOG_CONFIG_LEVEL     },
	{ "append", LOG_CONFIG_APPEND    },
	{ "file",   LOG_CONFIG_APPEND    },
	{ "trunc",  LOG_CONFIG_OVERWRITE },
	{ "syslog", LOG_CONFIG_SYSLOG    },
	{ "trace",  LOG_CONFIG_TRACE     },
	{ "dupe",   LOG_CONFIG_DUPE      },
	{ NULL }
};

static struct descript dupe_lut[] =
{
	{ "stderr", LOG_CONFIG_DUPE_STDERR },
	{ "stdout", LOG_CONFIG_DUPE_STDOUT },
	{ NULL }
};

static struct descript option_lut[] =
{
	{ "cons",   LOG_CONS   },
	{ "ndelay", LOG_NDELAY },
	{ "nowait", LOG_NOWAIT },
	{ "odelay", LOG_ODELAY },
	{ "perror", LOG_PERROR },
	{ "pid",    LOG_PID    },
	{ NULL }
};

static struct descript facility_lut[] =
{
	{ "auth",     LOG_AUTH     },
	{ "authpriv", LOG_AUTHPRIV },
	{ "cron",     LOG_CRON     },
	{ "daemon",   LOG_DAEMON   },
	{ "ftp",      LOG_FTP      },
	{ "local0",   LOG_LOCAL0   },
	{ "local1",   LOG_LOCAL1   },
	{ "local2",   LOG_LOCAL2   },
	{ "local3",   LOG_LOCAL3   },
	{ "local4",   LOG_LOCAL4   },
	{ "local5",   LOG_LOCAL5   },
	{ "local6",   LOG_LOCAL6   },
	{ "local7",   LOG_LOCAL7   },
	{ "lpr",      LOG_LPR      },
	{ "mail",     LOG_MAIL     },
	{ "news",     LOG_NEWS     },
	{ "syslog",   LOG_SYSLOG   },
	{ "user",     LOG_USER     },
	{ "uucp",     LOG_UUCP     },
	{ NULL }
};

/** Configure logging from config file
 *
 *  This is provided as a convenience to calling programs and is intended to be used as a
 *  callback with libconfig.  The section is ignored, so it's the caller's responsibility
 *  to match the appropriate section.  The data argument is also ignored.
 */
int log_config (const char *section, const char *tag, const char *val,
                const char *file, int line, void *data)
{
	ENTER("section %s, tag %s, val %s, file %s, line %d, data %p",
	      section, tag, val, file, line, data);

	if ( !tag || !val )
		RETURN_ERRNO_VALUE(0, "%d", 0);

	int verb = descript_value_for_label(verb_lut, tag);
	switch ( verb )
	{
		case LOG_CONFIG_APPEND:
			if ( log_fopen(val, "a") )
				RETURN_VALUE("%d", -1);
			RETURN_ERRNO_VALUE(0, "%d", 0);

		case LOG_CONFIG_OVERWRITE:
			if ( log_fopen(val, "w") )
				RETURN_VALUE("%d", -1);
			RETURN_ERRNO_VALUE(0, "%d", 0);

		case LOG_CONFIG_SYSLOG:
			if ( !log_syslog )
			{
				char *buf = strdup(val);
				if ( !buf )
					RETURN_VALUE("%d", -1);

				int   opt = LOG_NDELAY|LOG_PID;
				int   fac = LOG_DAEMON;
				char *arg = NULL;
				char *tok = strtok(buf, ",");
				while ( tok )
				{
					int idx;
					if ( (idx = descript_value_for_label(option_lut, tok)) > -1 )
					{
						if ( opt == (LOG_NDELAY|LOG_PID) )
							opt = 0;
						opt |= idx;
					}
					else if ( (idx = descript_value_for_label(facility_lut, tok)) > -1 )
						fac = idx;
					else if ( !arg )
						arg = tok;

					tok = strtok(NULL, ",");
				}
				free(buf);
				log_openlog(arg, opt, fac);
			}
			RETURN_ERRNO_VALUE(0, "%d", 0);

		case LOG_CONFIG_TRACE:
			log_print_trace = binval(val);
			RETURN_ERRNO_VALUE(0, "%d", 0);

		case LOG_CONFIG_DUPE:
			switch ( descript_value_for_label(dupe_lut, val) )
			{
				case LOG_CONFIG_DUPE_STDERR:
					log_dupe(stderr);
					RETURN_ERRNO_VALUE(0, "%d", 0);

				case LOG_CONFIG_DUPE_STDOUT:
					log_dupe(stdout);
					RETURN_ERRNO_VALUE(0, "%d", 0);
			}
			RETURN_ERRNO_VALUE(EINVAL, "%d", -1);
	}

	// by this point the tag is either "level" or a module name, and val is either a
	// symbolic or numeric level.
	int level = descript_value_for_label(log_level_descript, val);
	if ( level < 0 )
	{
		errno = 0;
		level = strtoul(val, NULL, 0);
		if ( errno )
			RETURN_VALUE("%d", -1);
	}

	if ( verb == LOG_CONFIG_LEVEL )
		log_set_global_level(level);
	else 
		log_set_module_level(tag, level);

	RETURN_ERRNO_VALUE(0, "%d", 0);
}


/** Linker-generated symbols for modules inside the library */
extern struct log_module_map_t __start_log_module_map;
extern struct log_module_map_t  __stop_log_module_map;

static struct log_module_map_t *log_module_map_app_start = NULL;
static struct log_module_map_t *log_module_map_app_stop  = NULL;

void _log_init_module_list (struct log_module_map_t *start,
                            struct log_module_map_t *stop)
{
	log_module_map_app_start = start;
	log_module_map_app_stop  = stop;
}


/** Set all modules' verbosity level
 *
 *  \note This is implemented with a linker-generated list.
 *
 *  \param level  LOG_LEVEL_* value
 */
int _log_set_global_level (int level, const char *file, int line)
{
	ENTER("level %d, file %s, line %d", level, file, line);

	if ( level < 0 || level > LOG_LEVEL_TRACE )
	{
		log_printf(LOG_LEVEL_ERROR, module_level, file, line,
		           "Debug level %d is invalid, must be 0..%d\n", level, LOG_LEVEL_TRACE);
		RETURN_ERRNO_VALUE(EINVAL, "%d", -1);
	}

	struct log_module_map_t *map;

	if ( log_module_map_app_start && log_module_map_app_stop )
		for ( map = log_module_map_app_start; map != log_module_map_app_stop; map++ )
			*(map->var) = level;

	for ( map = &__start_log_module_map; map != &__stop_log_module_map; map++ )
		*(map->var) = level;

	log_printf(LOG_LEVEL_INFO, module_level, file, line,
	           "Global debug level set to %s\n",
	           log_label_for_level(level));
	RETURN_ERRNO_VALUE(0, "%d", 0);
}

/** Set a module's verbosity level by name
 *
 *  \note This is implemented with a linker-generated list.
 *
 *  \param module symbolic name of the module
 *  \param level  LOG_LEVEL_* value
 */
int _log_set_module_level (const char *module, int level, const char *file, int line)
{
	ENTER("module %s, level %d, file %s, line %d", module, level, file, line);

	if ( level < 0 || level > LOG_LEVEL_TRACE )
	{
		log_printf(LOG_LEVEL_ERROR, module_level, file, line,
		           "Debug level %d is invalid, must be 0..%d\n", level, LOG_LEVEL_TRACE);
		RETURN_ERRNO_VALUE(EINVAL, "%d", -1);
	}

	struct log_module_map_t *map;
	int                      cnt = 0;

	if ( log_module_map_app_start && log_module_map_app_stop )
		for ( map = log_module_map_app_start; map != log_module_map_app_stop; map++ )
			if ( strmatch(map->mod, module) )
			{
				*(map->var) = level;
				cnt++;
				log_printf(LOG_LEVEL_INFO, module_level, file, line,
				           "Module %s debug level set to %s\n",
				           map->mod, log_label_for_level(level));
			}

	for ( map = &__start_log_module_map; map != &__stop_log_module_map; map++ )
		if ( strmatch(map->mod, module) )
		{
			*(map->var) = level;
			cnt++;
			log_printf(LOG_LEVEL_INFO, module_level, file, line,
			           "Module %s debug level set to %s\n",
			           map->mod, log_label_for_level(level));
		}

	if ( !cnt )
	{
		log_printf(LOG_LEVEL_ERROR, module_level, file, line,
		           "Module name %s not matched\n", module);
		RETURN_ERRNO_VALUE(ENOENT, "%d", -1);
	}
	RETURN_ERRNO_VALUE(0, "%d", 0);
}

/** Get a packlist of all modules compiled in
 *
 *  \note Caller must free() the result
 *
 * 	\return A packlist of module names, or NULL on error
 */
char **log_get_module_list (void)
{
	struct log_module_map_t  *map;
	struct packlist           pl;
	char                    **ret;

	memset(&pl, 0, sizeof(struct packlist));
	if ( log_module_map_app_start && log_module_map_app_stop )
		for ( map = log_module_map_app_start; map != log_module_map_app_stop; map++ )
			packlist_size (&pl, map->mod, -1);
	for ( map = &__start_log_module_map; map != &__stop_log_module_map; map++ )
		packlist_size (&pl, map->mod, -1);
	packlist_size (&pl, NULL, 0);

	errno = 0;
	if ( !(ret = packlist_alloc(&pl)) )
		RETURN_VALUE("%p", NULL);

	if ( log_module_map_app_start && log_module_map_app_stop )
		for ( map = log_module_map_app_start; map != log_module_map_app_stop; map++ )
			packlist_data (&pl, map->mod, -1);
	for ( map = &__start_log_module_map; map != &__stop_log_module_map; map++ )
		packlist_data (&pl, map->mod, -1);
	packlist_data (&pl, NULL, 0);

	return ret;
}


void log_call_enter (const char *func)
{
	log_func_curs++;
	if ( log_func_curs >= LOG_FUNC_SIZE )
		log_func_curs = 0;

	log_func_list[log_func_curs] = func;
}

void log_call_leave (const char *func)
{
	if ( log_func_list[log_func_curs] != func )
		return;

	log_func_list[log_func_curs] = NULL;
	log_func_curs--;
	if ( log_func_curs < 0 )
		log_func_curs = LOG_FUNC_SIZE - 1;
}


/** Get a list of the last few functions which called ENTER() without calling one
 *  of the RETURN* macros.  
 *
 *  The most recent call ENTER()ed will be put in buff[0], the next most recent in
 *  buff[1], and so on towards main().
 *
 *  \note This list is voluntary, using the ENTER() and RETURN*() macros rather than stack
 *        frames or other architecture-specific means.  Coverage is thus not guaranteed
 *        and the data is presented in hopes it will be useful.
 *
 *  \note This fills in the caller's buffer for use in signal handlers where the heap may
 *        be corrupt. 
 *
 *  \param  buff  Pointer to an array of char pointers
 *  \param  size  Number of pointers in array
 *
 *  \return Number of non-NULL function names available (may be >size)
 */
int log_call_trace (char **buff, size_t size)
{
	signed  i = log_func_curs;
	size_t  j = 0;
	int     c = 0;

	do
	{
		if ( log_func_list[i] )
		{
			c++;
			if ( j < size )
				buff[j++] = (char *)log_func_list[i];
		}
		i--;
		if ( i < 0 )
			i = LOG_FUNC_SIZE - 1;
	}
	while ( i != log_func_curs );

	return c;
}


/* Ends    : lib/log.c */
