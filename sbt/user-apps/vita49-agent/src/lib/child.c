/** Child-process management
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
#include <stdarg.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <lib/log.h>
#include <lib/packlist.h>
#include <lib/child.h>


LOG_MODULE_STATIC("lib_child", LOG_LEVEL_WARN);


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
int child_close (struct child *ch, int num)
{
	ENTER("ch %p", ch);
	if ( !ch )
		RETURN_ERRNO_VALUE(EFAULT, "%d", -1);

	do
	{
		if ( ch->write != -1 )
		{
			if ( close(ch->write) < 0 )
				LOG_ERROR("close(%d.write %d) failed: %d:%s\n",
				          ch->pid, ch->write, errno, strerror(errno));
			ch->write = -1;
		}

		if ( ch->read != -1 )
		{
			if ( close(ch->read) < 0 )
				LOG_ERROR("close(%d.read %d) failed: %d:%s\n",
				          ch->pid, ch->read, errno, strerror(errno));
			ch->read = -1;
		}

		ch++;
		num--;
	}
	while ( num > 0 );
	

	RETURN_ERRNO_VALUE(0, "%d", 0);
}


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
int child_spawn (struct child *ch, int num)
{
	ENTER("ch %p", ch);
	if ( !ch )
		RETURN_ERRNO_VALUE(EFAULT, "%d", -1);

	do
	{
		/* Open pipes first */
		int read_pipe[2];
		if ( pipe(read_pipe) )
			RETURN_VALUE("%d", -1);

		int write_pipe[2];
		if ( pipe(write_pipe) )
		{
			close(read_pipe[0]);
			close(read_pipe[1]);
			RETURN_VALUE("%d", -1);
		}

		/* make both pipes nonblocking on the parent side, since we'll be select()ing on
		 * multiple children */
		if ( fcntl(write_pipe[1], F_SETFL, O_WRONLY | O_NONBLOCK) 
		  || fcntl(read_pipe[0],  F_SETFL, O_RDONLY | O_NONBLOCK) )
		{
			close(read_pipe[0]);
			close(read_pipe[1]);
			close(write_pipe[0]);
			close(write_pipe[1]);
			RETURN_VALUE("%d", -1);
		}

		/* pipes set up enough to fork(), record for later */
		ch->write = write_pipe[1];
		ch->read  = read_pipe[0];

		/* attempt to fork record the child pid */
		if ( (ch->pid = fork()) < 0 )
		{
			child_close(ch, 1);
			close(write_pipe[0]);
			close(read_pipe[1]);
			RETURN_VALUE("%d", -1);
		}

		/* Child thread: link to the pipe filedescriptors, do some other daemon-like disconnection
		 * stuff, exec the child process.  */
		if ( ch->pid == 0 )
		{
			/* setup child's stdin for parent to write */
			close  (write_pipe[1]);
			dup2   (write_pipe[0], 0);
			close  (write_pipe[0]);
			setbuf (stdin, NULL);

			/* setup child's stdout for parent to read from */
			close  (read_pipe[0]);
			dup2   (read_pipe[1], 1);
			close  (read_pipe[1]);
			setbuf (stdout, NULL);

			// TODO: setuid/setgid, maybe don't want to setsid() here?
			setsid ();

			execv  (ch->argv[0], ch->argv);
			perror ("execv()");
			exit   (127);
		}

		/* Parent thread: not much else here, if the fork succeeded. */
		close (write_pipe[0]);
		close (read_pipe[1]);

		ch++;
		num--;
	}
	while ( num > 0 );
	
	RETURN_ERRNO_VALUE(0, "%d", 0);
}


/** Format an argv array for child_spawn() above
 *
 *  Caller must free() the returned pointer when done with it
 *
 *  \param arg0 Path to the child process
 *  \param ...  NULL-terminated variable-length argument list
 *
 *  \return argv pointer on success, NULL on error
 */
