/** \file      fifo_dev.v
 *  \brief     Shared-library exports and version control
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
fifo_dev_0.1 {
	global:
		/* src/lib/access.c */
		fifo_access_avail;
		fifo_access_request;
		fifo_access_release;
		/* src/lib/adi_new.c */
		fifo_adi_new_read;
		fifo_adi_new_write;
		/* src/lib/adi_old.c */
		fifo_adi_old_set_ctrl;
		fifo_adi_old_get_ctrl;
		fifo_adi_old_set_tx_cnt;
		fifo_adi_old_get_tx_cnt;
		fifo_adi_old_set_rx_cnt;
		fifo_adi_old_get_rx_cnt;
		fifo_adi_old_get_sum;
		fifo_adi_old_get_last;
		fifo_adi_old_chksum_reset;
		fifo_adi_old_get_fifo_cnt;
		/* src/lib/dsxx.c */
		fifo_dsrc_set_ctrl;
		fifo_dsrc_get_stat;
		fifo_dsrc_set_bytes;
		fifo_dsrc_get_bytes;
		fifo_dsrc_get_sent;
		fifo_dsrc_set_type;
		fifo_dsrc_get_type;
		fifo_dsrc_set_reps;
		fifo_dsrc_get_reps;
		fifo_dsrc_get_rsent;
		fifo_dsnk_set_ctrl;
		fifo_dsnk_get_stat;
		fifo_dsnk_get_bytes;
		fifo_dsnk_get_sum;
		/* src/lib/pmon.c */
		fifo_pmon_read;
		fifo_pmon_write;
		/* src/lib/dev.c */
		fifo_dev_close;
		fifo_dev_reopen;
		fifo_dev_target_desc;
		fifo_dev_target_mask;
	local:
		*;
};
