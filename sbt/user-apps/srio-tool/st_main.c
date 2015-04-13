/** SRIO test tool
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
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <ctype.h>
#include <errno.h>
#include <getopt.h>

#include "sd_user.h"

#include "srio-tool.h"

#define  DEV_NODE "/dev/" SD_USER_DEV_NODE

char *argv0;

struct sd_user_mesg        *mesg;
struct sd_user_mesg_mbox   *mbox;
struct sd_user_mesg_swrite *swrite;
struct sd_user_mesg_dbell  *dbell;

uint32_t  buff[1024];
int       size;

uint16_t  opt_loc_addr = 2;
uint16_t  opt_rem_addr = 2;

long      opt_mbox_sub        = 0xF; // sub all 4 mboxes
uint16_t  opt_dbell_range[2]  = { 0, 0xFFFF }; // sub all dbells 
uint64_t  opt_swrite_range[2] = { 0, 0x3FFFFFFFF }; // sub all swrites

long      opt_mbox_send    = 0;
uint16_t  opt_dbell_send   = 0x1000;
uint64_t  opt_swrite_send  = 0x12340000;


/** control for main loop for orderly shutdown */
static int main_loop = 1;
static void signal_fatal (int signum)
{
	main_loop = 0;
}


static void hexdump_line (const unsigned char *ptr, const unsigned char *org, int len)
{
	char  buff[80];
	char *p = buff;
	int   i;

	p += sprintf (p, "%04x: ", (unsigned)(ptr - org));

	for ( i = 0; i < len; i++ )
		p += sprintf (p, "%02x ", ptr[i]);

	for ( ; i < 16; i++ )
	{
		*p++ = ' ';
		*p++ = ' ';
		*p++ = ' ';
	}

	for ( i = 0; i < len; i++ )
		*p++ = isprint(ptr[i]) ? ptr[i] : '.';
	*p = '\0';

	printf("%s\n", buff);
}

static void hexdump_buff (const void *buf, int len)
{
	const unsigned char *org = buf;
	const unsigned char *ptr = buf;

	while ( len >= 16 )
	{
		hexdump_line(ptr, org, 16);
		ptr += 16;
		len -= 16;
	}
	if ( len )
		hexdump_line(ptr, org, len);
}


void usage (void)
{
	printf("Usage: %s [-L devid] [-R devid]\n"
	       "Where:\n"
	       "-L devid  Set local device-ID\n"
	       "-R devid  Set remote device-ID\n",
	       argv0);

	exit(1);
}


void menu (void)
{
	printf("Commands understood:\n"
	       "Q - exit\n"
		   "1 - Send a short SWRITE\n"
		   "2 - Send a DBELL\n\n");
}


