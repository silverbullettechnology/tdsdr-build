/** Debug log macros and definitions
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
#ifndef INCLUDE_LIB_LOG_H
#define INCLUDE_LIB_LOG_H
#include <string.h>
#include <stdio.h>
#include <errno.h>


#define LOG_LEVEL_FOCUS  0
#define LOG_LEVEL_ERROR  1
#define LOG_LEVEL_WARN   2
#define LOG_LEVEL_INFO   3
#define LOG_LEVEL_DEBUG  4
#define LOG_LEVEL_TRACE  5

#define LOG_TIMESTAMP

/** Open a logfile with fopen() and use it for logging.
 *
 *  The file WILL be closed by log_close(), it is separate from the handle opened by
 *  log_dupe()
 *
 *  \param name  Filename to open
 *  \param mode  Mode string
 *
 *  \return 0 on success, <0 on error
 */
int log_fopen (const char *name, const char *mode);


/** Set the logging file to one already open, or set it NULL 
 *
 *  The handle will NOT be closed by log_close(), it is separate from the handle opened by
 *  log_fopen().  The handle must stay open until log_close(), or the caller must set it
 *  NULL with another call.
 *
 *  \param handle  FILE pointer already opened by caller
 */
void log_dupe (FILE *handle);


/** Open a syslog connection for logging messages
 *
 *  The handle will NOT be closed by log_close(), it is separate from the handle opened by
 *  log_fopen().  The handle must stay open until log_close(), or the caller must set it
 *  NULL with another call .
 *
 *  \param ident     Identity for syslog messages, traditionally basename(argv[0])
 *  \param option    Bitmask of options; see syslog(3) for details
 *  \param facility  Facility argument; see syslog(3) for details
 */
void log_openlog (const char *ident, int option, int facility);


/** Set the trace flag
 *
 *  After a call with trace set nonzero, the LOG_*() calls below will log the calling
 *  filename and line number in each message for debugging.  This has no effect on the
 *  LOG_CALL_TRACE() macro, which always logs this.
 *
 *  \param trace  nonzero to enable file name and line numbers in debug messages.
 */
void log_trace (int trace);


/** Close all logging output handles
 */
void log_close (void);


/** Configure logging from config file
 *
 *  This is provided as a convenience to calling programs and is intended to be used as a
 *  callback with libconfig.  The section is ignored, so it's the caller's responsibility
 *  to match the appropriate section.  The data argument is also ignored.
 */
int log_config (const char *section, const char *tag, const char *val,
                const char *file, int line, void *data);


/** The below are macros to pass caller filename and line number, and to ensure they don't
 *  bulk up the code when it's compiled without debugging.  Please use them rather than
 *  log_hexdump() and log_printf(), which may change without notice.
 */
#ifndef NDEBUG

/** Format and log a message unconditionally
 *
 *  This is most useful for printing messages during debugging regardless of a module's
 *  debug level.  LOG_FOCUS() calls should be removed from production code before commit,
 *  and not be found in releases.  LOG_FOCUS() should be used in unit tests to ensure
 *  correct merging of unit-test and debug/error messages.
 */
