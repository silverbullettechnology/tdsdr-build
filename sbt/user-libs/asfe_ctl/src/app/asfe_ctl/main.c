/** \file      src/app/main.c
 *  \brief     Test application
 *  \copyright Copyright 2013,2014 Silver Bullet Technology
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
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include <limits.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <ctype.h>
#include <errno.h>

//#ifdef ASFE_CTL_USE_READLINE 
#include <curses.h>
#include <readline/readline.h>
#include <readline/history.h>
//#endif

#include <asfe_ctl.h>
#include <config.h>

#include "map.h"
#include "main.h"
#include "util.h"

#define MAX_ARGS    32
#define SEP_CHARS   " \t\n\r"
#define DEF_DEV_NUM 0
#define EXPORT_FILE "/tmp/.asfe_ctl.channels"


char *argv0;
int   opt_dev_num  = DEF_DEV_NUM;
char *opt_dev_node = NULL;
/*int   opt_dev_gpio[3] = { -1, -1, -1 };     */
int   opt_soft_fail = 0;
int   opt_interact  = 0;
char *opt_lib_dir   = NULL;

char  env_script_path[PATH_MAX];

static struct 
{
	char *node;
/*	int   gpio[3];     */
}
/*
dev_node_list[] = 
{
	{ "/dev/spidev1.0", { -1, -1, -1 } },
	{ "/dev/spidev1.1", { -1, -1, -1 } },
};    */

dev_node_list[] = 
{
	{ "/dev/spidev1.0" },
	{ "/dev/spidev1.1" },
};

static void export_active_channels (void)
{
	unsigned char  fb[4];
	unsigned char *fp = fb;
	int            fd;
	int            ret;
	int            reg;
	int            adr;

	if ( (fd = open(EXPORT_FILE, O_RDWR|O_CREAT)) < 0 )
		stop(EXPORT_FILE);

	if ( (ret = read(fd, fb, sizeof(fb))) < 0 )
		stop(EXPORT_FILE);
	if ( ret < sizeof(fb) )
		memset(fb + ret, 0, sizeof(fb) - ret);

	if ( opt_dev_num > 0 )
		fp += 2;

	for ( adr = 0x002; adr <= 0x003; adr++ )
	{
		if ( (reg = asfe_ctl_hal_spi_reg_get(adr)) < 0 )
			reg = 0;
		*fp++ = reg & 0xC0;
	}

	if ( lseek(fd, 0, SEEK_SET) )
		stop(EXPORT_FILE);

	if ( (ret = write(fd, fb, sizeof(fb))) != sizeof(fb) )
		stop(EXPORT_FILE);

	if ( close(fd) )
		stop(EXPORT_FILE);
}

int dev_reopen (int num, int reinit)
{
	//int   i;
	//UINT8 v;

#ifdef SIM_HAL
	return 0;
#endif

	if ( num < 0 || num >= sizeof(dev_node_list) / sizeof(dev_node_list[0]) )
	{
		errno = EINVAL;
		return -1;
	}

	if ( reinit )
		export_active_channels();
	if ( reinit && (opt_dev_num != num) )
		asfe_ctl_hal_spi_reg_clr();

	asfe_ctl_hal_linux_spi_done();
	asfe_ctl_hal_linux_gpio_done();

	opt_dev_num  = num;
	opt_dev_node = dev_node_list[num].node;
/*	for ( i = 0; i < 3; i++ )
		opt_dev_gpio[i] = dev_node_list[opt_dev_num].gpio[i];     */

	// TODO: UART
	if ( asfe_ctl_hal_linux_spi_init(opt_dev_node) )
	{
		fprintf(stderr, "asfe_ctl_hal_linux_spi_init(%s): %s\n",
		        opt_dev_node, strerror(errno));
		return -1;
	}

/*	if ( asfe_ctl_hal_linux_gpio_init(opt_dev_gpio) )
	{
		fprintf(stderr, "asfe_ctl_hal_linux_gpio_init(%d,%d,%d): %s\n",
		        opt_dev_gpio[0], opt_dev_gpio[1], opt_dev_gpio[2], strerror(errno));
		return -1;
	}
*/
	//CMB_SPIReadByte(0x002, &v);
	//CMB_SPIReadByte(0x003, &v);

	errno = 0;
	return 0;
}


void stop (const char *fmt, ...)
{
	int      err = errno;
	va_list  ap;

	va_start(ap, fmt); 
	vfprintf(stderr, fmt, ap);
	va_end(ap);

	fprintf(stderr, ": %s\n", strerror(err));
	exit(1);
}

