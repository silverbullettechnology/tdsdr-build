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
#include <time.h>
#include <ctype.h>
#include <errno.h>
#include <getopt.h>

#include "sd_user.h"
#include "sd_mesg.h"

#include "srio-tool.h"

#define  DEV_NODE "/dev/" SD_USER_DEV_NODE

char *argv0;

struct sd_mesg        *mesg;
struct sd_mesg_mbox   *mbox;
struct sd_mesg_swrite *swrite;
struct sd_mesg_dbell  *dbell;

uint32_t  buff[8192];
int       size;

uint16_t  opt_loc_addr = 2;
uint16_t  opt_rem_addr = 2;

long      opt_mbox_sub        = 0xF; // sub all 4 mboxes
uint16_t  opt_dbell_range[2]  = { 0, 0xFFFF }; // sub all dbells 
uint64_t  opt_swrite_range[2] = { 0, 0x3FFFFFFFF }; // sub all swrites

long      opt_mbox_send    = 0;
long      opt_mbox_letter  = 0;
uint16_t  opt_dbell_send   = 0x5555;
uint64_t  opt_swrite_send  = 0x12340000;

uint32_t  prev_status;


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
	       "R - Reset SRIO core\n"
		   "1 - Send a short SWRITE\n"
		   "2 - Send a DBELL\n"
		   "3 - Send a short MESSAGE\n"
		   "4 - Ping peer with a DBELL\n"
		   "5 - Send a 2-frag MESSAGE (384 bytes)\n"
		   "6 - Send a 3-frag MESSAGE (640 bytes)\n\n");
}


#define STAT_SRIO_LINK_INITIALIZED  0x00000001
#define STAT_SRIO_PORT_INITIALIZED  0x00000002
#define STAT_SRIO_CLOCK_OUT_LOCK    0x00000004
#define STAT_SRIO_MODE_1X           0x00000008
#define STAT_PORT_ERROR             0x00000010
#define STAT_GTRX_NOTINTABLE_OR     0x00000020
#define STAT_GTRX_DISPERR_OR        0x00000040
#define STAT_DEVICE_ID              0xFF000000