#define LOG_FOCUS(fmt, ...) \
	do { \
		log_printf(LOG_LEVEL_FOCUS, 0, __FILE__, __LINE__, fmt, ##__VA_ARGS__); \
	} while (0)


/** Format and log a message with a flexible level
 *
 *  This is useful for code which may dump a structure for debug when things are working,
 *  or as part of an error message when not.
 */
#define LOG_MESSAGE(lvl, fmt, ...) \
	do { \
		log_printf(lvl, module_level, __FILE__, __LINE__, fmt, ##__VA_ARGS__); \
	} while (0)


/** Format and log an error message, if the module's local log level is at least
 *  LOG_LEVEL_ERROR
 */
#define LOG_ERROR(fmt, ...) \
	do { \
		log_printf(LOG_LEVEL_ERROR, module_level, __FILE__, __LINE__, fmt, ##__VA_ARGS__); \
	} while (0)


/** Format and log n warning message, if the module's local log level is at least
 *  LOG_LEVEL_WARN
 */
#define LOG_WARN(fmt, ...) \
	do { \
		log_printf(LOG_LEVEL_WARN, module_level, __FILE__, __LINE__, fmt, ##__VA_ARGS__); \
	} while (0)


/** Format and log an informational message, if the module's local log level is at
 *  least LOG_LEVEL_INFO
 */
#define LOG_INFO(fmt, ...) \
	do { \
		log_printf(LOG_LEVEL_INFO, module_level, __FILE__, __LINE__, fmt, ##__VA_ARGS__); \
	} while (0)


/** Format and log a debug message, if the module's local log level is at least
 *  LOG_LEVEL_DEBUG
 *
 *  LOG_DEBUG() should be preferred for debug messages which are not terribly noisy and
 *  are useful while running the software; for messages noisy enough to be easier read in
 *  a logfile, use LOG_TRACE()
 */
#define LOG_DEBUG(fmt, ...) \
	do { \
		log_printf(LOG_LEVEL_DEBUG, module_level, __FILE__, __LINE__, fmt, ##__VA_ARGS__); \
	} while (0)


/** Format and log a debug message, if the module's local log level is at least
 *  LOG_LEVEL_TRACE
 *
 *  LOG_TRACE() should be reserved for debug messages which are so noisy they make debug
 *  logging useless while running the software; for messages which are rare enough to read
 *  in realtime, use LOG_DEBUG()
 */
#define LOG_TRACE(fmt, ...) \
	do { \
		log_printf(LOG_LEVEL_TRACE, module_level, __FILE__, __LINE__, fmt, ##__VA_ARGS__); \
	} while (0)


/** Format a binary buffer in canonical hex format, 16 bytes per line, and output
 *  via the log 
 *
 *  Hexdumps are always LOG_LEVEL_DEBUG and use the module_level normally
 */
#define LOG_HEXDUMP(buf, len) \
	do { \
		log_hexdump(module_level, __FILE__, __LINE__, #buf, buf, len); \
	} while (0)


#ifdef LOG_CALL
#define LOG_CALL_TRACE(fmt, ...)    LOG_TRACE(fmt, ##__VA_ARGS__) 
#define LOG_CALL_ENTER(func)        log_call_enter(func)
#define LOG_CALL_LEAVE(func)        log_call_leave(func)
void log_call_enter (const char *func);
void log_call_leave (const char *func);
#else
#define LOG_CALL_TRACE(fmt, ...)    do { } while (0)
#define LOG_CALL_ENTER(func)        do { } while (0)
#define LOG_CALL_LEAVE(func)        do { } while (0)
#endif

void log_hexdump_line (const unsigned char *ptr, const unsigned char *org, int len);
void log_hexdump (int module_level, const char *file, int line, const char *var,
                  const void *buf, int len);
void log_printf (int message_level, int module_level, const char *file, int line,
                 const char *format, ...) __attribute__ ((format (printf, 5, 6)));

#else

#define LOG_FOCUS(fmt, ...)       do { } while (0)
#define LOG_ERROR(fmt, ...)       do { } while (0)
#define LOG_INFO(fmt, ...)        do { } while (0)
#define LOG_WARN(fmt, ...)        do { } while (0)
#define LOG_DEBUG(fmt, ...)       do { } while (0)
#define LOG_TRACE(fmt, ...)       do { } while (0)
#define LOG_PRINTF(fmt, ...)      do { } while (0)
#define LOG_HEXDUMP(buf, len)     do { } while (0)
#define LOG_CALL_TRACE(fmt, ...)  do { } while (0)

#endif


/** Every function should start with a ENTER() and every function return should use the
 *  appropriate RETURN*().  These macros provide convenient ways to set errno during a
 *  return.
 *
 *  If LOG_CALL is defined and a module's debug level is LOG_LEVEL_TRACE, will print for
 *  each function call:
 *  - function name and argument summary
 *  - errno value and source at return and return value if applicable
 *  - source file name and line number if log_set_trace() has been set appropriately
 */
#define ENTER(fmt, ...) \
	do { \
		LOG_CALL_ENTER(__func__); \
		LOG_CALL_TRACE("enter %s[%d](" fmt ")\n", __func__, __LINE__, ##__VA_ARGS__); \
	} while (0)

#define RETURN_VALUE(fmt, val)\
	do { \
		LOG_CALL_LEAVE(__func__); \
		LOG_CALL_TRACE("leave %s[%d](), return(" fmt "), errno is %d:%s\n", \
		               __func__, __LINE__, (val), (errno), strerror(errno)); \
		return (val); \
	} while (0)

#define RETURN \
	do { \
		LOG_CALL_LEAVE(__func__); \
		LOG_CALL_TRACE("leave %s[%d](), void, errno is %d:%s\n", \
		               __func__, __LINE__, (errno), strerror(errno)); \
		return; \
	} while (0)

#define RETURN_ERRNO_VALUE(err,fmt,val) \
	do { \
		LOG_CALL_LEAVE(__func__); \
		LOG_CALL_TRACE("leave %s[%d](), return(" fmt "), set errno %d:%s\n", \
		               __func__, __LINE__, (val), (err), strerror(err)); \
		errno = (err); \
		return (val); \
	} while (0)

#define RETURN_ERRNO(err) \
	do { \
		LOG_CALL_LEAVE(__func__); \
		LOG_CALL_TRACE("leave %s[%d](), void, set errno %d:%s\n", \
		               __func__, __LINE__, (err), strerror(err)); \
		errno = (err); \
		return; \
	} while (0)


/** Simple modules (with one source file) should use LOG_MODULE_STATIC() to define a log-
 *  level variable that's static and local to that file file.  For example:
 *    LOG_MODULE_STATIC("simple_module", LOG_LEVEL_WARN);
 *
 *  Modules spanning multiple files may treat those files as separate submodules, OR use
 *  LOG_MODULE_EXPORT() in one file to define a log-level variable and a #define to map
 *  the reserved name "module_level" onto the shared name.  For example:
 *    LOG_MODULE_EXPORT("complex_module", complex_module_level, LOG_LEVEL_WARN);
 *    #define module_level complex_module_level
 *
 *  The rest of the files in the module will need to extern the shared variable, either
 *  directly or via a module-private header, and reproduce the #define 
 *    LOG_MODULE_IMPORT(complex_module_level);
 *    #define module_level complex_module_level
 *
 * Either approach will add an entry in a linker-generated list which will be used by
 * log_set_module_level() and log_set_global_level().  There are no special rquirements
 * for the module names; they will be visible to users.
 */

#define LOG_MODULE_STATIC(mod,def) \
	static int module_level = def; \
	static struct log_module_map_t _log_module_map \
	     __attribute__((unused,used,section("log_module_map"),aligned(sizeof(void *)))) \
	     = { mod, &module_level };

#define LOG_MODULE_EXPORT(mod,var,def) \
	int var = def; \
	static struct log_module_map_t _log_module_map \
	     __attribute__((unused,used,section("log_module_map"),aligned(sizeof(void *)))) \
	     = { mod, &var };

#define LOG_MODULE_IMPORT(var) extern int var;

struct log_module_map_t
{
	const char *mod;
	int        *var;
};

void _log_init_module_list (struct log_module_map_t *start, struct log_module_map_t *stop);


/** Initialize library with calling program's module list
 *
 *  \note The calling program must call this function to expose modules in the calling
 *        program to the library, before calling the list-related functions below.
 *
 *  \return A packlist of module names, or NULL on error
 */
static inline void log_init_module_list (void)
{
	/** Linker-generated symbols for modules inside the application */
	extern struct log_module_map_t __start_log_module_map;
	extern struct log_module_map_t  __stop_log_module_map;

	_log_init_module_list(&__start_log_module_map, &__stop_log_module_map);
}


/** Set all modules' verbosity level
 *
 *  \note The calling program must call log_init_module_list() before using this function,
 *        to expose modules in the calling program to the library.
 *
 *  \param level  LOG_LEVEL_* value
 */
#define log_set_global_level(level) _log_set_global_level(level, __FILE__, __LINE__)
int _log_set_global_level (int level, const char *file, int line);

/** Set a module's verbosity level by name
 *
 *  \note The calling program must call log_init_module_list() before using this function,
 *        to expose modules in the calling program to the library.
 *
 *  \param module symbolic name of the module
 *  \param level  LOG_LEVEL_* value
 */
#define log_set_module_level(module,level) _log_set_module_level(module, level, __FILE__, __LINE__)
int _log_set_module_level (const char *module, int level, const char *file, int line);

/** Get a packlist of all modules compiled in
 *
 *  \note Caller must free() the result
 *  \note The calling program must call log_init_module_list() before using this function,
 *        to expose modules in the calling program to the library.
 *
 *  \return A packlist of module names, or NULL on error
 */
char **log_get_module_list (void);


/** Return label/level lookups (log_level_descript[] is now static) */
const char *log_label_for_level (int level);
int log_level_for_label (const char *label);


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
int log_call_trace (char **buff, size_t size);


#endif /* INCLUDE_LIB_LOG_H */
/* Ends */
