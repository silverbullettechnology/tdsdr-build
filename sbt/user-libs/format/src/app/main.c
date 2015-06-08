/** \file      src/app/main.c
 *  \brief     I/O formatters - unit test and conversion tool
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
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/stat.h>
#include <ctype.h>
#include <math.h>
#include <errno.h>

#include <format.h>
#include <common.h>

// temporary
#define DC_CHAN_1      0x10
#define DC_CHAN_2      0x20
#define DSM_BUS_WIDTH  8

char *argv0;
char *opt_in_file    = NULL;
char *opt_in_format  = NULL;
char *opt_out_file   = NULL;
char *opt_out_format = NULL;
int   opt_size       = 0;
int   opt_chan       = 0;
int   opt_lsh        = 0;
int   opt_verbose    = 0;

static int usage (void)
{
	printf("Usage: %s [-12lv] [-s size] in-format[:in-file] out-format[:out-file]\n"
	       "Where:\n"
	       "-1       For single-channel formats like .iqw, use only channel 1\n"
	       "-2       For single-channel formats like .iqw, use only channel 2\n"
	       "-l       Left-shift loaded data 4 bits for new ADI PL\n"
	       "-v       Verbose debugging messages\n"
	       "-s size  When reading stdin, specify the buffer size.\n"
	       "\n"
	       "If in-file is not given or '-' then read from stdin\n"
	       "If out-file is not given or '-' then write to stdout\n",
	       argv0);

	printf("in-format and out-format should be one of:\n");
	format_class_list(stdout);

	return 1;
}

int main (int argc, char **argv)
{
	setbuf(stdout, NULL);

	if ( (argv0 = strrchr(argv[0], '/')) )
		argv0++;
	else
		argv0 = argv[0];

	int opt;
	while ( (opt = getopt(argc, argv, "12vs:S:")) != -1 )
		switch ( opt )
		{
			case '1':
				opt_chan |= DC_CHAN_1;
				break;

			case '2':
				opt_chan |= DC_CHAN_2;
				break;

			case 'l':
				opt_lsh = 1;
				break;

			case 'v':
				opt_verbose = 1;
				format_debug_setup(stderr);
				break;

			case 's':
				errno = 0;
				if ( (opt_size = size_bin(optarg)) < 0 )
					return usage();

			case 'S':
				errno = 0;
				if ( (opt_size = size_dec(optarg)) < 0 )
					return usage();
				opt_size /= DSM_BUS_WIDTH;
				break;
		}
	if ( !opt_chan )
		opt_chan = DC_CHAN_1|DC_CHAN_2;

	if ( (argc - optind) < 1 )
		return usage();

	struct format_class *in_format;
	opt_in_format = argv[optind];
	if ( (opt_in_file = strchr(opt_in_format, ':')) )
	{
		*opt_in_file++ = '\0';
		in_format = format_class_find(opt_in_format);
	}
	else if ( (in_format = format_class_find(opt_in_format)) )
		opt_in_file = "-";
	else if ( (in_format = format_class_guess(opt_in_format)) )
		opt_in_file = opt_in_format;
	else
		return usage();

	if ( !in_format )
		return usage();

	if ( (argc - optind) > 1 )
		opt_out_format = argv[optind + 1];
	else
	{
		opt_out_format = opt_in_format;
		opt_out_file = "-";
	}

	struct format_class *out_format;
	if ( (opt_out_file = strchr(opt_out_format, ':')) )
	{
		*opt_out_file++ = '\0';
		out_format = format_class_find(opt_out_format);
	}
	else if ( (out_format = format_class_find(opt_out_format)) )
		opt_out_file = "-";
	else if ( (out_format = format_class_guess(opt_out_format)) )
		opt_out_file = opt_out_format;
	else
		return usage();

	if ( !out_format )
		return usage();

	if ( opt_verbose )
	{
		fprintf(stderr, "opt_in_format '%s' and opt_in_file '%s'\n",
		        format_class_name(in_format), opt_in_file);
		fprintf(stderr, "opt_out_format '%s' and opt_out_file '%s'\n",
		        format_class_name(out_format), opt_out_file);
	}
	
	FILE *in_file;
	if ( !strcmp(opt_in_file, "-") )
		in_file = stdin;
	else if ( !(in_file = fopen(opt_in_file, "r")) )
		stop("fopen(%s, r)", opt_in_file);

	if ( !opt_size )
	{
//		if ( !strcmp(opt_in_file, "-") )
//			stop("Specify size with -s when using stdin");
//		if ( !in_format->size )
//			stop("Specify size with -s when using %s", opt_in_format);

		if ( (opt_size = format_size(in_format, in_file, opt_chan)) < 1 )
			stop("format_%s_size(%s)", opt_in_format, opt_in_file);
	}

//	if ( opt_size > DSM_MAX_SIZE )
//		stop("size %zu exceeds maximum %u", opt_size, DSM_MAX_SIZE);

	if ( opt_size % DSM_BUS_WIDTH )
		stop("size %zu not a multiple of width %u", opt_size, DSM_BUS_WIDTH);

	void *buff = calloc(opt_size, 1);
	if ( !buff )
		stop("failed to alloc buffer");

	if ( format_read(in_format, in_file, buff, opt_size, opt_chan, opt_lsh) < 0 )
		stop("format_%s_read()", opt_in_format);
	
	if ( in_file != stdin )
		fclose(in_file);
	in_file = NULL;

	FILE *out_file;
	if ( !strcmp(opt_out_file, "-") )
		out_file = stdout;
	else if ( !(out_file = fopen(opt_out_file, "w")) )
		stop("fopen(%s, r)", opt_out_file);

	if ( format_write(out_format, out_file, buff, opt_size, opt_chan) < 0 )
		stop("format_%s_write()", opt_out_format);
	
	if ( out_file != stdout )
		fclose(out_file);
	out_file = NULL;

	free(buff);
	return 0;
}

