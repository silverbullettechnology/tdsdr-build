/** Non-blocking echo test 
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
#include <stdio.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <getopt.h>
#include <time.h>


char *argv0;

char  send_buff[1024];
int   send_size = 0;

char  recv_buff[1024];
int   recv_size;


static void stop (const char *fmt, ...)
{
	int      err = errno;
	va_list  ap;

	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);

	fprintf(stderr, ": %s\n", strerror(err));
	exit(1);
}


/** control for main loop for orderly shutdown */
static int main_loop = 1;
static void signal_fatal (int signum)
{
	main_loop = 0;
}


int main (int argc, char **argv)
{
	signal(SIGTERM, signal_fatal);
	signal(SIGINT,  signal_fatal);

	// set stdin nonblocking
	int flags = fcntl(0, F_GETFL);
	if ( flags < 0 )
		stop("fcntl(0, F_GETFL)");
	if ( fcntl(0, F_SETFL, flags | O_NONBLOCK) < 0 )
		stop("fcntl(0, F_SETFL)");
	
	// save old terminal modes and make raw
	struct termios tio_old;
	if ( isatty(0) )
	{
		if ( tcgetattr(0, &tio_old) )
			stop("tcgetattr(0, &tio_old)");

		struct termios tio_new = tio_old;
		tio_new.c_lflag &= ~(ECHO|ICANON);
		if ( tcsetattr(0, TCSANOW, &tio_new) )
			stop("tcsetattr(0, TCSANOW, &tio_new)");
	}

	time_t     now = time(NULL);
	struct tm *tm  = localtime(&now);
	char      *dst = send_buff;
	char      *end = send_buff + sizeof(send_buff);
	memset(send_buff, 0, sizeof(send_buff));
	dst += snprintf(dst, end - dst, "%5d: ", getpid());
	dst += strftime(dst, end - dst, "%c: Starting up\n", tm);
	send_size = dst - send_buff;
	if ( write(1, send_buff, send_size) < send_size )
		stop("Failed to write() %ld bytes", send_size);

	/* Standard set of vars to support select */
	struct timeval  tv_cur;
	fd_set          rfds;
	int             mfda;
	int             sel;
	int             ret;

	while ( main_loop )
	{
		FD_ZERO(&rfds);
		FD_SET(0, &rfds);
		mfda = 0;

		tv_cur.tv_sec  = 0;
		tv_cur.tv_usec = 125000;
		errno = 0;
		if ( (sel = select(mfda + 1, &rfds, NULL, NULL, &tv_cur)) < 0 )
		{
			if ( errno != EINTR )
				fprintf(stderr, "select: %s: stop\n", strerror(errno));
			break;
		}

		now = time(NULL);
		tm  = localtime(&now);
		dst = send_buff;
		end = send_buff + sizeof(send_buff);

		memset(send_buff, 0, sizeof(send_buff));
		dst += snprintf(dst, end - dst, "%5d: ", getpid());
		dst += strftime(dst, end - dst, "%c: ",  tm);

		// idle
		if ( !sel )
			dst += snprintf(dst, end - dst, "Idling\n");

		// data from terminal
		else if ( FD_ISSET(0, &rfds) )
		{
			memset(recv_buff, 0, sizeof(recv_buff));
			if ( (recv_size = read(0, recv_buff, sizeof(recv_buff))) < 0 )
				stop("read() from terminal");

			if ( recv_size )
			{
				char *src = recv_buff;
				switch ( tolower(*recv_buff) )
				{
					case 'q':
					case '\033':
						dst += snprintf(dst, end - dst, "Shutdown command received\n");
						main_loop = 0;
						break;

					default:
						dst += snprintf(dst, end - dst, "%d bytes received: {", recv_size);
						while ( recv_size-- && dst < end )
							dst += snprintf(dst, end - dst, " %02x", *src++);
						dst += snprintf(dst, end - dst, " }\n");
				}
			}
		}

		else
			dst += snprintf(dst, end - dst, "sel %d, not in rfds, wtf?\n", sel);

		// data to send
		if ( (send_size = dst - send_buff) > 0 )
		{
			if ( (ret = write(1, send_buff, send_size)) < send_size )
				stop("Failed to write() %ld bytes", send_size);
			send_size = 0;
		}
	}

	if ( fcntl(0, F_SETFL, flags) < 0 )
		stop("fcntl(0, F_SETFL)");

	if ( isatty(0) )
	{
		if ( tcsetattr(0, TCSANOW, &tio_old) )
			stop("tcsetattr(0, TCSANOW, &tio_old)");
	}

	return 0;
}

/* Ends    : src/tool/test-stub.c */
