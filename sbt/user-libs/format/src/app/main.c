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

#ifndef min
#define min(a,b) ((a) < (b) ? (a) : (b))
#endif

char   *argv0;
FILE   *debug        = NULL;
long    opt_size_num = -1;
char    opt_size_pre = '\0';
char    opt_size_suf = '\0';
size_t  size         = 0;

enum { IN, OUT, MAX };
struct opts
{
	char                  *file;
	char                  *format;
	struct format_class   *fmt_class;
	struct format_options  fmt_opts;
}
opts[MAX];
const char *dirs[MAX] = { "IN", "OUT" };

struct format_options  default_opts =
{
	.channels = 0,
	.single   = sizeof(uint16_t) * 2,
	.sample   = sizeof(uint16_t) * 4,
	.bits     = 12,
	.packet   = 0,
	.head     = 0,
	.data     = 0,
	.foot     = 0,
	.flags    = 0,
};


static void progress (size_t done, size_t size)
{
	if ( !size )
		fprintf(stderr, "Failed!");
	else if ( !done )
		fprintf(stderr, "  0%%");
	else if ( done >= size )
		fprintf(stderr, "\b\b\b\b100%%\n");
	else
	{
		unsigned long long prog = done;
		prog *= 100;
		prog /= size;
		fprintf(stderr, "\b\b\b\b%3llu%%", prog);
	}
}


static void usage (void)
{
	fprintf(stderr,
	       "Usage: %s [-v] [-s|-S size] [-c num] [in-opts] [in-format:]in-file [out-opts] [out-format:]out-file\n"
	       "\n"
	       "Global options:\n"
	       "-v       Enable debugging messages\n"
	       "-S size  Specify buffer size in samples, K or M suffix allowed\n"
	       "-s size  Specify buffer size to load/save in bytes, K or M suffix allowed\n"
	       "-c num   Set number of channels in buffer (default 2, max 8)\n"
	       "Size is calculated from in-file if not specified; if specified and smaller than\n"
	       "in-file, the data will be truncated, if larger than in-file then in-file will\n"
	       "repeated to fill the buffer.  K/M mean 1E3/1E6 for samples, 2^10/2^20 for bytes\n"
	       "\n"
	       "File options may be given before each filename; out-file options default to the\n"
	       "in-file options for straight format conversion, or may differ to isolate/copy\n"
	       "sample data.\n"
	       "\n"
	       "File options: [-12345678aieq] [-b bits] [-p spec]\n"
	       "-1-8     Operate on numbered channel (default all)\n"
	       "-a       Operate on all channels\n"
	       "-i       Invert I and Q order\n"
	       "-e       Swapp endian on samples\n"
	       "-q       Quiet - do not show progresss\n"
	       "-b bits  Number of significant bits (default 12)\n"
	       "-p spec  Packetize or depacketize according to spec, a set of 4 comma-separated\n"
	       "         values in bytes: packet,head,data,foot\n"
	       "If out-file is '-' then write to stdout.  Note that 1-8, a, i, and e options\n"
	       "reset between input and output, since they modify load/save operations. The\n"
	       "s, S, c, b, and p options are not reset, since they modify the data buffer\n"
	       "and sample data format.\n",
	       argv0);

	printf("in-format and out-format should be one of:\n");
	format_class_list(stdout);

	exit(1);
}

