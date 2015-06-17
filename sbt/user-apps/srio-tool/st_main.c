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
#include "common.h"

#define  DEV_NODE "/dev/" SD_USER_DEV_NODE
#define  PERIOD 5

char *argv0;

struct sd_mesg        *mesg;
struct sd_mesg_mbox   *mbox;
struct sd_mesg_swrite *swrite;
struct sd_mesg_dbell  *dbell;

uint32_t  buff[8192];
int       size;

uint16_t  opt_loc_addr = 0xFFFF;
uint16_t  opt_rem_addr = 2;

long      opt_mbox_sub        = 0xF; // sub all 4 mboxes
uint16_t  opt_dbell_range[2]  = { 0, 0xFFFF }; // sub all dbells 
uint64_t  opt_swrite_range[2] = { 0, 0x3FFFFFFFF }; // sub all swrites

long      opt_mbox_send    = 0;
long      opt_mbox_letter  = 0;
uint16_t  opt_dbell_send   = 0x5555;
uint64_t  opt_swrite_send  = 0x12340000;

uint32_t  prev_status;
time_t    periodic = -1;


uint8_t  v49_disco_mesg[] = 
{
	0x59,0x00,0x00,0x0a,0x00,0x00,0x00,0x00,0x00,0x11,0x22,0x33,0x00,0x01,0x00,0x01,
	0x00,0x00,0x00,0x00,0x40,0x00,0x00,0x00,0x67,0x45,0x8b,0xc6,0x23,0x7b,0x69,0x98,
	0x3c,0x73,0x48,0x33,0x51,0xdc,0xb0,0xff,
};


/** control for main loop for orderly shutdown */
static int main_loop = 1;
static void signal_fatal (int signum)
{
	main_loop = 0;
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
	printf("\nCommands understood:\n"
	       "Q - exit\n"
	       "R - Reset SRIO core\n"
	       "P - Toggle Periodic mode\n"
	       "G - Get RX settings\n"
	       "A/S/D/F - Adjust RX0/1/2/3 CM_TRIM (cap +, lower -)\n"
	       "Z/X/C/V - Adjust RX0/1/2/3 CM_SEL (AVTT, GND, Float, Adjust)\n"
	       "1 - Send a short SWRITE\n"
	       "2 - Send a DBELL\n"
	       "3 - Send a short MESSAGE\n"
	       "4 - Ping peer with a DBELL\n"
	       "5 - Send a 2-frag MESSAGE (384 bytes)\n"
	       "6 - Send a 3-frag MESSAGE (640 bytes)\n"
	       "7 - Send a Vita49 DISCO message (40 bytes)\n"
	       "8 - Send a burst of 2 SWRITEs, 256 bytes each\n"
	       "9 - Send a burst of 4 SWRITEs, 256 bytes each\n"
	       "0 - Send a burst of 8 SWRITEs, 256 bytes each\n"
	       "\n");
}


#define STAT_SRIO_LINK_INITIALIZED  0x00000001
#define STAT_SRIO_PORT_INITIALIZED  0x00000002
#define STAT_SRIO_CLOCK_OUT_LOCK    0x00000004
#define STAT_SRIO_MODE_1X           0x00000008
#define STAT_PORT_ERROR             0x00000010
#define STAT_GTRX_NOTINTABLE_OR     0x00000020
#define STAT_GTRX_DISPERR_OR        0x00000040
#define STAT_PHY_RCVD_MCE           0x00000100
#define STAT_PHY_RCVD_LINK_RESET    0x00000200
#define STAT_DEVICE_ID              0xFFFF0000