void usage (void)
{
	printf("Run command: asfe_ctl [options] command [args...]\n"
	       "Run script : asfe_ctl [options] script\n"
	       "Interactive: asfe_ctl [options]\n\n"
	       "Options: asfe_ctl [-ei] [-d num] [-D node] [-l dir]\n"
	       "Where:\n"
	       "-e      As for /bin/sh, tolerate command failures\n"
	       "-i      Enter interactive mode after running command or script\n"
/*	       "-g gpio GPIO pins for TXNRX,ENABLE,RESETN (default %d,%d,%d)\n"		*/
	       "-d num  Specify ASFE_CTL device number (0-1, default %d)\n"
	       "-D node Specify ASFE_CTL device node (default %s)\n"
	       "-l dir  Specify library dir (default %s)\n",
/*	       opt_dev_gpio[0], opt_dev_gpio[1], opt_dev_gpio[2],                      */
	       DEF_DEV_NUM, dev_node_list[DEF_DEV_NUM].node, LIB_DIR);
	exit(1);
}


int command (int argc, char **argv)
{
	struct asfe_ctl_map_cmd_t *map;
	int                      ret;

	// check for a script file here, open and run with interact()
	if ( !(map = asfe_ctl_map_find(argv[0])) )
	{
		char  buff[PATH_MAX];
		char *path = argv[0];
		FILE *fp = fopen(argv[0], "r");

		if ( !fp )
		{
			if ( !(path = path_match(buff, sizeof(buff), env_script_path, argv[0])) )
			{
				fprintf(stderr, "%s: unknown as command or script\n", argv[0]);
				return opt_soft_fail ? 0 : -1;
			}
			fp = fopen(path, "r");
		}

		if ( !fp )
		{
			fprintf(stderr, "%s: %s\n", argv[0], strerror(errno));
			return -1;
		}

		ret = script(fp, NULL);
		fclose(fp);
		return ret;
	}

	if ( argc < map->argc )
	{
		fprintf(stderr, "%s: %d arguments given, %d required:\n",
		        map->arg0, argc - 1, map->argc - 1);
		asfe_ctl_map_help(map->name);
		return opt_soft_fail ? 0 : -1;
	}

	// errors in the actual library/middleware layers are >0
	ret = asfe_ctl_map_call(map, argc, (const char **)argv);

	// soft-fail ignores warnings
	return opt_soft_fail ? 0 : ret;
}


int execute (char *line)
{
	char *argv[MAX_ARGS];
	int   argc = 0;
	char *t;
	char *s;

	memset(argv, 0, sizeof(argv));
	if ( (t = strtok_r(line, SEP_CHARS, &s)) )
		do
			argv[argc++] = t;
		while ( (t = strtok_r(NULL, SEP_CHARS, &s)) && argc < MAX_ARGS );

	return command(argc, argv);
}


int script (FILE *fp, script_hint_f hint_func)
{
	struct config_buffer  line_buff;
	struct config_buffer  hint_buff;
	char                 *line;
	char                 *hint;
	int                   count = 1;
	int                   ret = 0;
	char                 *t;
	char                 *s;

	if ( config_buffer_init(&line_buff, 4096, 4096) ||
	     config_buffer_init(&hint_buff, 4096, 4096) )
		stop("failed to config_buffer_init()");

	while ( ret >= 0 )
	{
		if ( isatty(fileno(fp)) )
			printf("%s:%d> ", argv0, count);

		if ( (ret = config_buffer_line(&line_buff, fp)) < 0 )
		{
			ret++; // -1 for EOF -> 0, -2 for error -> -1
			break;
		}

		// match comments and keep as a "hint"
		if ( (hint = strstr(line_buff.buff, "#")) )
			*hint++ = '\0';
		else if ( (hint = strstr(line_buff.buff, "//")) )
		{
			*hint = '\0';
			hint += 2;
		}

		// if a hint was found, strip leading space
		if ( hint )
			while ( *hint && isspace(*hint) )
				hint++;

		// clean up line, will also find empty (ie comment-only) lines
		for ( line = line_buff.buff; *line && isspace(*line); line++ ) ;

		// holding previous hints and this line has commands:
		// dispatch hint text if function given, always reset
		if ( *line && hint_func )
		{
			if ( hint_buff.used )
			{
				hint_func(hint_buff.buff);
				hint_buff.used = 0;
			}
			if ( hint )
			{
				hint_func(hint);
				hint = NULL;
			}
		}

		// execute commands 
		if ( (t = strtok_r(line, ";", &s)) )
			do
			{
				ret = execute(t);
				count++;
			}
			while ( !ret && (t = strtok_r(NULL, ";", &s)) );

		// this line has a hint with no command: append to buffer; fail on error
		if ( hint && (ret = config_buffer_append(&hint_buff, hint, -1)) < 0 )
			break;
	}

	if ( hint_buff.used && hint_func )
		hint_func(hint_buff.buff);

	config_buffer_done(&hint_buff);
	config_buffer_done(&line_buff);
	return ret;
}