int main (int argc, char **argv)
{
	if ( (argv0 = strrchr(argv[0], '/')) )
		argv0++;
	else
		argv0 = argv[0];

	default_opts.prog_func = progress;
	opts[IN].fmt_opts      = default_opts;

	int dir;
	for ( dir = IN; dir < MAX; dir++ )
	{
		struct opts *cur = &opts[dir];
		char        *ptr;
		int          opt;
		int          ret;

		if ( dir > 0 )
			opts[dir] = opts[dir - 1];
		cur->fmt_opts.channels = 0;
		cur->fmt_opts.flags    = 0;

		optind = 1;
		while ( (opt = getopt(argc, argv, "+vs:S:c:12345678aieqb:p:")) != -1 )
			switch ( opt )
			{
				case 'v':
					debug = stderr;
					format_debug_setup(stderr);
					break;

				case 's':
				case 'S':
					if ( dir > IN )
					{
						fprintf(stderr, "-%c not allowed for output file\n", opt);
						usage();
					}

					opt_size_pre = opt;
					opt_size_num = strtol(optarg, &ptr, 0);
					opt_size_suf = *ptr;
					break;

				case 'c':
					if ( dir > IN )
					{
						fprintf(stderr, "-c not allowed for output file\n");
						usage();
					}

					cur->fmt_opts.sample  = strtoul(optarg, NULL, 0) * cur->fmt_opts.single;
					break;

				case '1':
				case '2':
				case '3':
				case '4':
				case '5':
				case '6':
				case '7':
				case '8':
					cur->fmt_opts.channels |= 1 << (opt - '1');
					break;

				case 'a':
					cur->fmt_opts.channels = (1 << (cur->fmt_opts.sample / cur->fmt_opts.single)) - 1;
					break;

				case 'e':
					cur->fmt_opts.flags |= FO_ENDIAN;
					break;

				case 'i':
					cur->fmt_opts.flags |= FO_IQ_SWAP;
					break;

				case 'q':
					cur->fmt_opts.prog_func = NULL;
					break;

				case 'b':
					cur->fmt_opts.bits = strtoul(optarg, NULL, 0);
					if ( cur->fmt_opts.bits > (cur->fmt_opts.single * 4) )
					{
						fprintf(stderr, "-b: value %zu > possible %zu\n", 
						        cur->fmt_opts.bits, cur->fmt_opts.single * 4);
						usage();
					}
					break;

				case 'p':
					ret = sscanf(optarg, "%zu,%zu,%zu,%zu", &cur->fmt_opts.packet,
					             &cur->fmt_opts.head, &cur->fmt_opts.data,
					             &cur->fmt_opts.foot);
					if ( ret != 4 )
					{
						fprintf(stderr, "-b: malformed: %s\n", strerror(errno));
						usage();
					}
					break;

				default:
					fprintf(stderr, "Option -%c unknown\n", opt);
					usage();
		}

		if ( optind >= argc )
		{
			fprintf(stderr, "Missing a file/format spec\n");
			usage();
		}

		cur->format = argv[optind];
		if ( debug )
			fprintf(stderr, "%s: try to identify '%s'\n", dirs[dir], cur->format);

		if ( (cur->file = strchr(cur->format, ':')) )
		{
			*cur->file++ = '\0';
			cur->fmt_class = format_class_find(cur->format);
		}
		else if ( (cur->fmt_class = format_class_find(cur->format)) )
			cur->file = "-";
		else if ( (cur->fmt_class = format_class_guess(cur->format)) )
			cur->file = cur->format;

		if ( !cur->fmt_class )
		{
			fprintf(stderr, "Couldn't identify format\n");
			usage();
		}

		argv += optind;
		argc -= optind;
	}

	for ( dir = IN; dir < MAX; dir++ )
	{
		struct opts *cur      = &opts[dir];
		unsigned     channels = (1 << (cur->fmt_opts.sample / cur->fmt_opts.single)) - 1;

		if ( debug )
			fprintf(debug, "%s: format '%s' and file '%s'\n", dirs[dir],
			        format_class_name(opts[dir].fmt_class), opts[dir].file);

		if ( !cur->fmt_opts.channels )
			cur->fmt_opts.channels = channels;
		else if ( cur->fmt_opts.channels & ~channels )
			stop("Requested channels don't exist");

		if ( cur->fmt_opts.packet )
		{
			if ( debug )
				fprintf(debug, "%s: packet %zu, head %zu, data %zu, foot %zu\n",
				        dirs[dir], cur->fmt_opts.packet, cur->fmt_opts.head,
				        cur->fmt_opts.data, cur->fmt_opts.foot);

			if ( !cur->fmt_opts.data )
				cur->fmt_opts.data = cur->fmt_opts.packet - (cur->fmt_opts.head +
				                                             cur->fmt_opts.foot);

			size_t need = cur->fmt_opts.head + cur->fmt_opts.data + cur->fmt_opts.foot;
			if ( cur->fmt_opts.packet < need )
				stop("%s: packet options invalid (head %zu + data %zu + foot %zu) "
				     "> packet %zu", 
				     dirs[dir], cur->fmt_opts.head, cur->fmt_opts.data,
				     cur->fmt_opts.foot, cur->fmt_opts.packet);
		}
	}
	
	FILE *in_file;
	if ( !(in_file = fopen(opts[IN].file, "r")) )
		stop("fopen(%s, r)", opts[IN].file);

	if ( opt_size_num > -1 )
	{
		if ( debug )
			fprintf(debug, "User size: -%c%ld%c -> ",
			        opt_size_pre, opt_size_num, opt_size_suf);

		switch ( opt_size_pre )
		{
			case 's': // bytes
				size = opt_size_num;
				switch ( tolower(opt_size_suf) )
				{
					case 'm': size <<= 10; // fall-through
					case 'k': size <<= 10;
				}
				break;

			case 'S': // samples
				size = opt_size_num;
				switch ( tolower(opt_size_suf) )
				{
					case 'm': size *= 1000; // fall-through
					case 'k': size *= 1000;
				}
				size *= opts[IN].fmt_opts.sample;
				break;
		}

		if ( debug )
			fprintf(debug, "%zu bytes\n", size);
	}

	if ( !size )
		size = format_size(opts[IN].fmt_class, &opts[IN].fmt_opts, in_file);
	if ( !size )
		stop("format_%s_size(%s)", format_class_name(opts[IN].fmt_class), opts[IN].file);

	if ( size % opts[IN].fmt_opts.sample )
		fprintf(stderr, "warning: size %zu not a multiple of sample width %zu\n",
		        size, opts[IN].fmt_opts.sample);

	void *buff = malloc(size);
	if ( !buff )
		stop("failed to alloc buffer");
	if ( debug )
		fprintf(debug, "buffer: %zu bytes allocated\n", size);
	
	if ( opts[IN].fmt_opts.packet )
	{
		void   *end  = buff + size;
		void   *ptr  = buff;
		size_t  skip = opts[IN].fmt_opts.head +
		               opts[IN].fmt_opts.data +
		               opts[IN].fmt_opts.foot;

		if ( skip < opts[IN].fmt_opts.packet )
			skip = opts[IN].fmt_opts.packet - skip;
		else
			skip = 0;

		while ( ptr < end )
		{
			memset(ptr, 'H', min(opts[IN].fmt_opts.head, end - ptr));
			ptr += min(opts[IN].fmt_opts.head, end - ptr);

			memset(ptr, 'D', min(opts[IN].fmt_opts.data, end - ptr));
			ptr += min(opts[IN].fmt_opts.data, end - ptr);

			memset(ptr, 'F', min(opts[IN].fmt_opts.foot, end - ptr));
			ptr += min(opts[IN].fmt_opts.foot, end - ptr);

			if ( skip )
			{
				memset(ptr, 'S', min(skip, end - ptr));
				ptr += min(skip, end - ptr);
			}
		}
	}
	else
		memset(buff, 0xFF, size);
	
	if ( opts[IN].fmt_opts.prog_func )
	{
		opts[IN].fmt_opts.prog_step  = size / 100;
		fprintf(stderr, "Reading: ");
	}

	if ( !format_read(opts[IN].fmt_class, &opts[IN].fmt_opts, buff, size, in_file) )
		stop("format_%s_read()", (opts[IN].fmt_class));
	
	fclose(in_file);
	in_file = NULL;

	FILE *out_file;
	if ( !strcmp(opts[OUT].file, "-") )
		out_file = stdout;
	else if ( !(out_file = fopen(opts[OUT].file, "w")) )
		stop("fopen(%s, r)", opts[OUT].file);

	if ( opts[OUT].fmt_opts.prog_func )
	{
		opts[OUT].fmt_opts.prog_step = size / 100;
		fprintf(stderr, "Writing: ");
	}

	if ( !format_write(opts[OUT].fmt_class, &opts[OUT].fmt_opts, buff, size, out_file) )
		stop("format_%s_write()", format_class_name(opts[OUT].fmt_class));
	
	if ( out_file != stdout )
		fclose(out_file);
	out_file = NULL;

	free(buff);
	return 0;
}