char **child_argv (const char *arg0, ...)
{
	struct packlist  pl;
	va_list          ap;
	char            *cp;
	char           **ret;

	// reset packlist and size arg0, then the var-args, last the terminating NULL
	memset(&pl, 0, sizeof(struct packlist));
	packlist_size (&pl, arg0, -1);
	va_start(ap, arg0);
	while ( (cp = va_arg(ap, char *)) )
		packlist_size (&pl, cp, -1);
	va_end(ap);
	packlist_size (&pl, NULL, 0);

	// allocation may fail
	errno = 0;
	if ( !(ret = packlist_alloc(&pl)) )
		RETURN_VALUE("%p", NULL);

	// copy arg0, then the var-args, last the terminating NULL
	packlist_data (&pl, arg0, -1);
	va_start(ap, arg0);
	while ( (cp = va_arg(ap, char *)) )
		packlist_data (&pl, cp, -1);
	va_end(ap);
	packlist_data (&pl, NULL, 0);

	RETURN_ERRNO_VALUE(0, "%p", ret);
}


/** Check status of one child process
 *
 *  \param ch  The context to use
 *
 *  \return 0 on success, -1 on error
 */
int child_check (struct child *ch)
{
	ENTER("ch %p", ch);
	if ( !ch )
		RETURN_ERRNO_VALUE(EFAULT, "%d", -1);

	/* I don't like this, but it seems to be necessary under Linux 2.6.3x */
	usleep (0);

	if ( ch->pid < 0 )
		RETURN_ERRNO_VALUE(EINVAL, "%d", CHILD_STAT_NONE);

	int   status;
	pid_t ret = waitpid(ch->pid, &status, WNOHANG);

	/* if the child process is running normally, exit success */
	if ( ret == 0 )
		RETURN_VALUE("CHILD_STAT_READY=%d", CHILD_STAT_READY);
	
	/* if the child process exited or was killed by a signal, return _SPAWN */
	else if ( ret == ch->pid )
	{
		if ( WIFEXITED(status) )
			ch->ret = WEXITSTATUS(status);
		else
			ch->ret = -1;
		RETURN_VALUE("CHILD_STAT_EXIT=%d", CHILD_STAT_EXIT);
	}

	/* waitpid() error ECHILD: the process went away and we missed it, return _SPAWN */
	else if ( errno == ECHILD )
		RETURN_VALUE("CHILD_STAT_EXIT=%d", CHILD_STAT_EXIT);

	/* some other waitpid() error: return _ERROR */
	else
		RETURN_VALUE("CHILD_STAT_ERROR=%d", CHILD_STAT_ERROR);
}


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
int child_fd_set (struct child *ch, int num, fd_set *rfds, fd_set *wfds, int *mfda)
{
	ENTER("ch %p, num %d, rfds %p, wfds %p, *mfda %d", ch, num, rfds, wfds, *mfda);

	if ( !ch || !rfds || !wfds )
		RETURN_ERRNO_VALUE(EFAULT, "%d", -1);

	do
	{
		if ( rfds && ch->read != -1 )
		{
			FD_SET(ch->read, rfds);
			if ( ch->read > *mfda )
				*mfda = ch->read;
		}
		if ( wfds && ch->write != -1 )
		{
			FD_SET(ch->write, wfds);
			if ( ch->write > *mfda )
				*mfda = ch->write;
		}
		ch++;
		num--;
	}
	while ( num > 0 );

	RETURN_ERRNO_VALUE(0, "%d", 0);
}


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
int child_select (struct child *ch, int num,
                  fd_set *rfds, fd_set *wfds, fd_set *efds, int mfda,
                  struct timeval *tv)
{
	ENTER("ch %p, num %d, rfds %p, wfds %p, mfda %d, tv %d.%06d",
	      ch, num, rfds, wfds, mfda, tv ? (int)tv->tv_sec : 0, tv ? (int)tv->tv_usec : 0);
	
	int ret = child_fd_set(ch, num, rfds, wfds, &mfda);
	if ( ret )
		RETURN_VALUE("%d", ret);

	errno = 0;
	ret = select(mfda + 1, rfds, wfds, efds, tv);
	RETURN_VALUE("%d", ret);
}


