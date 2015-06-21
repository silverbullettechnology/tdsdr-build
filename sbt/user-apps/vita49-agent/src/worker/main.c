/** Worker main loop
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
#include <signal.h>
#include <unistd.h>
#include <time.h>
#include <ctype.h>
#include <errno.h>

#include <sbt_common/log.h>
#include <sbt_common/util.h>
#include <sbt_common/timer.h>
#include <sbt_common/mbuf.h>
#include <sbt_common/mqueue.h>

#include <common/default.h>

#include <worker/config.h>
#include <worker/message.h>


LOG_MODULE_STATIC("worker", LOG_LEVEL_DEBUG);


char           *argv0 = NULL;
char           *worker_opt_log     = NULL;
char           *worker_opt_config  = DEF_CONFIG_PATH;
char           *worker_opt_section = NULL;
struct timeval  worker_opt_tv_min  = { .tv_sec = 0, .tv_usec = 1000 };
struct timeval  worker_opt_tv_max  = { .tv_sec = 1, .tv_usec = 0 };
int             worker_opt_timer   = 5;
unsigned        worker_sid;


/** control for main loop for orderly shutdown */
typedef enum
{
	ML_STOPPED,
	ML_STOPPING,
	ML_RUNNING
}
main_loop_t;
static main_loop_t main_loop = ML_RUNNING;
void worker_shutdown (void)
{
	LOG_INFO("Orderly shutdown command received\n");
	main_loop = ML_STOPPING;
}


static void signal_fatal (int signum)
{
	LOG_ERROR ("Caught fatal signal %d, shutting down\n", signum);
	main_loop = ML_STOPPING;
}

static void signal_config (int signum)
{
	LOG_ERROR ("Caught non-fatal signal %d, ignoring\n", signum);
}


static void signal_trace (void)
{
	static char *trace[32];
	int          idx;

	log_call_trace(trace, 32);
	for ( idx = 0; idx < 32; idx++ )
		if ( trace[idx] )
			LOG_ERROR(" %2d: %s\n", idx, trace[idx]);
}

static void signal_segfault (int signum)
{
	LOG_ERROR("Caught SEGFAULT, partial call trace:\n");
	signal_trace();
	LOG_ERROR("Exit due to SEGFAULT\n");
//	shutdown();
	exit(1);
}

static int loop_stall = 0;
static void signal_alarm (int signum)
{
	LOG_ERROR("Alarm: main loop stall, partial call trace:\n");
	signal_trace();
	loop_stall = 1;
}



static void usage (void)
{
	printf ("Usage: %s [-f] [-d lvl] [-l log] [-c config] SID\n\n"
	        "Where:\n"
	        "-d [mod:]lvl  Debug: set module or global message verbosity (0/focus - 5/trace)\n"
	        "-l log        Set log file (default stderr)\n"
	        "-c config     Specify a different config file (default %s)\n"
			"SID           Specifies the numeric Stream-ID assigned to this worker\n",
	        argv0, DEF_CONFIG_PATH);
	        
	exit (1);
}


int main (int argc, char **argv)
{
	// same as basename(), but works reliably under uClibc
	if ( (argv0 = strrchr(argv[0], '/')) && argv0[1] )
		argv0++;
	else
		argv0 = argv[0];

	// not sure we use random stuff in this project...
	srand(time(NULL));
	log_init_module_list();
	log_dupe(stderr);

	// basic arguments parsing
	int   opt;
	char *ptr;
	while ( (opt = getopt(argc, argv, "fd:l:c:")) != -1 )
		switch ( opt )
		{
			case 'l': worker_opt_log    = optarg;  break;
			case 'c': worker_opt_config = optarg;  break;

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
					if ( log_set_global_level(opt) )
						usage();
				}
				log_trace(1);
				break;

			default:
				usage();
		}
	
	if ( optind >= argc || !(worker_sid = strtoul(argv[optind], NULL, 0)) )
		usage();
	LOG_DEBUG("Section: %u\n", worker_sid);

    // Global config first, and learn names of the module instances
	if ( worker_config(worker_opt_config, worker_opt_section) )
	{
		LOG_ERROR ("Initial config failed: %s\n", strerror(errno));
		return 1;
	}

	if ( worker_opt_log && log_fopen(worker_opt_log, "a") )
	{
		LOG_ERROR ("Failed to open log file %s: %s\n", worker_opt_log, strerror(errno));
		return 1;
	}
	LOG_CALL_ENTER("main");

	// have to init this before using any timers
	clock_rate_init();

	// shutdown and signal handlers
