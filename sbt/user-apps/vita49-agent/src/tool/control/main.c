/** Control tool main loop
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

#include <config/config.h>

#include <sbt_common/log.h>
#include <sbt_common/clocks.h>
#include <common/default.h>
#include <common/control/local.h>
#include <v49_message/types.h>
#include <v49_message/common.h>
#include <v49_message/control.h>

#include <v49_client/socket.h>
#include <v49_client/sequence.h>


LOG_MODULE_STATIC("control_main", LOG_LEVEL_INFO);


char *argv0;
char *opt_log       = NULL;
char *opt_config    = DEF_CONFIG_PATH;
long  opt_reps      = 1;
long  opt_pause     = 0;

char *opt_sock_type = "srio";
char *opt_sock_addr = NULL;

char *default_argv[] = { "list", NULL };


char  send_buff[CONTROL_LOCAL_BUFF_SIZE];
int   send_size = 0;

char  recv_buff[CONTROL_LOCAL_BUFF_SIZE];
int   recv_size;

struct socket *sock;



#if 0
void stop (const char *fmt, ...)
{
	int      err = errno;
	va_list  ap;

	int t1 = clocks_to_ms(get_clocks());
	fprintf(stderr, "[%5u.%03u] ", (t1 / 1000) % 100000, t1 % 1000);

	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);

	fprintf(stderr, ": %s\n", strerror(err));
	exit(1);
}
#endif


/** control for main loop for orderly shutdown */
static int main_loop = 1;
static void signal_fatal (int signum)
{
	main_loop = 0;
}


static void usage (void)
{
	printf ("Usage: %s [-r reps] [-p pause] [-d lvl] [-l log] [-c config]\n"
	        "       %*s [-s type[:address]] [command [args ...]]\n\n"
	        "Where:\n"
	        "-r reps       Run the command reps repetitions\n"
	        "-p pause      Pause between reps for pause ms\n"
	        "-d [mod:]lvl  Debug: set module or global message verbosity (0/focus - 5/trace)\n"
	        "-l log        Set log file (default stderr)\n"
	        "-c config     Specify a different config file (default %s)\n" 
	        "-s type[:adr] Specify a different socket type and address than config\n"
	        "\n"
	        "Socket types and optional address format.  The type name may be followed by an\n"
	        "optional address, separated by a : character.  Types recognized:\n"
	        "unix - Unix Domain socket: address is the filesystem path of the socket:\n"
	        "       -s unix\n"
	        "       -s unix:/path/to/socket\n"
	        "srio - Serial RapidIO type 11 (message): address is the mbox number,\n"
	        "       optionally followed by the RapidIO device-ID of the agent node:\n"
	        "       -s srio\n"
	        "       -s srio:3\n"
	        "       -s srio:3,12\n",
	        argv0, strlen(argv0), " ", DEF_CONFIG_PATH);
	        
	exit (1);
}


