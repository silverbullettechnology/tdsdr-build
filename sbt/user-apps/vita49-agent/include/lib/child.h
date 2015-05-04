/** Child process management
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
#ifndef INCLUDE_LIB_CHILD_H
#define INCLUDE_LIB_CHILD_H
#include <stdlib.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#define CHILD_STAT_READY 	0
#define CHILD_STAT_EXIT  	1
#define CHILD_STAT_ERROR 	2
#define CHILD_STAT_NONE  	3

/** Defines a child process.  Functions which could logically operate on multiple children
 *  will take a num argument; if num > 1 then the passed pointer is treated as an array, 
 *  and the function will iterate over num children in turn. */
struct child
{
	int     write;
	int     read;
	pid_t   pid;
	int     ret;
	char  **argv;
};


/** Spawn one or more child processes
 *
 *  This may be called on a single child process with num < 2, or on an array of child
 *  structs with num set to the number of elements.
 *
 *  \param ch  The child process context
 *  \param num The number of children
 *
 *  \return 0 on success, -1 on error
 */
int child_spawn (struct child *ch, int num);


/** Format an argv array for child_spawn() above
 *
 *  Caller must free() the returned pointer when done with it
 *
 *  \param arg0 Path to the child process
 *  \param ...  NULL-terminated variable-length argument list
 *
 *  \return argv pointer on success, NULL on error
 */
char **child_argv (const char *arg0, ...);


/** Check status of one child process
 *
 *  \param ch  The context to use
 *
 *  \note  After CHILD_STAT_EXIT is returned, child_ret will be the child return code if
 *         the child exited normally, or -1 if the child was killed by a signal
 *
 *  \return -1 on error, or one of the CHILD_STAT_* constants above:
 *    CHILD_STAT_READY: child is running normally
 *    CHILD_STAT_EXIT:  child exited; check child_ret for exit code
 *    CHILD_STAT_ERROR: waitpid() erro
 */
int child_check (struct child *ch);


/** Close one or more child process pipes
 *
 *  This may be called on a single child process with num < 2, or on an array of child
 *  structs with num set to the number of elements.
 *
 *  \param ch  The child process context
 *  \param num The number of children
 *
 *  \return 0 on success, -1 on error
 */
int child_close (struct child *ch, int num);


/**  Add the read/write handles of one or more child processes to the fd_set(s) for
 *          a select() call 
 *
 *  This may be called on a single child process with num < 2, or on an array of child
 *  structs with num set to the number of elements.
 *
 *  \note  the caller must FD_ZERO() rfds/wfds before calling child_fd_set(), which will 
 *         FD_SET the child handles, call select(), and return its return value.  The
 *         caller may optionally add other handles to rfds/wfds before/after calling
 *         child_fd_set() to use a single select() which covers other handles.
 *
 *  \note  *mfda will be updated for each file handle added to rfds/wfds, if the handle is
 *         higher, for use by the eventual select() call.  The caller of select() is
 *         responsible for adding the +1 specified for the first parameter of select().
 *
 *  \param  ch    The child process context
 *  \param  num   The number of children
 *  \param  rfds  Pointer to fd_set for read
 *  \param  wfds  Pointer to fd_set for write
 *  \param  mfda  Pointer to maximum handle value accumulator
 *
 *  \return  0 on success, -1 on error
 */
int child_fd_set (struct child *ch, int num, fd_set *rfds, fd_set *wfds, int *mfda);


/**  Call select() on the read/write handles of one or more child processes
 *
 *  This may be called on a single child process with num < 2, or on an array of child
 *  structs with num set to the number of elements.
 *
 *  \note  the caller must FD_ZERO() rfds/wfds before calling child_select(), which will 
 *         FD_SET the child handles, call select(), and return its return value.  The
 *         caller may optionally add other handles to rfds/wfds before calling
 *         child_select() to select() on other handles.  If this is not needed, pass -1
 *         for mfda.
 *
 *  \note  calls child_fd_set() internally to setup rfds/wfds.
 *
 *  \param  ch    The child process context
 *  \param  num   The number of children
 *  \param  rfds  Pointer to fd_set for read
 *  \param  wfds  Pointer to fd_set for write
 *  \param  efds  Pointer to fd_set for exceptions
 *  \param  mfda  Maximum handle value accumulator
 *  \param  tv    Timeout value for select()
 *
 *  \return 0 on success, -1 on error
 */
int child_select (struct child *ch, int num, fd_set *rfds, fd_set *wfds, fd_set *efds,
                  int mfda, struct timeval *tv);


/** Test the file handle(s) of one or more child processes against the rfds/wfds
 *  after a select() call
 *
 *  \param  ch    The child process context
 *  \param  num   The number of children
 *  \param  rfds  A read fd_set previously passed to a select() call
 *  \param  rfds  A write fd_set previously passed to a select() call
 *
 *  \return Number of instance's file handle(s) set in the fd_set, or <0 on failure
 */
int child_fd_is_set (struct child *ch, int num, fd_set *rfds, fd_set *wfds);



/** Test whether the read handle of a child is set in the passed fd_set
 *
 *  \note  calls child_fd_is_set() internally to check rfds
 *
 *  \param ch  The child process context
 *  \param rfds Pointer to read fd_set passed to child_select()
 *
 *  \return >0 if the fd is ready to read, 0 if not, -1 on error
 */
int child_can_read (struct child *ch, fd_set *rfds);


/** Read from the read handle of a child
 *
 *  \param ch  The child process context
 *  \param buf Buffer to read into
 *  \param max Maximum number of bytes to read into buf
 *
 *  \return number of bytes read, which may be 0, or -1 on error
 */
int child_read (struct child *ch, void *buf, int max);


/** Test whether the write handle of a child is set in the passed fd_set
 *
 *  \note  calls child_fd_is_set() internally to check wfds
 *
 *  \param ch  The child process context
 *  \param wfds Pointer to write fd_set passed to child_select()
 *
 *  \return >0 if the fd is ready to write, 0 if not, -1 on error
 */
int child_can_write (struct child *ch, fd_set *wfds);


/** Write to the write handle of a child
 *
 *  \param ch  The child process context
 *  \param buf Buffer to write from
 *  \param max Number of bytes to write
 *
 *  \return number of bytes written, which may be 0, or -1 on error
 */
int child_write (struct child *ch, const void *buf, int max);


#endif /* INCLUDE_LIB_CHILD_H */
/* Ends */