extern int _rl_completion_case_fold;
int interact (FILE *fp)
{
#ifdef ASFE_CTL_USE_READLINE 
	char        prompt[32];
	int         count = 1;
	char       *t;
	char       *s;
	int         ret = 0;
	char       *line;
	char       *buff;

	rl_readline_name                 = argv0;
	rl_attempted_completion_function = asfe_ctl_map_completion;
	_rl_completion_case_fold         = 1;

	while ( ret >= 0 )
	{
		snprintf(prompt, sizeof(prompt), "%s:%d> ", argv0, count);
		if ( !(line = buff = readline(prompt)) )
			break;

		for ( t = line; *t; t++ ) ;
		while ( t > t && isspace(*(t - 1)) )
			t--;
		*t = '\0';
		while ( *line && isspace(*line) )
			line++;
		if ( ! *line )
			continue;

		add_history(line);

		if ( (t = strtok_r(line, ";", &s)) )
			do
			{
				ret = execute(t);
				count++;
			}
			while ( !ret && (t = strtok_r(NULL, ";", &s)) );

		free(buff);
	}

	return ret;
#else
	return script(fp, NULL);
#endif
}


static void path_setup (char *dst, size_t max, const char *leaf)
{
	char  *d = dst;
	char  *e = dst + max;
	char  *root_list[] = { "/media/card", opt_lib_dir, NULL };
	char **root_walk = root_list;

	while ( *root_walk && d < e )
	{
#ifdef BOARD_REV
		d += snprintf(d, e - d, "%s%s/%s/%s", d > dst ? ":" : "",
		              *root_walk, leaf, BOARD_REV);
#endif
		d += snprintf(d, e - d, ":%s/%s", *root_walk, leaf);
		root_walk++;
	}
}


int main (int argc, char **argv)
{
	int ret = 0;
	int opt;

	if ( (argv0 = strrchr(argv[0], '/')) )
		argv0++;
	else
		argv0 = argv[0];

	while ( (opt = getopt(argc, argv, "eig:d:D:")) > -1 )
		switch ( opt )
		{
			case 'e': opt_soft_fail = 1; break;
			case 'i': opt_interact  = 1; break;

/*			case 'g':
				ret = sscanf(optarg, "%d,%d,%d",
				             &opt_dev_gpio[0], &opt_dev_gpio[1], &opt_dev_gpio[2]);
				if ( ret != 3 )
					usage();
				break;
*/
			case 'd': // Specify ASFE_CTL device number
				errno = 0;
				opt_dev_num = strtol(optarg, NULL, 0);
				if ( errno || opt_dev_num < 0 ||
				     opt_dev_num >= (sizeof(dev_node_list)/sizeof(dev_node_list[0])) )
					usage();
				break;

			case 'D': // 
				opt_dev_node = optarg;
				break;

			case 'l': // 
				opt_lib_dir = optarg;
				break;

			default:
				usage();
		}

	char *p;
	if ( ! opt_lib_dir )
		opt_lib_dir = (p = getenv("ASFE_CTL_LIB_DIR")) ? p : LIB_DIR;

	if ( (p = getenv("ASFE_CTL_SCRIPT_PATH")) )
		snprintf(env_script_path, sizeof(env_script_path), "%s", p);
	else
		path_setup(env_script_path, sizeof(env_script_path), "script");

#ifdef SIM_HAL
	asfe_ctl_hal_sim_attach();
#else
	asfe_ctl_hal_linux_attach();
	dev_reopen(opt_dev_num, 0);
#endif

	// if at least one argv is given, search map for function name or try to run as script
	if ( optind < argc )
		ret = command(argc - optind, argv + optind);

	// if no commands in argv, or -i given, run an interactive loop.  
	if ( optind >= argc || opt_interact )
	{
		opt_soft_fail = 1;
		setbuf(stdout, NULL);
#ifdef ASFE_CTL_USE_READLINE
		fprintf(stderr, "Entering Readline interactive mode\n");
#else
		fprintf(stderr, "Entering interactive mode\n");
#endif
		ret = interact(stdin);
		fprintf(stderr, "\nLeaving interactive mode\n");
	}
	
	asfe_test_function();
	asfe_ctl_dev_reopen(0,0);
	UINT16 asfe_data[] = {1,2,3,4,5};

	asfe_ctl_spiwritearray(&asfe_data[0], 5);

	export_active_channels();
	return ret < 0;
}

