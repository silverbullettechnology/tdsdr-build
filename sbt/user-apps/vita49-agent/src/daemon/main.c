/** Daemon main loop
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
#include <sbt_common/growlist.h>
#include <sbt_common/packlist.h>

#include <common/default.h>
#include <common/resource.h>

#include <daemon/resource.h>
#include <daemon/worker.h>
#include <daemon/control.h>
#include <daemon/message.h>
#include <daemon/config.h>
#include <daemon/daemon.h>


char           *argv0;
int             daemon_opt_daemon = 1;
char           *daemon_opt_log    = NULL;
char           *daemon_opt_config = DEF_CONFIG_PATH;
struct timeval  daemon_opt_tv_min = { .tv_sec = 0, .tv_usec = 1000 };
struct timeval  daemon_opt_tv_max = { .tv_sec = 1, .tv_usec = 0 };
int             daemon_opt_timer_limit = 5;


LOG_MODULE_STATIC("daemon", LOG_LEVEL_INFO);


/** control for main loop for orderly shutdown */
typedef enum
{
	ML_STOPPED,
	ML_WAITING,
	ML_STOPPING,
	ML_RUNNING
}
main_loop_t;
static main_loop_t main_loop = ML_RUNNING;
void daemon_shutdown (struct mbuf *mbuf)
{
	LOG_ERROR ("Orderly shutdown command received\n");
	main_loop = ML_STOPPING;
	mbuf_deref(mbuf);
}


/** Shutdown function, called during orderly shutdown
 *
 *  \note This is not called if the application is killed by a signal, even
 *  caught; thus the fatal signal handler must call shutdown() explicitly 
 */
static void shutdown (void)
{
	LOG_ERROR ("Starting shutdown.\n");

	struct worker *worker;
	growlist_reset(&worker_list);
	while ( (worker = growlist_next(&worker_list) ) )
	{
		worker_close(worker);
		free((void *)worker->name);
		worker_free(worker);
		growlist_reset(&worker_list);
	}
	growlist_done(&worker_list, NULL, NULL);

	struct control *ctrl;
	growlist_reset(&control_list);
	while ( (ctrl = growlist_next(&control_list) ) )
	{
		control_close(ctrl);
		free((void *)ctrl->name);
		control_free(ctrl);
		growlist_reset(&control_list);
	}
	growlist_done(&control_list, NULL, NULL);

	LOG_ERROR ("Shutdown complete.\n");
	log_close();
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
	shutdown();
	exit(1);
}

static int loop_stall = 0;
static void signal_alarm (int signum)
{
	LOG_ERROR("Alarm: main loop stall, partial call trace:\n");
	signal_trace();
	loop_stall = 1;
}


/* test timer / log */
static clocks_t      mark_timer_period;
static struct timer  mark_timer;
static void mark_timer_cb (void *arg)
{
	LOG_INFO ("MARK\n");
	timer_attach (&mark_timer, mark_timer_period, mark_timer_cb, arg);
}


