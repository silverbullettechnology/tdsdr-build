/** \file      dsa_main.h
 *  \brief     application global declarations and defaults
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
#ifndef _INCLUDE_DSA_MAIN_H_
#define _INCLUDE_DSA_MAIN_H_
#include <dma_streamer_mod.h>
#include <fd_main.h>
#include "dsa/channel.h"

#define DEF_SIZE         1048576
#define DEF_REPS         1
#define DEF_DSM_DEVICE   "/dev/" DSM_DRIVER_NODE
#define DEF_FIFO_DEVICE  "/dev/" FD_DRIVER_NODE
#define DEF_FORMAT       "iqw"

#define VOL_QUIET    0
#define VOL_NORMAL   1
#define VOL_VERBOSE  2

extern const char *dsa_argv0;
extern int         dsa_adi_new;
extern int         dsa_dsxx;

extern size_t      dsa_opt_len;
extern unsigned    dsa_opt_timeout;
extern const char *dsa_opt_dsm_dev;
extern const char *dsa_opt_fifo_dev;
extern long        dsa_opt_adjust;
extern size_t      dsa_total_words;

extern char  env_data_path[];

extern struct format            *dsa_opt_format;
extern struct dsa_channel_event  dsa_evt;

extern unsigned long  dsa_dsm_rx_channels[];
extern unsigned long  dsa_dsm_tx_channels[];

void dsa_main_show_stats (const struct dsm_xfer_stats *st, int slot);
void dsa_main_show_fifos (const struct fd_fifo_counts *buff);
void dsa_main_dev_close (void);
int dsa_main_dev_reopen (void);

int dsa_main_map   (int reps);
int dsa_main_unmap (void);

#endif // _INCLUDE_DSA_MAIN_MOD_H_
