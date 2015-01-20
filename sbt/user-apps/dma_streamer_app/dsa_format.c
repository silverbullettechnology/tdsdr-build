/** \file      dsa_format.c
 *  \brief     I/O formatters
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
#include <unistd.h>
#include <stdint.h>
#include <sys/stat.h>
#include <ctype.h>
#include <math.h>
#include <errno.h>

#ifdef UNIT_TEST
#define DSM_BUS_WIDTH 8
#define DSM_MAX_SIZE  840000000
#else
#include <dma_streamer_mod.h>
#endif

#include "dsa_format.h"
#include "dsa_channel.h"

#include "log.h"
LOG_MODULE_STATIC("format", LOG_LEVEL_INFO);


static void spin (void)
{
	static int idx = 0;
	static char chars[4] = { '/', '-', '\\', '|' };
	fputc('\r', stderr);
	fputc(chars[idx++ & 3], stderr);
}


static long fmt_bin_size (FILE *fp, int chan)
{
	if ( fp == stdin )
	{
		LOG_DEBUG("fp is stdin, can't size\n");
		errno = ENOTTY;
		return -1;
	}

	struct stat sb;
	if ( fstat(fileno(fp), &sb) )
		return -1;

	LOG_DEBUG("fp is %ld bytes\n", (long)sb.st_size);
	return sb.st_size;
}

static int fmt_bin_read (FILE *fp, void *buff, size_t size, int chan, int lsh)
{
	long    page = sysconf(_SC_PAGESIZE);
	int     file = 0;
	int     offs = 0;
	size_t  want;
	size_t  left = size;
	char   *dst = buff;
	int     ret;
	int     every = 0;

	if ( page < 1024 )
	{
		page = 1024;
		LOG_DEBUG("page adjusted to %ld\n", page);
	}

	if ( fp != stdin )
	{
		if ( (file = fseek(fp, 0, SEEK_END)) < 0 )
			return -1;

		LOG_DEBUG("file is %d bytes\n", file);
		fseek(fp, 0, SEEK_SET);
	}

	while ( left )
	{
		if ( (want = page) > left )
			want = left;
		if ( file && want > (file - offs) )
			want = file - offs;
		LOG_DEBUG("page %ld, file %d, offs %d -> want %zu\n", page, file, offs, want);

		if ( (ret = fread(dst, 1, want, fp)) < 0 )
			return -1;

		// EOF handling
		if ( ret == 0 )
		{
			// more buffer to fill from file: rewind to 0 and repeat
			if ( file && left > 0 )
			{
				LOG_DEBUG("file > left, rewind and reload\n");
				fseek(fp, 0, SEEK_SET);
				offs = 0;
				continue;
			}
			// stdin or when done: return success
			else
				return 0;
		}

		if ( (every++ & 0xfff) == 0xfff )
			spin();
		LOG_DEBUG("read block of %d at %d, %zu left\n", ret, offs, left - ret);
		dst  += ret;
		left -= ret;
		offs += ret;
	}

	if ( lsh )
	{
		uint16_t *walk = (uint16_t *)buff;
		for ( left = size; left; left -= sizeof(uint16_t) )
		{
			if ( *walk )
				*walk <<= 4;
			walk++;
		}
	}

	fputc('\r', stderr);
	LOG_DEBUG("success.\n");
	return 0;
}

static int fmt_bin_write (FILE *fp, void *buff, size_t size, int chan)
{
	char  *dst  = buff;
	long   page = sysconf(_SC_PAGESIZE);
	int    ret;
	int    every = 0;

	while ( size )
	{
		if ( page > size )
			page = size;

		if ( (ret = fwrite(dst, 1, page, fp)) < 0 )
			return -1;

		if ( (every++ & 0xfff) == 0xfff )
			spin();
		LOG_DEBUG("wrote block of %d, %zu left\n", ret, size - ret);
		dst  += ret;
		size -= ret;
	}

	fputc('\r', stderr);
	return 0;
}


void hexdump_line (FILE *fp, const unsigned char *ptr, int len)
{
	int i;
	for ( i = 0; i < len; i++ )
		fprintf (fp, "%02x ", ptr[i]);

	for ( ; i < 16; i++ )
		fputs("   ", fp);

	for ( i = 0; i < len; i++ )
		fputc(isprint(ptr[i]) ? ptr[i] : '.', fp);
	fputc('\n', fp);
}
 
void hexdump_buff (FILE *fp, const void *buf, int len)
{
	const unsigned char *ptr = buf;
	while ( len >= 16 )
	{
		hexdump_line(fp, ptr, 16);
		ptr += 16;
		len -= 16;
	}
	if ( len )
		hexdump_line(fp, ptr, len);
}

static int fmt_hex_write (FILE *fp, void *buff, size_t size, int chan)
{
	hexdump_buff(fp, buff, size);
	return 0;
}


void smpdump (FILE *fp, void *buff, size_t size)
{
	uint16_t *s = buff;
	long      v;
	int       i;

	size /= sizeof(uint16_t);
	while ( size )
	{
		for ( i = 0; i < 4; i++ )
		{
			v = s[i];
			fprintf(fp, "%03lx:%c%c%c%c%c%c%c%c%c%c%c%c%c",
			        v, 
			        v & 0x800 ? '1' : '0',
			        v & 0x400 ? '1' : '0',
			        v & 0x200 ? '1' : '0',
			        v & 0x100 ? '1' : '0',
			        v & 0x080 ? '1' : '0',
			        v & 0x040 ? '1' : '0',
			        v & 0x020 ? '1' : '0',
			        v & 0x010 ? '1' : '0',
			        v & 0x008 ? '1' : '0',
			        v & 0x004 ? '1' : '0',
			        v & 0x002 ? '1' : '0',
			        v & 0x001 ? '1' : '0',
			        (i == 3) ? '\n' : '\t');
		}
		s += 4;
		size -= 4;
	}

	fprintf(fp, "\n");
}

static int fmt_bit_write (FILE *fp, void *buff, size_t size, int chan)
{
	smpdump(fp, buff, size);
	return 0;
}


static long fmt_bist_size (FILE *fp, int chan)
{
	char  buf[256];
	long  val;
	long  cnt = 0;

	if ( fp != stdin )
		fseek(fp, 0, SEEK_SET);

	while ( fgets(buf, sizeof(buf), fp) )
	{
		errno = 0;
		val = strtoul(buf, NULL, 16);
		if ( errno || val > 0xFFFF )
			return -1;
		cnt++;
	}

	return cnt * sizeof(uint16_t);
}

static int fmt_bist_read (FILE *fp, void *buff, size_t size, int chan, int lsh)
{
	char      buf[256];
	long      val;
	long      cnt = 0;
	char     *prompts[4] = { "TX1_I", "TX1_Q", "TX2_I", "TX2_Q" };
	uint16_t *d = buff;

	if ( fp == stdin )
		printf("%s[%ld]: ", prompts[0], cnt);
	else
		fseek(fp, 0, SEEK_SET);

	size /= sizeof(uint16_t);
	while ( size-- && fgets(buf, sizeof(buf), fp) )
	{
		errno = 0;
		val = strtoul(buf, NULL, 16);
		if ( errno || val > 0xFFF )
			return -1;

		*d = (uint16_t)val & 0xFFF;
		LOG_DEBUG("v %03lx -> d[%ld] %03x\n", val, cnt & 3, *d);
		if ( lsh )
			*d <<= 4;
		d++;

		cnt++;
		if ( fp == stdin )
			printf("%s[%ld]: ", prompts[cnt & 3], cnt >> 2);
	}

	return cnt * sizeof(uint16_t);
}


static int fmt_bist_write (FILE *fp, void *buff, size_t size, int chan)
{
	uint8_t *ptr = buff;
	size_t   idx = 0;

	while ( idx < size )
	{
		if ( !(idx & 7) )
			fprintf(fp, "\nValue[%4zu]: ", idx);
		fprintf(fp, "%02x ", ptr[idx]);
		idx++;
	}

	fprintf(fp, "\n");
	return 0;
}


static int fmt_null_write (FILE *fp, void *buff, size_t size, int chan)
{
	return 0;
}


static long fmt_dec_size (FILE *fp, int chan)
{
	char  buf[256];
	long  cnt = 0;

	if ( fp != stdin )
		fseek(fp, 0, SEEK_SET);

	while ( fgets(buf, sizeof(buf), fp) )
		cnt += 4;

	return cnt * sizeof(uint16_t);
}

static int fmt_dec_read (FILE *fp, void *buff, size_t size, int chan, int lsh)
{
	char      l[256];
	char     *p;
	int       i;
	long      v;
	uint16_t *d = buff;

	if ( fp != stdin )
		fseek(fp, 0, SEEK_SET);

	size /= DSM_BUS_WIDTH;
	while ( size-- && fgets(l, sizeof(l), fp) )
	{
		LOG_DEBUG("line: %s", l);
		d[0] = d[1] = d[2] = d[3] = 0;

		p = l;
		for ( i = 0; i < 4; i++ )
		{
			errno = 0;
			v = strtol(p, &p, 10);
			if ( errno || v < -2048 || v > 2047 )
				return -1;

			d[i] = (uint16_t)v & 0x7FF;
			if ( v < 0 )
				d[i] |= 0x800;
			LOG_DEBUG("v %ld -> d[%d] %03x\n", v, i, d[i]);
			if ( lsh )
				d[i] <<= 4;
		}

		d += 4;
	}

	return 0;
}

static int fmt_dec_write (FILE *fp, void *buff, size_t size, int chan)
{
	uint16_t *s = buff;
	long      v;
	int       i;

	size /= DSM_BUS_WIDTH;
	while ( size-- )
	{
		for ( i = 0; i < 4; i++ )
		{
			v = s[i] & 0x7FF;
			if ( s[i] & 0x800 )
				v -= 0x800;
			fprintf(fp, "%6ld%c", v, (i == 3) ? '\n' : '\t');
		}
		s += 4;
	}

	return 0;
}


static long fmt_iqw_data_size (uint32_t head)
{
	long data = head;
	data /= 2;
	data *= DSM_BUS_WIDTH;
	return data;
}

static long fmt_iqw_size (FILE *fp, int chan)
{
	struct stat  sb;
	uint32_t     head;
	off_t        file;
	long         ret;

	fseek(fp, 0, SEEK_SET);
	if ( (ret = fread(&head, 1, sizeof(head), fp)) < sizeof(head) )
		return -1;

	// TODO: endian swap head if necessary
	if ( head & 1 )
		return -1;
	
	file  = head;
	file *= sizeof(float);
	file += sizeof(uint32_t);

	if ( fstat(fileno(fp), &sb) || sb.st_size != file )
		return -2;
	
	ret = fmt_iqw_data_size(head);
	LOG_DEBUG("head %08x -> %lu bytes\n", head, ret);
	return ret;
}

static int fmt_iqw_read (FILE *fp, void *buff, size_t size, int chan, int lsh)
{
	int       i;
	float     f;
	int16_t   s;
	uint16_t *d = buff;
	size_t    p;
	int       c = 0;
	uint32_t  every = 0;

	if ( fp != stdin )
		fseek(fp, sizeof(uint32_t), SEEK_SET);
	else
	{
		uint32_t head;
		if ( fread(&head, 1, sizeof(head), fp) < sizeof(head) )
			return -1;
		// TODO: endian swap head if necessary
		if ( size != fmt_iqw_data_size(head) )
		{
			fprintf(stderr, "iqw: file has data size %ld, buffer size %zu, stop\n",
			        size, fmt_iqw_data_size(head));
			errno = EINVAL;
			return -1;
		}
	}

	// two passes through data: first I, then Q.  Use i=0 for I, i=1 for Q
	for ( i = 0; i < 2; i++ )
	{
		LOG_DEBUG("start pass %d\n", i);
		d = buff;
		p = size / DSM_BUS_WIDTH;
		while ( p-- )
		{
			if ( fread(&f, 1, sizeof(f), fp) < sizeof(f) )
				return -1;
			// TODO: endian swap f if necessary

			s = roundf(f * 2048);
			if ( s > 2047 )
			{
				s = 2047;
				c++;
			}
			else if ( s < -2048 )
			{
				s = -2048;
				c++;
			}

			// store in channel 1
			if ( chan & DC_CHAN_1 )
			{
				d[i] = (unsigned long)s & 0x7FF;
				if ( s < 0 )
					d[i] |= 0x800;
				if ( lsh )
					d[i] <<= 4;
			}

			// store in channel 2
			if ( chan & DC_CHAN_2 )
			{
				d[i + 2] = (unsigned long)s & 0x7FF;
				if ( s < 0 )
					d[i + 2] |= 0x800;
				if ( lsh )
					d[i + 2] <<= 4;
			}
			
			d += DSM_BUS_WIDTH / sizeof(uint16_t);

			if ( (every++ & 0x1ffff) == 0x1ffff )
				spin();
		}
		LOG_DEBUG("done pass %d\n", i);
	}


	if ( c )
		LOG_DEBUG("warning: %d samples clipped on input\n", c);

	fputc('\r', stderr);
	return 0;
}

static int fmt_iqw_write (FILE *fp, void *buff, size_t size, int chan)
{
	uint32_t  head = size;
	int       i;
	float     f;
	int16_t   s;
	uint16_t *d = buff;
	size_t    p;
	uint32_t  every = 0;

	head /= DSM_BUS_WIDTH;
	head *= 2;
	if ( fwrite(&head, 1, sizeof(head), fp) < sizeof(head) )
		return -1;

	// two passes through data: first I, then Q.  Use i=0 for I, i=1 for Q
	for ( i = 0; i < 2; i++ )
	{
		LOG_DEBUG("start pass %d\n", i);
		d = buff;
		p = size / DSM_BUS_WIDTH;
		if ( chan & DC_CHAN_2 )
			d += 2;
		while ( p-- )
		{
			s = d[i] & 0x7FF;
			if ( d[i] & 0x800 )
				s -= 0x800;

			f  = s;
			f /= 2048.0;
			// TODO: endian swap f if necessary

			if ( fwrite(&f, 1, sizeof(f), fp) < sizeof(f) )
				return -1;

			d += DSM_BUS_WIDTH / sizeof(uint16_t);
			if ( (every++ & 0x1ffff) == 0x1ffff )
				spin();
		}
		LOG_DEBUG("done pass %d\n", i);
	}

	fputc('\r', stderr);
	return 0;
}


static struct format format_list[] =
{
	{ "bin",   "",  fmt_bin_size,   fmt_bin_read,   fmt_bin_write   },
	{ "hex",   "",  NULL,           NULL,           fmt_hex_write   },
	{ "bist",  "",  fmt_bist_size,  fmt_bist_read,  fmt_bist_write  },
	{ "null",  "",  NULL,           NULL,           fmt_null_write  },
	{ "dec",   "",  fmt_dec_size,   fmt_dec_read,   fmt_dec_write   },
	{ "iqw",   "",  fmt_iqw_size,   fmt_iqw_read,   fmt_iqw_write   },
	{ "bit",   "",  NULL,           NULL,           fmt_bit_write  },
	{ NULL }
};

struct format *format_find (const char *name)
{
	struct format *fmt;

	for ( fmt = format_list; fmt->name; fmt++ )
		if ( !strcasecmp(fmt->name, name) )
			return fmt;

	return NULL;
}

// Future: guess format from filename extension?
struct format *format_guess (const char *name)
{
	struct format *fmt;
	char          *ptr;

	if ( !(ptr = strrchr(name, '.')) )
		return NULL;

	ptr++;
	for ( fmt = format_list; fmt->name; fmt++ )
		if ( !strcasecmp(fmt->name, ptr) )
			return fmt;
		//TODO: search the exts[] array

	return NULL;
}


void dsa_format_list(FILE *fp)
{
	static struct format *fmt;
	for ( fmt = format_list; fmt->name; fmt++ )
	{
		fprintf(fp, "  %-7s  ", fmt->name);
		if ( fmt->read && fmt->write )
			fprintf(fp, "in/out\n");
		else if ( fmt->read )
			fprintf(fp, "in-only\n");
		else
			fprintf(fp, "out-only\n");
	}
}

#ifdef UNIT_TEST
#include "dsa_common.h"

char *argv0;
char *opt_in_file    = NULL;
char *opt_in_format  = NULL;
char *opt_out_file   = NULL;
char *opt_out_format = NULL;
int   opt_size       = 0;
int   opt_chan       = 0;
int   opt_lsh        = 0;

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
	dsa_format_list(stdout);

	return 1;
}

int main (int argc, char **argv)
{
	log_dupe(stdout);
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
				log_set_global_level(LOG_LEVEL_DEBUG);
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

	struct format *in_format;
	opt_in_format = argv[optind];
	if ( (opt_in_file = strchr(opt_in_format, ':')) )
	{
		*opt_in_file++ = '\0';
		in_format = format_find(opt_in_format);
	}
	else if ( (in_format = format_find(opt_in_format)) )
		opt_in_file = "-";
	else if ( (in_format = format_guess(opt_in_format)) )
		opt_in_file = opt_in_format;
	else
		return usage();

	if ( !in_format || !in_format->read )
		return usage();

	if ( (argc - optind) > 1 )
		opt_out_format = argv[optind + 1];
	else
	{
		opt_out_format = opt_in_format;
		opt_out_file = "-";
	}

	struct format *out_format;
	if ( (opt_out_file = strchr(opt_out_format, ':')) )
	{
		*opt_out_file++ = '\0';
		out_format = format_find(opt_out_format);
	}
	else if ( (out_format = format_find(opt_out_format)) )
		opt_out_file = "-";
	else if ( (out_format = format_guess(opt_out_format)) )
		opt_out_file = opt_out_format;
	else
		return usage();

	if ( !out_format || !out_format->write )
		return usage();

	LOG_DEBUG("opt_in_format '%s' and opt_in_file '%s'\n", in_format->name, opt_in_file);
	LOG_DEBUG("opt_out_format '%s' and opt_out_file '%s'\n", out_format->name, opt_out_file);
	
	FILE *in_file;
	if ( !strcmp(opt_in_file, "-") )
		in_file = stdin;
	else if ( !(in_file = fopen(opt_in_file, "r")) )
		stop("fopen(%s, r)", opt_in_file);

	if ( !opt_size )
	{
		if ( !strcmp(opt_in_file, "-") )
			stop("Specify size with -s when using stdin");
		if ( !in_format->size )
			stop("Specify size with -s when using %s", opt_in_format);

		if ( (opt_size = format_size(in_format, in_file, opt_chan)) < 1 )
			stop("format_%s_size(%s)", opt_in_format, opt_in_file);
	}

	if ( opt_size > DSM_MAX_SIZE )
		stop("size %zu exceeds maximum %u", opt_size, DSM_MAX_SIZE);

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
#endif