int main (int argc, char **argv)
{
	char **cmd_argv;
	int    cmd_argc;
	char  *ptr;
	int    opt;
	int    ret = 0;

	// avoid basename, doesn't work in some uClibc versions
	if ( (argv0 = strrchr(*argv, '/')) && argv0[1] )
		argv0++;
	else
		argv0 = *argv;

	// defaults
	log_init_module_list();
	log_dupe(stderr);

	// basic arguments parsing
	while ( (opt = getopt(argc, argv, "r:p:d:l:c:s:")) != -1 )
		switch ( opt )
		{
			case 'l': opt_log    = optarg;  break;
			case 'c': opt_config = optarg;  break;
			case 'r': opt_reps   = strtoul(optarg, NULL, 0); break;
			case 'p': opt_pause  = strtoul(optarg, NULL, 0) * 1000; break;

			// set debug verbosity level and enable trace
			case 'd':
				if ( (ptr = strchr(optarg, ':')) || (ptr = strchr(optarg, '=')) ) // for particular module(s)
				{
					*ptr++ = '\0';
					if ( (opt = log_level_for_label(ptr)) < 0 && isdigit(*ptr) )
						opt = strtoul(ptr, NULL, 0);
					if ( log_set_module_level(optarg, opt) )
					{
						if ( errno == ENOENT )
						{
							char **list = log_get_module_list();
							char **walk;
							printf("Available modules: {");
							for ( walk = list; *walk; walk++ )
								printf(" %s", *walk);
							printf(" }\n");
							free(list);
						}
						usage();
					}
				}
				else // globally
				{
					if ( (opt = log_level_for_label(optarg)) < 0 && isdigit(*optarg) )
						opt = strtoul(optarg, NULL, 0);
					fprintf(stderr, "optarg '%s' -> %d\n", optarg, opt);
					if ( log_set_global_level(opt) )
						usage();
				}
				log_trace(1);
				break;

			case 's':
				if ( (ptr = strchr(optarg, ':')) ) // for particular module(s)
				{
					*ptr++ = '\0';
					opt_sock_addr = ptr;
				}
				else
					opt_sock_addr = NULL;
				opt_sock_type = optarg;
				LOG_DEBUG("-s set type '%s', addr '%s'\n", opt_sock_type, opt_sock_addr);
				break;

			default:
				usage();
		}

	if ( !(sock = socket_alloc(opt_sock_type)) )
	{
		LOG_ERROR("Socket type '%s' unknown.\n", opt_sock_type);
		usage();
	}

	// Command-line overrides config file
	if ( opt_sock_addr )
	{
		LOG_DEBUG("Configure sock with addr '%s'\n", opt_sock_addr);
		if ( socket_cmdline(sock, "addr", opt_sock_addr) )
		{
			LOG_ERROR("Bad command-line address: %s", opt_sock_addr);
			return 1;
		}
		LOG_DEBUG("Configured sock with addr '%s'\n", opt_sock_addr);
	}
	else if ( socket_config_inst(sock, opt_config) )
	{
		LOG_ERROR("Failed to read config file: %s", strerror(errno));
		return 1;
	}

	// remaining arguments form the command
	if ( optind < argc )
	{
		cmd_argv = argv + optind;
		cmd_argc = 1 + argc - optind;
	}
	else
	{
		cmd_argv = default_argv;
		cmd_argc = sizeof(default_argv) / sizeof(default_argv[0]);
	}

	// Find sequence implementing that command
	struct sequence_map *map = sequence_find(cmd_argv[0]);
	if ( !map )
	{
		LOG_ERROR("Command '%s' unknown.  Available commands:\n", cmd_argv[0]);
		sequence_list(LOG_LEVEL_ERROR);
		return 1;
	}
	else if ( !strcmp(map->name, "list") )
	{
		LOG_FOCUS("Available commands:\n");
		sequence_list(LOG_LEVEL_FOCUS);
		return 0;
	}
	LOG_DEBUG("Command '%s' matched: %s\n", map->name, map->desc);

	// open connection to daemon
	if ( socket_check(sock) < 0 )
	{
		LOG_ERROR("Failed to connect to server: %s", strerror(errno));
		return 1;
	}


	signal(SIGTERM, signal_fatal);
	signal(SIGINT,  signal_fatal);

	long rep = 0;
	while ( rep++ < opt_reps && main_loop )
		if ( (ret = map->func(sock, cmd_argc, cmd_argv)) < 0 )
		{
			LOG_ERROR("%s: %d: %s\n", map->name, ret, strerror(errno));
			ret = 1;
			break;
		}
		else if ( opt_pause )
			usleep(opt_pause);

#if 0
	// get process list
	LOG_DEBUG("request list...\n");
	int  ret;
	memset(send_buff, 0, sizeof(send_buff));
	send_size = snprintf(send_buff, sizeof(send_buff), "list");
	if ( (ret = write(sock, send_buff, send_size)) < send_size )
		stop("Failed to write() %ld bytes", send_size);
	LOG_DEBUG("requested list...\n");
	
	LOG_DEBUG("receive list...\n");
	if ( (recv_size = read(sock, recv_buff, sizeof(recv_buff))) < 0 )
		stop("Failed to read()");
	LOG_DEBUG("received list...\n");

	// no args: get process list, display, and exit
	if ( ! *argv )
	{
		LOG_DEBUG("display list...\n");
		printf("Process list (%d bytes):\n", recv_size);
		write(1, recv_buff, recv_size);
		printf("\n");
		LOG_DEBUG("displayed list...\n");
		LOG_DEBUG("close sock...\n");
		close(sock);
		LOG_DEBUG("closed sock...\n");
		return 0;
	}

	// args: format and send, then go interactive
	LOG_DEBUG("format args...\n");
	memset(send_buff, 0, sizeof(send_buff));
	char *ptr = send_buff;
	char *end = send_buff + sizeof(send_buff) - 2;

	while ( *argv && ptr < end )
	{
		ptr += snprintf(ptr, end - ptr, "%s", *argv++);
		if ( ptr < end )
			*ptr++ = '\0';
	}

	if ( ptr >= end )
		stop("Arguments too long for buffer size\n");
	else
		*ptr++ = '\0';
	send_size = ptr - send_buff;
	LOG_DEBUG("formatted args...\n");

	// set stdin nonblocking
	int flags = fcntl(0, F_GETFL);
	if ( flags < 0 )
		stop("fcntl(0, F_GETFL)");
	if ( fcntl(0, F_SETFL, flags | O_NONBLOCK) < 0 )
		stop("fcntl(0, F_SETFL)");
	
	// save old terminal modes and make raw
	struct termios tio_old;
	if ( tcgetattr(0, &tio_old) )
		stop("tcgetattr(0, &tio_old)");

	struct termios tio_new = tio_old;
	tio_new.c_lflag &= ~(ECHO|ICANON);
	if ( tcsetattr(0, TCSANOW, &tio_new) )
		stop("tcsetattr(0, TCSANOW, &tio_new)");

	/* Standard set of vars to support select */
	struct timeval  tv_cur;
	fd_set          rfds;
	fd_set          wfds;
	int             sel;

	while ( main_loop )
	{
		FD_ZERO(&rfds);
		FD_ZERO(&wfds);

		FD_SET(0,    &rfds);
		FD_SET(sock, &rfds);
		if ( send_size )
			FD_SET(sock, &wfds);

		tv_cur.tv_sec  = 1;
		tv_cur.tv_usec = 0;
		LOG_DEBUG("top of loop, select:\n");
		if ( (sel = select(sock + 1, &rfds, &wfds, NULL, &tv_cur)) < 0 )
		{
			LOG_ERROR("select: %s: stop\n", strerror(errno));
			break;
		}
		else if ( !sel )
		{
			LOG_DEBUG("select: idle\n");
			continue;
		}
		LOG_DEBUG("select: %d\n", sel);

		// received data from server - send to terminal
		if ( FD_ISSET(sock, &rfds) )
		{
			LOG_DEBUG("sock is in rfds\n");
			if ( (recv_size = read(sock, recv_buff, sizeof(recv_buff))) < 1 )
			{
				LOG_INFO("server closed connection\n");
				main_loop = 0;
			}
			else
				write(1, recv_buff, recv_size);
		}
		else
			LOG_DEBUG("sock NOT in rfds\n");

		// data to send to server - send
		if ( send_size && FD_ISSET(sock, &wfds) )
		{
			LOG_DEBUG("sock is in wfds and send_size %d\n", send_size);
			if ( (ret = write(sock, send_buff, send_size)) < send_size )
				stop("Failed to write() %ld bytes", send_size);
			send_size = 0;
		}
		else
			LOG_DEBUG("sock NOT in wfds or !send_size\n");

		// data from terminal - send next loop
		if ( FD_ISSET(0, &rfds) )
		{
			LOG_DEBUG("stdin is in rfds\n");
			memset(send_buff, 0, sizeof(send_buff));
			if ( (send_size = read(0, send_buff, sizeof(send_buff))) < 0 )
				stop("read() from terminal");
		}
		else
			LOG_DEBUG("stdin NOT in rfds\n");

		LOG_DEBUG("bottom of loop\n");
	}

	if ( fcntl(0, F_SETFL, flags) < 0 )
		stop("fcntl(0, F_SETFL)");

	if ( tcsetattr(0, TCSANOW, &tio_old) )
		stop("tcsetattr(0, TCSANOW, &tio_old)");

	close(sock);
#endif

	socket_close(sock);
	socket_free(sock);

	return ret;
}

/* Ends    : src/tool/control.c */