//	atexit(shutdown);
	signal(SIGSEGV, signal_segfault);
	signal(SIGTERM, signal_fatal);
	signal(SIGINT,  signal_fatal);
	signal(SIGHUP,  signal_config);
	signal(SIGALRM, signal_alarm);
	signal(SIGCHLD, SIG_IGN);
	signal(SIGPIPE, SIG_IGN);

	/* Standard set of vars to support select */
	struct timeval  tv_cur;
	struct mbuf    *mbuf;
//	clocks_t        timer_due;
	fd_set          rfds;
	fd_set          wfds;
	int             nfds;
	int             sel;
	int             ret;
	char            buff[256];

	/* Primary select/poll loop, with dispatch following */
	while ( main_loop > ML_STOPPED )
	{
		FD_ZERO(&rfds);
		FD_ZERO(&wfds);
		FD_SET(0, &rfds);
		FD_SET(1, &wfds);
		nfds = 1;

		// initial timeout calculated from timer head: a long fixed select() timeout when
		// idle makes short timers jittery, while a short select() timeout with long
		// timers wastes CPU.  Set timeout based on next timer due, then bound by sensible
		// configurable min/max limits: when idle the select() will yield the CPU and the 
		// timer will be due at the bottom of the loop.
//		if ( (timer_due = timer_head_due_in()) < TIMER_LIST_EMPTY )
//		{
//			clocks_to_tv (&tv_cur, timer_due);
//			if ( timercmp(&tv_cur, &worker_opt_tv_max, >) )
//				tv_cur = worker_opt_tv_max;
//			else if ( timercmp(&tv_cur, &worker_opt_tv_min, <) )
//				tv_cur = worker_opt_tv_min;
//		}
//		else 
			tv_cur = worker_opt_tv_max;
		
		alarm(tv_cur.tv_sec + 5);
		if ( (sel = select(nfds + 1, &rfds, &wfds, NULL, &tv_cur)) < 0 )
		{
			if ( errno != EINTR )
			{
				LOG_ERROR("select() failed: %s\n", strerror(errno));
				main_loop = ML_STOPPED;
			}
			else
				LOG_INFO("select(): EINTR ignored\n");
			sel = 0;
		}

		// read southbound traffic
		if ( FD_ISSET(0, &rfds) )
		{
			if ( !(mbuf = mbuf_alloc(DEFAULT_MBUF_SIZE, 0)) )
			{
				LOG_ERROR("mbuf_alloc() failed: %s\n", strerror(errno));
				while ( read(0, buff, sizeof(buff)) > 0 ) ;
				main_loop = ML_STOPPING;
			}

			if ( (ret = mbuf_read(mbuf, 0, DEFAULT_MBUF_SIZE)) < 0 )
			{
				LOG_ERROR("mbuf_read() failed: %s\n", strerror(errno));
				main_loop = ML_STOPPING;
			}

			if ( ret )
				worker_southbound(mbuf);
			else
				LOG_DEBUG("Ignore zero-length read\n");
		}

		// send northbound traffic
		if ( FD_ISSET(1, &wfds) )
		{
			// send queued, if nothing queued but shutting down, we're finished
			if ( (mbuf = mqueue_dequeue(&message_queue)) )
			{
				mbuf_cur_set_beg(mbuf);
				if ( (ret = mbuf_write(mbuf, 1, DEFAULT_MBUF_SIZE)) < 0 )
				{
					LOG_ERROR("mbuf_write() failed: %s\n", strerror(errno));
					main_loop = ML_STOPPING;
				}
				else
					mbuf_deref(mbuf);
			}
			else if ( main_loop == ML_STOPPING )
			{
				LOG_INFO("TX queue empty while stopping, stop\n");
				main_loop = ML_STOPPED;
			}
		}

		// service timers last
		timer_check (worker_opt_timer);
	}
	if ( loop_stall )
	{
		LOG_ERROR("After stalled main loop, partial call trace:\n");
		signal_trace();
	}

	LOG_ERROR("Shutting down\n");
	return 0;
}