static const char *desc_status (uint32_t  reg)
{
	static char buff[256];

	snprintf(buff, sizeof(buff), "bits: { %s%s%s%s%s%s%s} device_id: %02x", 
	         reg & STAT_SRIO_LINK_INITIALIZED ? "LINK "       : "",
	         reg & STAT_SRIO_PORT_INITIALIZED ? "PORT "       : "",
	         reg & STAT_SRIO_CLOCK_OUT_LOCK   ? "CLOCK "      : "",
	         reg & STAT_SRIO_MODE_1X          ? "1X "         : "",
	         reg & STAT_PORT_ERROR            ? "ERROR "      : "",
	         reg & STAT_GTRX_NOTINTABLE_OR    ? "NOTINTABLE " : "",
	         reg & STAT_GTRX_DISPERR_OR       ? "DISPERR "    : "",
	         (reg & STAT_DEVICE_ID) >> 24);

	return buff;
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

//	setbuf(stdout, NULL);

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
	int             mfda;
	int             sel;
	int             ret;

	struct timespec  send_ts, recv_ts;
	uint64_t         send_ns, recv_ns, diff_ns;
	uint32_t         curr_status, diff_status;

	mesg   = (struct sd_mesg *)buff;
	mbox   = &mesg->mesg.mbox;
	swrite = &mesg->mesg.swrite;
	dbell  = &mesg->mesg.dbell;

	menu();
	while ( main_loop )
	{
		FD_ZERO(&rfds);
		FD_SET(0,   &rfds);
		FD_SET(dev, &rfds);
		mfda = dev;

		tv_cur.tv_sec  = 1;
		tv_cur.tv_usec = 990000;
		errno = 0;
		if ( (sel = select(mfda + 1, &rfds, NULL, NULL, &tv_cur)) < 0 )
		{
			if ( errno != EINTR )
				fprintf(stderr, "select: %s: stop\n", strerror(errno));
			break;
		}
		else if ( !sel )
		{
//			printf("Idling\n");
			continue;
		}

		if ( ioctl(dev, SD_USER_IOCG_STATUS, &curr_status) )
			perror("SD_USER_IOCG_STATUS");
		else if ( (diff_status = (curr_status ^ prev_status)) )
		{
			printf("\nSTAT: %08x -> %08x\n", prev_status, curr_status);
			if ( (diff_status & curr_status) )
				printf(" SET: %08x -> %s\n", diff_status & curr_status,
				       desc_status(diff_status & curr_status));
			if ( (diff_status & prev_status) )
				printf(" CLR: %08x -> %s\n", diff_status & prev_status,
				       desc_status(diff_status & prev_status));
			prev_status = curr_status;
		}

		// data from driver to terminal
		if ( FD_ISSET(dev, &rfds) )
		{
			clock_gettime(CLOCK_MONOTONIC, &recv_ts);
			memset(buff, 0, sizeof(buff));
			if ( (size = read(dev, buff, sizeof(buff))) < 0 )
			{
				perror("read() from driver");
				break;
			}
			printf("\nRECV %d bytes: ", size);

			if ( size >= offsetof(struct sd_mesg, mesg) )
			{
				printf("type %d, %02x->%02x hello %08x.%08x: ",
				       mesg->type, mesg->src_addr, mesg->dst_addr,
				       mesg->hello[0], mesg->hello[1]);

				switch ( mesg->type )
				{
					case 6:
						size -= offsetof(struct sd_mesg,        mesg);
						size -= offsetof(struct sd_mesg_swrite, data);
						printf("SWRITE to %09llx, payload %d:\n", swrite->addr, size);
						hexdump_buff(swrite->data, size);
						break;

					case 10:
						switch ( dbell->info )
						{
							case 0xFFF0: // PING req - inc info, return
								dbell->info++;
								mesg->src_addr = opt_loc_addr;
								mesg->dst_addr = opt_rem_addr;
								if ( (ret = write(dev, buff, size)) < size )
								{
									perror("write() to driver");
									main_loop = 0;
								}
								printf("PING req\n");
								break;

							case 0xFFF1: // PING req
								send_ns  = send_ts.tv_sec;
								send_ns *= 1000000000;
								send_ns += send_ts.tv_nsec;

								recv_ns  = recv_ts.tv_sec;
								recv_ns *= 1000000000;
								recv_ns += recv_ts.tv_nsec;

								diff_ns = recv_ns - send_ns;
								printf("PING rsp: %llu.%09llu\n", 
								       diff_ns / 1000000000,
								       diff_ns % 1000000000);
								break;

							default:
								printf("DBELL w/ info %04x\n", dbell->info);
								break;
						}
						break;

					case 11:
						size -= offsetof(struct sd_mesg,      mesg);
						size -= offsetof(struct sd_mesg_mbox, data);
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

			size = 0;
			memset(buff, 0, sizeof(buff));
			mesg->src_addr = opt_loc_addr;
			mesg->dst_addr = opt_rem_addr;
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
					size += offsetof(struct sd_mesg_swrite, data);
					size += offsetof(struct sd_mesg,        mesg);
					break;

				case '2':
					mesg->type = 10;
					dbell->info = opt_dbell_send++;
					size  = sizeof(struct sd_mesg_dbell);
					printf("DBELL w/ info %04x\n", dbell->info);
					size += offsetof(struct sd_mesg, mesg);
					break;

				case '3':
					mesg->type = 11;
					mbox->mbox   = opt_mbox_send;
					mbox->letter = opt_mbox_letter;
					mbox->data[0] = 0x55AA55AA;
					mbox->data[1] = 0x5A5A5A5A;
					size = sizeof(uint32_t) * 2;
					printf("MESSAGE to mbox %d, letter %d, payload %d:\n",
					       opt_mbox_send, opt_mbox_letter, size);
					hexdump_buff(mbox->data, size);
					size += offsetof(struct sd_mesg_mbox, data);
					size += offsetof(struct sd_mesg,      mesg);
					break;

				case '4':
					mesg->type = 10;
					dbell->info = 0xFFF0;
					size  = sizeof(struct sd_mesg_dbell) + 
					        offsetof(struct sd_mesg, mesg);
					printf("DBELL w/ info %04x\n", dbell->info);
					break;

				case '5':
					mesg->type = 11;
					mbox->mbox   = opt_mbox_send;
					mbox->letter = opt_mbox_letter;
					memset(&mbox->data[  0], 0x55, 256);
					memset(&mbox->data[ 64], 0xAA, 128);
					size = 384;
					printf("MESSAGE to mbox %d, letter %d, payload %d:\n",
					       opt_mbox_send, opt_mbox_letter, size);
					hexdump_buff(mbox->data, size);
					size += offsetof(struct sd_mesg_mbox, data);
					size += offsetof(struct sd_mesg,      mesg);
					break;

				case '6':
					mesg->type = 11;
					mbox->mbox   = opt_mbox_send;
					mbox->letter = opt_mbox_letter;
					memset(&mbox->data[  0], 0x55, 256);
					memset(&mbox->data[ 64], 0xAA, 256);
					memset(&mbox->data[128], 0x55, 128);
					size = 640;
					printf("MESSAGE to mbox %d, letter %d, payload %d:\n",
					       opt_mbox_send, opt_mbox_letter, size);
					hexdump_buff(mbox->data, size);
					size += offsetof(struct sd_mesg_mbox, data);
					size += offsetof(struct sd_mesg,      mesg);
					break;

				case 'q':
				case '\033':
					printf("Shutting down\n");
					main_loop = 0;
					break;

				case '\r':
				case '\n':
					printf("\n\n");
					break;

				default:
					menu();

			}
			mesg->size = size;
		}

		// data from buffer to driver
		if ( size )
		{
			clock_gettime(CLOCK_MONOTONIC, &send_ts);
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
