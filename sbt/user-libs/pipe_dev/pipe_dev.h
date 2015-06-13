/** \file      pipe_dev.h
 *  \brief     interface declarations for pipe-dev IOCTLs
 *  \copyright Copyright 2015 Silver Bullet Technology
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
#ifndef _INCLUDE_PIPE_DEV_H_
#define _INCLUDE_PIPE_DEV_H_
#include <pd_main.h>


/******* Device handle open/close functions *******/


int  pipe_dev_reopen (const char *node);
void pipe_dev_close  (void);


/******* Access request / release functions *******/


int pipe_access_avail   (unsigned long *bits);
int pipe_access_request (unsigned long  bits);
int pipe_access_release (unsigned long  bits);


/******* ADI2AXIS IOCTLs *******/


int pipe_adi2axis_set_ctrl  (int dev, unsigned long  reg);
int pipe_adi2axis_get_ctrl  (int dev, unsigned long *reg);
int pipe_adi2axis_get_stat  (int dev, unsigned long *reg);
int pipe_adi2axis_set_bytes (int dev, unsigned long  reg);
int pipe_adi2axis_get_bytes (int dev, unsigned long *reg);


/******* ROUTING_REG IOCTLs *******/


int pipe_routing_reg_get_adc_sw_dest (unsigned long *reg);
int pipe_routing_reg_set_adc_sw_dest (unsigned long  reg);


/******* SWRITE_PACK IOCTLs *******/


int pipe_swrite_pack_set_cmd     (int dev, unsigned long  reg);
int pipe_swrite_pack_get_cmd     (int dev, unsigned long *reg);
int pipe_swrite_pack_set_addr    (int dev, unsigned long  reg);
int pipe_swrite_pack_get_addr    (int dev, unsigned long *reg);
int pipe_swrite_pack_set_srcdest (int dev, unsigned long  reg);
int pipe_swrite_pack_get_srcdest (int dev, unsigned long *reg);


/******* SWRITE_UNPACK IOCTLs *******/


int pipe_swrite_unpack_set_cmd  (unsigned long  reg);
int pipe_swrite_unpack_get_cmd  (unsigned long *reg);
int pipe_swrite_unpack_set_addr (int dev, unsigned long  reg);
int pipe_swrite_unpack_get_addr (int dev, unsigned long *reg);


/******* VITA49_ASSEM IOCTLs *******/


int pipe_vita49_assem_set_cmd     (int dev, unsigned long reg);
int pipe_vita49_assem_get_cmd     (int dev, unsigned long *reg);
int pipe_vita49_assem_set_hdr_err (int dev, unsigned long reg);
int pipe_vita49_assem_get_hdr_err (int dev, unsigned long *reg);


/******* VITA49_CLK IOCTLs *******/


int pipe_vita49_clk_set_ctrl (unsigned long  reg);
int pipe_vita49_clk_get_ctrl (unsigned long *reg);
int pipe_vita49_clk_get_stat (unsigned long *reg);
int pipe_vita49_clk_set_tsi  (unsigned long  reg);
int pipe_vita49_clk_get_tsi  (unsigned long *reg);
int pipe_vita49_clk_read     (int dev, struct pd_vita49_ts *ts);


/******* VITA49_PACK IOCTLs *******/


int pipe_vita49_pack_set_ctrl     (int dev, unsigned long  reg);
int pipe_vita49_pack_get_ctrl     (int dev, unsigned long *reg);
int pipe_vita49_pack_get_stat     (int dev, unsigned long *reg);
int pipe_vita49_pack_set_streamid (int dev, unsigned long  reg);
int pipe_vita49_pack_get_streamid (int dev, unsigned long *reg);
int pipe_vita49_pack_set_pkt_size (int dev, unsigned long  reg);
int pipe_vita49_pack_get_pkt_size (int dev, unsigned long *reg);
int pipe_vita49_pack_set_trailer  (int dev, unsigned long  reg);
int pipe_vita49_pack_get_trailer  (int dev, unsigned long *reg);


/******* VITA49_TRIG IOCTLs *******/


int pipe_vita49_trig_adc_set_ctrl (int dev, unsigned long  reg);
int pipe_vita49_trig_adc_get_ctrl (int dev, unsigned long *reg);
int pipe_vita49_trig_adc_get_stat (int dev, unsigned long *reg);
int pipe_vita49_trig_adc_set_ts   (int dev, struct pd_vita49_ts *buf);
int pipe_vita49_trig_adc_get_ts   (int dev, struct pd_vita49_ts *buf);

int pipe_vita49_trig_dac_set_ctrl (int dev, unsigned long  reg);
int pipe_vita49_trig_dac_get_ctrl (int dev, unsigned long *reg);
int pipe_vita49_trig_dac_get_stat (int dev, unsigned long *reg);
int pipe_vita49_trig_dac_set_ts   (int dev, struct pd_vita49_ts *buf);
int pipe_vita49_trig_dac_get_ts   (int dev, struct pd_vita49_ts *buf);


/******* VITA49_UNPACK IOCTLs *******/


int pipe_vita49_unpack_set_ctrl    (int dev, unsigned long  reg);
int pipe_vita49_unpack_get_ctrl    (int dev, unsigned long *reg);
int pipe_vita49_unpack_get_stat    (int dev, unsigned long *reg);
int pipe_vita49_unpack_set_strm_id (int dev, unsigned long  reg);
int pipe_vita49_unpack_get_strm_id (int dev, unsigned long *reg);
int pipe_vita49_unpack_get_counts  (int dev, struct pd_vita49_unpack *buf);


/******* SRIO_DMA_COMB IOCTLs *******/


int pipe_srio_dma_comb_set_cmd   (unsigned long  reg);
int pipe_srio_dma_comb_get_cmd   (unsigned long *reg);
int pipe_srio_dma_comb_get_stat  (unsigned long *reg);
int pipe_srio_dma_comb_set_npkts (unsigned long  reg);
int pipe_srio_dma_comb_get_npkts (unsigned long *reg);


/******* SRIO_DMA_COMB IOCTLs *******/


int pipe_srio_dma_comb_set_cmd   (unsigned long  reg);
int pipe_srio_dma_comb_get_cmd   (unsigned long *reg);
int pipe_srio_dma_comb_get_stat  (unsigned long *reg);
int pipe_srio_dma_comb_set_npkts (unsigned long  reg);
int pipe_srio_dma_comb_get_npkts (unsigned long *reg);
int pipe_srio_dma_comb_get_tuser (unsigned long *reg);


#endif // _INCLUDE_PIPE_DEV_H_