/** Test whether the read handle of a child is set in the passed fd_set
 *
 *  \param ch  The child process context
 *  \param rfd Pointer to read fd_set passed to child_select()
 *
 *  \return >0 if the fd is ready to read, 0 if not, -1 on error
 */
int child_can_read (struct child *ch, fd_set *rfd)
{
	ENTER("ch %p, rfd %p", ch, rfd);
	if ( !ch )
		RETURN_ERRNO_VALUE(EFAULT, "%d", -1);
	if ( ch->read < 0 )
		RETURN_ERRNO_VALUE(EBADF, "%d", -1);

	int ret = FD_ISSET(ch->read, rfd);
	RETURN_ERRNO_VALUE(0, "%d", ret);
}


/** Read from the read handle of a child
 *
 *  \param ch  The child process context
 *  \param buf Buffer to read into
 *  \param max Maximum number of bytes to read into buf
 *
 *  \return number of bytes read, which may be 0, or -1 on error
 */
int child_read (struct child *ch, void *buf, int max)
{
	ENTER("ch %p, buf %p, max %d", ch, buf, max);
	if ( !ch )
		RETURN_ERRNO_VALUE(EFAULT, "%d", -1);
	if ( ch->read < 0 )
		RETURN_ERRNO_VALUE(EBADF, "%d", -1);

	errno = 0;
	int ret = read(ch->read, buf, max);
	RETURN_VALUE("%d", ret);
}


/** Test whether the write handle of a child is set in the passed fd_set
 *
 *  \param ch  The child process context
 *  \param rfd Pointer to write fd_set passed to child_select()
 *
 *  \return >0 if the fd is ready to write, 0 if not, -1 on error
 */
int child_can_write (struct child *ch, fd_set *wfd)
{
	ENTER("ch %p, wfd %p", ch, wfd);
	if ( !ch )
		RETURN_ERRNO_VALUE(EFAULT, "%d", -1);
	if ( ch->write < 0 )
		RETURN_ERRNO_VALUE(EBADF, "%d", -1);

	int ret = FD_ISSET(ch->write, wfd);
	RETURN_ERRNO_VALUE(0, "%d", ret);
}


/** Write to the write handle of a child
 *
 *  \param ch  The child process context
 *  \param buf Buffer to write from
 *  \param max Number of bytes to write
 *
 *  \return number of bytes written, which may be 0, or -1 on error
 */
int child_write (struct child *ch, const void *buf, int max)
{
	ENTER("ch %p, buf %p, max %d", ch, buf, max);
	if ( !ch )
		RETURN_ERRNO_VALUE(EFAULT, "%d", -1);
	if ( ch->write < 0 )
		RETURN_ERRNO_VALUE(EBADF, "%d", -1);

	errno = 0;
	int ret = write(ch->write, buf, max);
	RETURN_VALUE("%d", ret);
}


#ifdef UNIT_TEST
#include <getopt.h>
int child_id  = -1;
int child_max = -1;
int child_num =  3;

void msg (const char *fmt, ...)
{
	if ( child_id < 0 )
		fprintf (stderr, "parent: ");
	else
		fprintf (stderr, "child%d: ", child_id);

	va_list  ap;
	va_start (ap, fmt);
	vfprintf (stderr, fmt, ap);
	va_end   (ap);
}