static void usage (void)
{
	printf ("Usage: %s [-f] [-d lvl] [-l log] [-c config]\n\n"
	        "Where:\n"
	        "-f            Foreground: don't fork and become a daemon\n"
	        "-d [mod:]lvl  Debug: set module or global message verbosity (0/focus - 5/trace)\n"
	        "-l log        Set log file (default stderr)\n"
	        "-c config     Specify a different config file (default %s)\n",
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
	log_dupe(stderr);

	// basic arguments parsing
	int   opt;
	char *ptr;
	while ( (opt = getopt(argc, argv, "fd:l:c:")) != -1 )
		switch ( opt )
		{
			case 'f': daemon_opt_daemon = 0;       break;
			case 'l': daemon_opt_log    = optarg;  break;
			case 'c': daemon_opt_config = optarg;  break;

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

			default:
				usage();
		}

    // Global config first, and learn names of the module instances
	if ( daemon_config(daemon_opt_config) )
	{
		LOG_ERROR ("Initial config failed: %s\n", strerror(errno));
		return 1;
	}

    // Resource config next
	if ( resource_config(resource_config_path) )
	{
		LOG_ERROR ("Resource config failed: %s\n", strerror(errno));
		return 1;
	}

	if ( daemon_opt_log && log_fopen(daemon_opt_log, "a") )
	{
		LOG_ERROR ("Failed to open log file %s: %s\n", daemon_opt_log, strerror(errno));
		return 1;
	}
	LOG_CALL_ENTER("main");

	// become a daemon
	LOG_INFO ("daemon startup%s: %d\n", daemon_opt_daemon ? "" : " (foreground)", getpid());
	if ( daemon_opt_daemon )
	{
		switch ( fork() )
		{
			case -1: // error case
				fprintf (stderr, "%s: failed to fork(), stop.\n", argv0);
				return  (1);

			case 0: // child thread
				setsid  ();
				chdir   ("/");
				freopen ("/dev/null", "r", stdin);
				freopen ("/dev/null", "w", stdout);
				freopen ("/dev/null", "w", stderr);
				break;

			default: // parent thread exits
				return(0);
		}
	}

	// have to init this before using any timers
	clock_rate_init();

	// shutdown and signal handlers
	atexit(shutdown);
	signal(SIGSEGV, signal_segfault);
	signal(SIGTERM, signal_fatal);
	signal(SIGINT,  signal_fatal);
	signal(SIGHUP,  signal_config);
	signal(SIGALRM, signal_alarm);
	signal(SIGCHLD, SIG_IGN);
	signal(SIGPIPE, SIG_IGN);


	// Config each worker instance
	struct worker *worker;
	growlist_reset(&worker_list);
	while ( (worker = growlist_next(&worker_list) ) )
		if ( worker_config_inst(worker, daemon_opt_config) )
		{
			LOG_ERROR("%s: Configuring from %s failed: %s\n",
			          worker->name, daemon_opt_config, strerror(errno));
			return 1;
		}

	// Config each control instance
	struct control *ctrl;
	growlist_reset(&control_list);
	while ( (ctrl = growlist_next(&control_list) ) )
		if ( control_config_inst(ctrl, daemon_opt_config) )
		{
			LOG_ERROR("%s: Configuring from %s failed: %s\n",
			          control_name(ctrl), daemon_opt_config, strerror(errno));
			return 1;
		}

	// Check/open each worker
	worker_state_t state;
	growlist_reset(&worker_list);
	while ( (worker = growlist_next(&worker_list) ) )
		if ( (state = worker_state_get(worker)) < 0 )
		{
			LOG_ERROR("%s: Startup check failed: %s\n", worker->name, strerror(errno));
			return 1;
		}

	// Check each control instance - passing this doesn't mean remote connections are up
	// yet, but that the config is valid and they should come up eventually 
	growlist_reset(&control_list);
	while ( (ctrl = growlist_next(&control_list) ) )
		if ( control_check(ctrl) < 0 )
		{
			LOG_ERROR("%s: Startup check failed: %s\n", control_name(ctrl), strerror(errno));
			return 1;
		}

	// mark timer
	mark_timer_period = s_to_clocks(60);
	timer_attach (&mark_timer, mark_timer_period, mark_timer_cb, NULL);

	/* Standard set of vars to support select */
	struct timeval  tv_cur;
	struct mbuf    *mbuf;
//	clocks_t        timer_due;
	fd_set          rfds;
	fd_set          wfds;
	int             nfds;
	int             sel;
	int             ret;

	/* Primary select/poll loop, with dispatch following */
	while ( main_loop > ML_STOPPED )
	{
		FD_ZERO(&rfds);
		FD_ZERO(&wfds);
		nfds = -1;
		
		// first check controls for fatal errors, if none then fd_set them
		growlist_reset(&control_list);
		while ( (ctrl = growlist_next(&control_list) ) )
			if ( control_check(ctrl) < 0 )
			{
				LOG_ERROR("%s: Runtime check failed: %s\n", control_name(ctrl), strerror(errno));
				main_loop = ML_STOPPED;
			}
			else
				control_fd_set(ctrl, &rfds, &wfds, &nfds);

		// next check workers for fatal errors, if none then fd_set them
		growlist_reset(&worker_list);
		while ( (worker = growlist_next(&worker_list) ) )
			if ( (state = worker_state_get(worker)) < 0 )
			{
				LOG_ERROR("%s: Runtime check failed: %s\n", worker->name, strerror(errno));
				main_loop = ML_STOPPED;
			}
			else
				worker_fd_set(worker, &rfds, &wfds, &nfds);

		// initial timeout calculated from timer head: a long fixed select() timeout when
		// idle makes short timers jittery, while a short select() timeout with long
		// timers wastes CPU.  Set timeout based on next timer due, then bound by sensible
		// configurable min/max limits: when idle the select() will yield the CPU and the 
		// timer will be due at the bottom of the loop.
//		if ( (timer_due = timer_head_due_in()) < TIMER_LIST_EMPTY )
//		{
//			clocks_to_tv (&tv_cur, timer_due);
//			if ( timercmp(&tv_cur, &daemon_opt_tv_max, >) )
//				tv_cur = daemon_opt_tv_max;
//			else if ( timercmp(&tv_cur, &daemon_opt_tv_min, <) )
//				tv_cur = daemon_opt_tv_min;
//		}
//		else 
			tv_cur = daemon_opt_tv_max;

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

		// read control messages first
		growlist_reset(&control_list);
		while ( (ctrl = growlist_next(&control_list) ) )
		{
			if ( (ret = control_fd_is_set(ctrl, &rfds, NULL)) < 0 )
			{
				LOG_ERROR("%s: Read ready failed: %s\n", control_name(ctrl), strerror(errno));
				main_loop = ML_STOPPED;
			}
			else if ( ret > 0 && control_read(ctrl, &rfds) < 0 )
			{
				LOG_ERROR("%s: Read data failed: %s\n", control_name(ctrl), strerror(errno));
				main_loop = ML_STOPPED;
			}
			else
				LOG_TRACE("%s: Read idle\n", control_name(ctrl));
		}

		// read worker messages first
		growlist_reset(&worker_list);
		while ( (worker = growlist_next(&worker_list) ) )
		{
			if ( (ret = worker_fd_is_set(worker, &rfds, NULL)) < 0 )
			{
				LOG_ERROR("%s: Read ready failed: %s\n", worker->name, strerror(errno));
				main_loop = ML_STOPPED;
			}
			else if ( ret > 0 && worker_read(worker, &rfds) < 0 )
			{
				LOG_ERROR("%s: Read data failed: %s\n", worker->name, strerror(errno));
				main_loop = ML_STOPPED;
			}
			else
				LOG_TRACE("%s: Read idle\n", worker->name);
		}

		// do internal processing and route southbound (received from control) messages
		growlist_reset(&control_list);
		while ( (ctrl = growlist_next(&control_list) ) )
		{
			control_process(ctrl);
			while ( (mbuf = control_dequeue(ctrl)) )
				daemon_southbound(mbuf);
			control_process(ctrl);
		}

		// do internal processing and route northbound (received from worker) messages
		growlist_reset(&worker_list);
		while ( (worker = growlist_next(&worker_list) ) )
		{
			worker_process(worker);
			while ( (mbuf = worker_dequeue(worker)) )
				daemon_northbound(mbuf);
			worker_process(worker);
		}

		// write control messages next
		growlist_reset(&control_list);
		while ( (ctrl = growlist_next(&control_list) ) )
		{
			if ( (ret = control_fd_is_set(ctrl, NULL, &wfds)) < 0 )
			{
				LOG_ERROR("%s: Write ready failed: %s\n", control_name(ctrl), strerror(errno));
				main_loop = ML_STOPPED;
			}
			else if ( ret > 0 && control_write(ctrl, &wfds) < 0 )
			{
				LOG_ERROR("%s: Write data failed: %s\n", control_name(ctrl), strerror(errno));
				main_loop = ML_STOPPED;
			}
			else
				LOG_TRACE("%s: Write idle\n", control_name(ctrl));
		}

		// write worker messages last
		growlist_reset(&worker_list);
		while ( (worker = growlist_next(&worker_list) ) )
		{
			if ( (ret = worker_fd_is_set(worker, NULL, &wfds)) < 0 )
			{
				LOG_ERROR("%s: Write ready failed: %s\n", worker->name, strerror(errno));
				main_loop = ML_STOPPED;
			}
			else if ( ret > 0 && worker_write(worker, &wfds) < 0 )
			{
				LOG_ERROR("%s: Write data failed: %s\n", worker->name, strerror(errno));
				main_loop = ML_STOPPED;
			}
			else
				LOG_TRACE("%s: Write idle\n", worker->name);
		}

		worker_state_t state;
		int            count = 0;
		switch ( main_loop )
		{
			case ML_STOPPING:
				LOG_DEBUG("stop requested, stop all workers\n");
				growlist_reset(&worker_list);
				while ( (worker = growlist_next(&worker_list) ) )
				{
					worker->flags &= ~(WF_RESTART_CLEAN|WF_RESTART_ERROR);
					worker_state_set(worker, WS_STOP);
				}
				main_loop = ML_WAITING;
				// fall-through

			case ML_WAITING:
				growlist_reset(&worker_list);
				while ( (worker = growlist_next(&worker_list) ) )
					switch ( (state = worker_state_get(worker)) )
					{
						case WS_READY:
						case WS_LIMIT:
							LOG_DEBUG("%s: state %s, ok\n", worker_name(worker),
							          worker_state_desc(state));
							break;

						default:
							count++;
							LOG_DEBUG("%s: state %s, waiting...\n", worker_name(worker),
							          worker_state_desc(state));
					}

				if ( !count )
				{
					main_loop = ML_STOPPED;
					LOG_DEBUG("all workers stopped, exit main loop\n");
				}
				break;

			case ML_RUNNING:
				growlist_reset(&worker_list);
				while ( (worker = growlist_next(&worker_list) ) )
					switch ( (state = worker_state_get(worker)) )
					{
						case WS_ZOMBIE:
							LOG_DEBUG("%s: Reap zombie worker\n", worker_name(worker));
							worker_free(worker);
							break;

						default:
							LOG_DEBUG("%s: state %s, ignore...\n", worker_name(worker),
							          worker_state_desc(state));
					}

			default:
				LOG_TRACE("main loop continues\n");
		}

		// service timers last
		timer_check (daemon_opt_timer_limit);
	}
	if ( loop_stall )
	{
		LOG_ERROR("After stalled main loop, partial call trace:\n");
		signal_trace();
	}

	LOG_ERROR("Shutting down\n");
	return 0;
}

/* Ends    : src/daemon/main.c */
