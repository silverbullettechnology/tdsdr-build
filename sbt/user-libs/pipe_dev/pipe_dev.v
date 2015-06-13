/** \file      pipe_dev.v
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
pipe_dev_0.1 {
	global:
		/* src/lib/access.c */
		pipe_access_avail;
		pipe_access_request;
		pipe_access_release;
		/* src/lib/adi2axis.c */
		pipe_adi2axis_set_ctrl;
		pipe_adi2axis_get_ctrl;
		pipe_adi2axis_get_stat;
		pipe_adi2axis_set_bytes;
		pipe_adi2axis_get_bytes;
		/* src/lib/dev.c */
		pipe_dev_close;
		pipe_dev_reopen;
		/* src/lib/routing_reg.c */
		pipe_routing_reg_get_adc_sw_dest;
		pipe_routing_reg_set_adc_sw_dest;
		/* src/lib/swrite_pack.c */
		pipe_swrite_pack_set_cmd;
		pipe_swrite_pack_get_cmd;
		pipe_swrite_pack_set_addr;
		pipe_swrite_pack_get_addr;
		pipe_swrite_pack_set_srcdest;
		pipe_swrite_pack_get_srcdest;
		/* src/lib/swrite_unpack.c */
		pipe_swrite_unpack_set_cmd;
		pipe_swrite_unpack_get_cmd;
		pipe_swrite_unpack_set_addr;
		pipe_swrite_unpack_get_addr;
		/* src/lib/vita49_assem.c */
		pipe_vita49_assem_set_cmd;
		pipe_vita49_assem_get_cmd;
		pipe_vita49_assem_set_hdr_err;
		pipe_vita49_assem_get_hdr_err;
		/* src/lib/vita49_clk.c */
		pipe_vita49_clk_set_ctrl;
		pipe_vita49_clk_get_ctrl;
		pipe_vita49_clk_get_stat;
		pipe_vita49_clk_set_tsi;
		pipe_vita49_clk_get_tsi;
		pipe_vita49_clk_read;
		/* src/lib/vita49_pack.c */
		pipe_vita49_pack_set_ctrl;
		pipe_vita49_pack_get_ctrl;
		pipe_vita49_pack_get_stat;
		pipe_vita49_pack_set_streamid;
		pipe_vita49_pack_get_streamid;
		pipe_vita49_pack_set_pkt_size;
		pipe_vita49_pack_get_pkt_size;
		pipe_vita49_pack_set_trailer;
		pipe_vita49_pack_get_trailer;
		/* src/lib/vita49_trig_adc.c */
		pipe_vita49_trig_adc_set_ctrl;
		pipe_vita49_trig_adc_get_ctrl;
		pipe_vita49_trig_adc_get_stat;
		pipe_vita49_trig_adc_set_ts;
		pipe_vita49_trig_adc_get_ts;
		/* src/lib/vita49_trig_dac.c */
		pipe_vita49_trig_dac_set_ctrl;
		pipe_vita49_trig_dac_get_ctrl;
		pipe_vita49_trig_dac_get_stat;
		pipe_vita49_trig_dac_set_ts;
		pipe_vita49_trig_dac_get_ts;
		/* src/lib/vita49_unpack.c */
		pipe_vita49_unpack_set_ctrl;
		pipe_vita49_unpack_get_ctrl;
		pipe_vita49_unpack_get_stat;
		pipe_vita49_unpack_set_strm_id;
		pipe_vita49_unpack_get_strm_id;
		pipe_vita49_unpack_get_counts;
		/* src/lib/srio_dma_comb.c */
		pipe_srio_dma_comb_set_cmd;
		pipe_srio_dma_comb_get_cmd;
		pipe_srio_dma_comb_get_stat;
		pipe_srio_dma_comb_set_npkts;
		pipe_srio_dma_comb_get_npkts;
		/* src/lib/srio_dma_comb.c */
		pipe_srio_dma_comb_set_cmd;
		pipe_srio_dma_comb_get_cmd;
		pipe_srio_dma_comb_get_stat;
		pipe_srio_dma_comb_set_npkts;
		pipe_srio_dma_comb_get_npkts;
		pipe_srio_dma_comb_get_tuser;
	local:
		*;
};