int main (int argc, char **argv)
{
	module_level = LOG_LEVEL_TRACE;
	log_dupe(stderr);
	setbuf(stderr, NULL);

	int opt;
	while ( (opt = getopt(argc, argv, "c:x:n:")) != -1 )
		switch ( opt )
		{
			case 'c': child_id  = atoi(optarg); break;
			case 'x': child_max = atoi(optarg); break;
			case 'n': child_num = atoi(optarg); break;
		}
	msg("starting pid %d\n", getpid());

	// child code is simple enough - start a loop where we read a line and echo it back
	// if max is set, exit after that many exchanges
	if ( child_id >= 0 )
	{
		int  cnt = 0;
		char buf[64];

		while ( cnt != child_max && fgets(buf, sizeof(buf), stdin) )
		{
			msg("recv: "); LOG_HEXDUMP(buf, strlen(buf));
			fputs(buf, stdout);
			msg("sent: "); LOG_HEXDUMP(buf, strlen(buf));
			cnt++;
		}
	
		msg("stopping pid %d\n", getpid());
		return child_id;
	}

	// parent code is more involved - allocate child_num children
	struct child *ch = malloc( sizeof(struct child) * child_num );
	if ( !ch )
	{
		perror("malloc");
		return 1;
	}
	memset (ch, 0, sizeof(struct child) * child_num);

	// bitmap of active children
	long active = 0;

	// configure children and spawn
	int i;
	for ( i = 0; i < child_num; i++ )
	{
		char  argc[16], argx[16];
		char **pp;

		snprintf (argc, sizeof(argc), "-c%d", i);
		if ( child_max > 0 )
			snprintf (argx, sizeof(argx), "-x%d", child_max);
		else
			argx[0] = '\0';

		if ( !(ch[i].argv = child_argv(argv[0], argc, argx, NULL)) )
			perror("child_argv");

		pp = ch[i].argv;
		msg("child%d args: {", i);
		while ( *pp )
			fprintf (stderr, " '%s',", *pp++);
		fprintf (stderr, " NULL }\n");

		active |= (1 << i);
	}
	if ( child_spawn(ch, child_num) )
		perror("child_spawn");
	msg("spawned children, start loop\n");

	// params for select()
	fd_set rfd, wfd;
	struct timeval tv_cur, tv_set = { .tv_sec = 0, .tv_usec = 125000 };

	// interact with active children
	int clock = 0;
	while ( active && clock < 100 )
	{
		clock++;
		msg("clock %d\n", clock);

		// check for child exit first; also cleans up zombies since it calls waitpid()
		for ( i = 0; i < child_num; i++ )
			switch ( child_check(ch + i) )
			{
				case CHILD_STAT_READY:
					msg("child%d running\n", i);
					break;

				case CHILD_STAT_EXIT:
					if ( ch[i].ret < 0 )
						msg("child%d killed\n", i);
					else
						msg("child%d exited with %d\n", i, ch[i].ret);
					child_close(ch + i, 1);
					active &= ~(1 << i);
					break;

				case CHILD_STAT_ERROR:
					msg("child%d waitpid() failed: %s\n", i, strerror(errno));
					return 1;
			}

		// select() on active children next
		FD_ZERO (&rfd);
		FD_ZERO (&wfd);
		tv_cur = tv_set;
		switch ( child_select(ch, child_num, &rfd, &wfd, NULL, -1, &tv_cur) )
		{
			case -1:
				msg("child_select() failed: %s\n", strerror(errno));
				return 1;

			case 0:
				msg("idle\n");
				break;

			default:
				msg("activity\n");
				for ( i = 0; i < child_num; i++ )
					if ( active & (1 << i) )
					{
						char buf[64];
						int  ret;

						if ( child_can_read(ch + i, &rfd) )
						{
							memset (buf, 0, sizeof(buf));
							if ( (ret = child_read(ch + i, buf, sizeof(buf) - 1)) < 0 )
								msg("child%d read failed: %s\n", i, strerror(errno));
							else
							{
								msg("child%d read: %d bytes: ", i, ret);
								LOG_HEXDUMP(buf, ret);
							}
						}

						if ( child_can_write(ch + i, &wfd) )
						{
							ret = snprintf (buf, sizeof(buf), "clock %d\n", clock);
							if ( (ret = child_write(ch + i, buf, ret)) < 0 )
								msg("child%d write failed: %s\n", i, strerror(errno));
							else
							{
								msg("child%d write: %d bytes: ", i, ret);
								LOG_HEXDUMP(buf, ret);
							}
						}
					}
				break;
		}
	}
	msg("children stopped, stopping\n");
	
	// cleanup
	child_close(ch, child_num);
	for ( i = 0; i < child_num; i++ )
		free (ch[i].argv);
	free(ch);

	msg("stopping pid %d\n", getpid());
	return 0;
}
#endif


/* Ends    : child.c */
