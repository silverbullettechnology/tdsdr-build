/*
 * Licensed under GPLv2 or later, see file LICENSE in this source tree.
 *  Copyright (C) 2000, Jan-Derk Bakker (J.D.Bakker@its.tudelft.nl)
 *  Copyright (C) 2008, BusyBox Team. -solar 4/26/08
 *
 * Ported from busybox to standalone for SBT SDRDC project
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <ctype.h>
#include <errno.h>
#include <getopt.h>


char *opt_outfile = NULL;
FILE *opt_debug   = NULL;


static void usage (void)
{
	fprintf(stderr, 
	        "Usage: devmem [-o outfile] ADDRESS LENGTH\n\n"
	        "Where:\n"
	        "-v          Debugging messages on stderr\n"
	        "-o outfile  Filename to dump to\n"
	        "ADDRESS     Physical base address\n"
	        "LENGTH      Length in bytes to dump\n"
	        "If -o is not given, data is dumped to stdout\n");
	exit(1);
}


int main (int argc, char **argv)
{
	size_t    target;
	size_t    length;
	size_t    offset;
	size_t    map_size; 
	size_t    map_page;
	void     *map_base;
	void     *virt;
	long      page = sysconf(_SC_PAGESIZE);
	int       opt;
	int       dev;
	int       out;
	int       ret;

	while ( (opt = getopt(argc, argv, "?hvo:")) > -1 )
		switch ( opt )
		{
			case 'v': opt_debug   = stderr; break;
			case 'o': opt_outfile = optarg; break;

			default:
				usage();
		}

	if ( argc - optind < 2 )
		usage();
	target = strtoul(argv[optind    ], NULL, 0);
	length = strtoul(argv[optind + 1], NULL, 0);

	map_page = target & ~(page - 1);   // base of page containing target
	offset   = target &  (page - 1);   // offset of target within map_page
	map_size = (target + length) - map_page;  // top of requested region to map_base
	if ( map_size & (page - 1) )         // round up to full pages
	{
		map_size += (page - 1);
		map_size &= ~(page - 1);
	}

	if ( opt_debug )
		fprintf(opt_debug,
		        "Requested 0x%08zx..0x%08zx: map_page 0x%08zx, map_size 0x%08zx, "
		        "offset 0x%zx\n",
		        target, target + length - 1, map_page, map_size, offset);

	if ( (dev = open("/dev/mem", O_RDONLY|O_SYNC)) < 0 )
	{
		perror("/dev/mem");
		return 1;
	}
	if ( opt_debug )
		fprintf(opt_debug, "/dev/mem opened as %d\n", dev);

	map_base = mmap(NULL, map_size, PROT_READ, MAP_SHARED, dev, map_page);
	if ( map_base == MAP_FAILED )
	{
		perror("mmap");
		close(dev);
		return 1;
	}
	if ( opt_debug )
		fprintf(opt_debug, "map_base mapped as %p\n", map_base);

	if ( !opt_outfile ) 
		out = 1;
	else if ( (out = open(opt_outfile, O_WRONLY|O_CREAT|O_TRUNC, 0644)) < 0 )
	{
		perror(opt_outfile);
		munmap(map_base, map_size);
		close(dev);
		return 1;
	}
	if ( opt_debug )
		fprintf(opt_debug, "%s opened as %d\n", opt_outfile ? opt_outfile : "stdout", out);

	virt = map_base + offset;
	while ( length >= 4096 )
	{
		if ( (ret = write(out, virt, 4096)) < 0 )
		{
			perror("write");
			munmap(map_base, map_size);
			close(dev);
			return 1;
		}
		if ( opt_debug )
			fprintf(opt_debug, "full: virt -> %p, length %zu -> 4096, ret %d\n",
			        virt, length, ret);

		virt   += ret;
		length -= ret;
	}
	if ( length )
	{
		if ( (ret = write(out, virt, length)) < 0 )
		{
			perror("write");
			munmap(map_base, map_size);
			close(dev);
			return 1;
		}
		if ( opt_debug )
			fprintf(opt_debug, "part: virt -> %p, length %zu, ret %d\n",
			        virt, length, ret);
	}

	errno = 0;
	if ( out != 1 && close(out) )
		perror("close(out)");

	if ( munmap(map_base, map_size) )
		perror("munmap");

	if ( close(dev) )
		perror("close(dev)");

	if ( opt_debug && !errno )
		fprintf(opt_debug, "unmapped, closed, return success\n");

	return 0;
}