static const char *desc_status (uint32_t  reg)
{
	static char buff[256];

	snprintf(buff, sizeof(buff), "bits: { %s%s%s%s%s%s%s%s%s} device_id: %02x", 
	         reg & STAT_SRIO_LINK_INITIALIZED ? "LINK "       : "",
	         reg & STAT_SRIO_PORT_INITIALIZED ? "PORT "       : "",
	         reg & STAT_SRIO_CLOCK_OUT_LOCK   ? "CLOCK "      : "",
	         reg & STAT_SRIO_MODE_1X          ? "1X "         : "",
	         reg & STAT_PORT_ERROR            ? "ERROR "      : "",
	         reg & STAT_GTRX_NOTINTABLE_OR    ? "NOTINTABLE " : "",
	         reg & STAT_GTRX_DISPERR_OR       ? "DISPERR "    : "",
	         reg & STAT_PHY_RCVD_MCE          ? "PHY_RCVD_MCE " : "",
	         reg & STAT_PHY_RCVD_LINK_RESET   ? "PHY_RCVD_LINK_RESET " : "",
	         (reg & STAT_DEVICE_ID) >> 16);

	return buff;
}

static const char *cm_ctrl_sel (unsigned sel)
{
	switch ( sel )
	{
		case 0: return "AVTT";
		case 1: return "GND";
		case 2: return "Float";
		case 3: return "Adjust";
	}
	return "???";
}

static unsigned cm_ctrl_trim (unsigned sel)
{
	switch ( sel )
	{
		case  0: return  100;
		case  1: return  200;
		case  2: return  250;
		case  3: return  300;
		case  4: return  350;
		case  5: return  400;
		case  6: return  500;
		case  7: return  550;
		case  8: return  600;
		case  9: return  700;
		case 10: return  800;
		case 11: return  850;
		case 12: return  900;
		case 13: return  950;
		case 14: return 1000;
		case 15: return 1100;
	}
	return 0;
}