int main (int argc, char **argv)
{
	argv0 = argv[0];

	int opt;
	while ( (opt = getopt(argc, argv, "L:R:")) > -1 )
		switch ( opt )
		{
			case 'L':
				opt_loc_addr = strtoul(optarg, NULL, 0);
				break;

			case 'R':
				opt_rem_addr = strtoul(optarg, NULL, 0);
				break;

			default:
				usage();
				break;
		}

	setbuf(stdout, NULL);

	int dev = open(DEV_NODE, O_RDWR|O_NONBLOCK);
	if ( dev < 0 )
	{
		perror(DEV_NODE);
		return 1;
	}

	signal(SIGTERM, signal_fatal);
	signal(SIGINT,  signal_fatal);

	st_term_setup();

	if ( ioctl(dev, SD_USER_IOCS_LOC_DEV_ID, opt_loc_addr) )
		perror("SD_USER_IOCS_LOC_DEV_ID");

	if ( ioctl(dev, SD_USER_IOCS_MBOX_SUB, opt_mbox_sub) )
		perror("SD_USER_IOCS_MBOX_SUB");

	if ( ioctl(dev, SD_USER_IOCS_DBELL_SUB, opt_dbell_range) )
		perror("SD_USER_IOCS_DBELL_SUB");

	if ( ioctl(dev, SD_USER_IOCS_SWRITE_SUB, opt_swrite_range) )
		perror("SD_USER_IOCG_SWRITE_SUB");


	/* Standard set of vars to support select */
	struct timeval  tv_cur;
	fd_set          rfds;
	fd_set          wfds;
	int             mfda;
	int             sel;
	int             ret;

	mesg   = (struct sd_user_mesg *)buff;
	mbox   = &mesg->mesg.mbox;
	swrite = &mesg->mesg.swrite;
	dbell  = &mesg->mesg.dbell;

	menu();
	while ( main_loop )
	{
		FD_ZERO(&rfds);
		FD_ZERO(&wfds);
		FD_SET(0,   &rfds);
		FD_SET(dev, &rfds);
		FD_SET(dev, &wfds);
		mfda = dev;

		tv_cur.tv_sec  = 5;
		tv_cur.tv_usec = 0;
		errno = 0;
		if ( (sel = select(mfda + 1, &rfds, &wfds, NULL, &tv_cur)) < 0 )
		{
			if ( errno != EINTR )
				fprintf(stderr, "select: %s: stop\n", strerror(errno));
			break;
		}
		else if ( !sel )
		{
			printf("Idling\n");
			continue;
		}

		// data from driver to terminal
		if ( FD_ISSET(dev, &rfds) )
		{
			memset(buff, 0, sizeof(buff));
			if ( (size = read(dev, buff, sizeof(buff))) < 0 )
			{
				perror("read() from driver");
				break;
			}
			printf("\nRECV raw buffer:\n");
			hexdump_buff(buff, size);
			printf("\nRECV %d bytes: ", size);

			if ( size >= offsetof(struct sd_user_mesg, mesg) )
			{
				printf("type %d, %02x->%02x hello %08x.%08x: ",
				       mesg->type, mesg->src_addr, mesg->dst_addr,
				       mesg->hello[0], mesg->hello[1]);

				switch ( mesg->type )
				{
					case 6:
						size -= offsetof(struct sd_user_mesg,        mesg);
						size -= offsetof(struct sd_user_mesg_swrite, data);
						printf("SWRITE to %09llx, payload %d:\n", swrite->addr, size);
						hexdump_buff(swrite->data, size);
						break;

					case 10:
						printf("DBELL w/ info %04x\n", dbell->info);
						break;

					case 11:
						size -= offsetof(struct sd_user_mesg,      mesg);
						size -= offsetof(struct sd_user_mesg_mbox, data);
						printf("MESSAGE, mbox %d, letter %d, payload %d:\n",
						       mbox->mbox, mbox->letter, size);
						hexdump_buff(mbox->data, size);
						break;

					default:
						printf("type %d invalid\n", mesg->type);
				}
			}
			else
			{
				printf("too short, buffer:\n");
				hexdump_buff(buff, sizeof(buff));
			}

			size = 0;
		}

		// data from terminal to buffer
		if ( FD_ISSET(0, &rfds) )
		{
			char  key;
			if ( read(0, &key, sizeof(key)) < 1 )
			{
				perror("read() from terminal");
				break;
			}
			printf("SEND: key '%c': ", key);

			size = 0;
			memset(buff, 0, sizeof(buff));
			switch ( tolower(key) )
			{
				case '1':
					mesg->type = 6;
					swrite->addr    = opt_swrite_send;
					swrite->data[0] = 0x12345678;
					swrite->data[1] = 0x87654321;
					size = sizeof(uint32_t) * 2;
					printf("SWRITE to %09llx, payload %d:\n", opt_swrite_send, size);
					hexdump_buff(swrite->data, size);
					size += offsetof(struct sd_user_mesg_swrite, data);
					size += offsetof(struct sd_user_mesg, mesg);
					break;

				case '2':
					mesg->type = 10;
					dbell->info = opt_dbell_send++;
					size  = sizeof(struct sd_user_mesg_dbell);
					printf("DBELL w/ info %04x\n", dbell->info);
					size += offsetof(struct sd_user_mesg, mesg);
					break;

				case 'q':
				case '\033':
					printf("Shutting down\n");
					main_loop = 0;
					break;

				default:
					menu();

			}

			if ( size )
			{
				mesg->size     = size;
				mesg->src_addr = opt_loc_addr;
				mesg->dst_addr = opt_rem_addr;
				printf("\nSEND raw buffer, %d bytes:\n", size);
				hexdump_buff(buff, size);
			}
		}

		// data from buffer to driver
		if ( size && FD_ISSET(dev, &wfds) )
		{
			if ( (ret = write(dev, buff, size)) < size )
			{
				perror("write() to driver");
				break;
			}
			size = 0;
		}
		else if ( size )
			printf("Driver write blocked\n");
	}

	st_term_cleanup();

	close(dev);

	return 0;
}

/* Ends    : src/tool/test-stub.c */