static void cm_ctrl_print (const struct sd_user_cm_ctrl *cm_ctrl, const char *pref)
{
	printf("%sCH %u sel %u (%s) trim %u (%umV)\n", pref, cm_ctrl->ch,
	       cm_ctrl->sel,  cm_ctrl_sel(cm_ctrl->sel),
	       cm_ctrl->trim, cm_ctrl_trim(cm_ctrl->trim));
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

	if ( opt_loc_addr < 0xFFFF && ioctl(dev, SD_USER_IOCS_LOC_DEV_ID, opt_loc_addr) )
		perror("SD_USER_IOCS_LOC_DEV_ID");

	if ( opt_loc_addr == 0xFFFF && ioctl(dev, SD_USER_IOCG_LOC_DEV_ID, &opt_loc_addr) )
		perror("SD_USER_IOCG_LOC_DEV_ID");
	printf("Local SRIO device-ID is 0x%04x\n", opt_loc_addr);

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

	/* RX CM_CTRL buffer */
	struct sd_user_cm_ctrl cm_ctrl;
	int                    swrite_burst;

	struct timespec  send_ts, recv_ts;
	uint64_t         send_ns, recv_ns, diff_ns;
	uint32_t         curr_status, diff_status;
	unsigned         diff_count = 0;
	time_t           diff_reset = time(NULL) + 15;
	char             repeat = 0;

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

		tv_cur.tv_sec  = 0;
		tv_cur.tv_usec = 125000;
		errno = 0;
		if ( (sel = select(mfda + 1, &rfds, NULL, NULL, &tv_cur)) < 0 )
		{
			if ( errno != EINTR )
				fprintf(stderr, "select: %s: stop\n", strerror(errno));
			break;
		}

		if ( ioctl(dev, SD_USER_IOCG_STATUS, &curr_status) )
			perror("SD_USER_IOCG_STATUS");
		else if ( (diff_status = (curr_status ^ prev_status)) )
		{
			if ( diff_count++ < 5 )
			{
				printf("\nSTAT: %08x -> %08x\n", prev_status, curr_status);
				if ( (diff_status & curr_status) )
					printf(" SET: %08x -> %s\n", diff_status & curr_status,
					       desc_status(diff_status & curr_status));
				if ( (diff_status & prev_status) )
					printf(" CLR: %08x -> %s\n", diff_status & prev_status,
					       desc_status(diff_status & prev_status));
			}
			prev_status = curr_status;
		}

		if ( !sel )
		{
			time_t now = time(NULL);
			if ( now >= diff_reset )
			{
				if ( diff_count >= 5 )
					printf("%u lines of status change suppressed\n", diff_count);
				diff_count = 0;
				diff_reset = now + 15;
			}
			if ( periodic > 0 && periodic <= now )
				periodic = now + PERIOD;
			else
				continue;
		}

		// data from driver to terminal
		if ( FD_ISSET(dev, &rfds) )
		{
			memset(buff, 0xEE, sizeof(buff));
			clock_gettime(CLOCK_MONOTONIC, &recv_ts);
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
						hexdump_buff(mesg, 512);
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
		char  key = '\0';
		if ( FD_ISSET(0, &rfds) )
		{
			if ( read(0, &key, sizeof(key)) < 1 )
			{
				perror("read() from terminal");
				break;
			}
			repeat = key;
		}
		else if ( !sel && periodic > 0 && repeat )
			key = repeat;

		if ( key > '\0' )
		{
			size = 0;
			memset(buff, 0, sizeof(buff));
			mesg->src_addr = opt_loc_addr;
			mesg->dst_addr = opt_rem_addr;
			cm_ctrl.ch = 0;
			swrite_burst = 2;
			switch ( tolower(key) )
			{
				case '1':
					mesg->type = 6;
					swrite->addr    = opt_swrite_send;
					swrite->data[0] = 0x12345678;
					swrite->data[1] = 0x87654321;
					size = sizeof(uint32_t) * 2;
					printf("\nSEND: SWRITE to %09llx, payload %d:\n", opt_swrite_send, size);
					hexdump_buff(swrite->data, size);
					size += offsetof(struct sd_mesg_swrite, data);
					size += offsetof(struct sd_mesg,        mesg);
					break;

				case '2':
					mesg->type = 10;
					dbell->info = opt_dbell_send++;
					size  = sizeof(struct sd_mesg_dbell);
					printf("\nSEND: DBELL w/ info %04x\n", dbell->info);
					size += offsetof(struct sd_mesg, mesg);
					break;

				case '3':
					mesg->type = 11;
					mbox->mbox   = opt_mbox_send;
					mbox->letter = opt_mbox_letter;
					mbox->data[0] = 0x11111111;
					mbox->data[1] = 0x22222222;
					mbox->data[2] = 0x33333333;
					mbox->data[3] = 0x44444444;
					mbox->data[4] = 0x55555555;
					mbox->data[5] = 0x66666666;
					size = sizeof(uint32_t) * 6;
					printf("\nSEND: MESSAGE to mbox %d, letter %d, payload %d:\n",
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
					printf("\nSEND: DBELL w/ info %04x\n", dbell->info);
					break;

				case '5':
					mesg->type = 11;
					mbox->mbox   = opt_mbox_send;
					mbox->letter = opt_mbox_letter;
					memset(&mbox->data[  0], 0x55, 256);
					memset(&mbox->data[ 64], 0xAA,  60);
					size = 316;
					printf("\nSEND: MESSAGE to mbox %d, letter %d, payload %d:\n",
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
					memset(&mbox->data[128], 0x55,  11);
					size = 523;
					printf("\nSEND: MESSAGE to mbox %d, letter %d, payload %d:\n",
					       opt_mbox_send, opt_mbox_letter, size);
					hexdump_buff(mbox->data, size);
					size += offsetof(struct sd_mesg_mbox, data);
					size += offsetof(struct sd_mesg,      mesg);
					break;

				case '7':
					mesg->type = 11;
					mbox->mbox   = opt_mbox_send;
					mbox->letter = opt_mbox_letter;
					memcpy(mbox->data, v49_disco_mesg, sizeof(v49_disco_mesg));
					size = sizeof(v49_disco_mesg);
					printf("\nSEND: DISCO to mbox %d, payload %d:\n",
					       opt_mbox_send, size);
					hexdump_buff(mbox->data, size);
					size += offsetof(struct sd_mesg_mbox, data);
					size += offsetof(struct sd_mesg,      mesg);
					break;

				// note: depends on swrite_burst being reset to 2 before switch
				case '0':
					swrite_burst <<= 1; // fall-through
				case '9':
					swrite_burst <<= 1; // fall-through
				case '8':
				{
					int        slot, byte;
					uint16_t   paint;
					uint16_t  *word;
					for ( slot = 0; slot < swrite_burst; slot++ )
					{
						mesg   = (struct sd_mesg *)&buff[slot * 128];
						swrite = &mesg->mesg.swrite;

						mesg->type     = 6;
						mesg->size     = 256;
						mesg->size    += offsetof(struct sd_mesg,        mesg);
						mesg->size    += offsetof(struct sd_mesg_swrite, data);
						mesg->src_addr = opt_loc_addr;
						mesg->dst_addr = opt_rem_addr;
						swrite->addr   = 0;

						word  = (uint16_t *)swrite->data;
						paint = slot << 8;
						for ( byte = 0; byte < 128; byte++ )
							*word++ = paint++;
					}

					size = mesg->size;
					for ( slot = 0; slot < swrite_burst; slot++ )
						if ( (ret = write(dev, &buff[slot * 128], size)) < size )
							perror("write() to driver");
					size = 0;
					break;
				}


				case 'p':
					if ( periodic < 0 )
					{
						periodic = time(NULL) + PERIOD;
						printf("\nPeriodic mode enabled: commands will repeat every %d sec\n",
						       PERIOD);
						repeat = '\0';
					}
					else
					{
						periodic = -1;
						printf("\nPeriodic mode disabled\n");
					}
					break;

				case 'r':
					printf("\nRESET SRIO core...\n");
					if ( ioctl(dev, SD_USER_IOCS_SRIO_RESET, 0) )
						perror("SD_USER_IOCS_SRIO_RESET");
					break;

				case 'g':
					printf("RX settings:\n");
					for ( cm_ctrl.ch = 0; cm_ctrl.ch < 4; cm_ctrl.ch++ )
						if ( ioctl(dev, SD_USER_IOCG_CM_CTRL, &cm_ctrl) )
							perror("SD_USER_IOCG_CM_CTRL");
						else
							cm_ctrl_print(&cm_ctrl, "  ");
					break;

				// note: depends on cm_ctrl.ch being reset to zero before switch
				case 'f': cm_ctrl.ch++; // fall-through
				case 'd': cm_ctrl.ch++; // fall-through
				case 's': cm_ctrl.ch++; // fall-through
				case 'a':
					if ( ioctl(dev, SD_USER_IOCG_CM_CTRL, &cm_ctrl) )
						perror("SD_USER_IOCG_CM_CTRL");

					if ( cm_ctrl.sel != 3 )
						cm_ctrl.sel = 3;

					if ( isupper(key) && cm_ctrl.trim < 15 )
						cm_ctrl.trim++;
					else if ( islower(key) && cm_ctrl.trim > 0 )
						cm_ctrl.trim--;
					else
						break;

					if ( ioctl(dev, SD_USER_IOCS_CM_CTRL, &cm_ctrl) )
						perror("SD_USER_IOCS_CM_CTRL");
					else
						cm_ctrl_print(&cm_ctrl, "SET: ");
					break;

				// note: depends on cm_ctrl.ch being reset to zero before switch
				case 'v': cm_ctrl.ch++; // fall-through
				case 'c': cm_ctrl.ch++; // fall-through
				case 'x': cm_ctrl.ch++; // fall-through
				case 'z':
					if ( ioctl(dev, SD_USER_IOCG_CM_CTRL, &cm_ctrl) )
						perror("SD_USER_IOCG_CM_CTRL");

					cm_ctrl.sel++;
					cm_ctrl.sel &= 3;

					if ( ioctl(dev, SD_USER_IOCS_CM_CTRL, &cm_ctrl) )
						perror("SD_USER_IOCS_CM_CTRL");
					else
						cm_ctrl_print(&cm_ctrl, "SET: ");
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
		}

		// data from buffer to driver
		if ( size )
		{
			mesg->size = size;
			clock_gettime(CLOCK_MONOTONIC, &send_ts);
			if ( (ret = write(dev, buff, size)) < size )
			{
				perror("write() to driver");
				break;
			}
			size = 0;
		}
	}

	st_term_cleanup();

	close(dev);

	return 0;
}

/* Ends    : src/tool/test-stub.c */
