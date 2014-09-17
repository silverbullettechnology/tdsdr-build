/** \file      fifo/adi_new.h
 *  \brief     interface declarations for new ADI DAC/ADC controls
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
 *           Copyright 2013(c) Analog Devices, Inc.
 *
 *           All rights reserved.
 *
 *           Redistribution and use in source and binary forms, with or without
 *           modification, are permitted provided that the following conditions are met:
 *            - Redistributions of source code must retain the above copyright
 *              notice, this list of conditions and the following disclaimer.
 *            - Redistributions in binary form must reproduce the above copyright
 *              notice, this list of conditions and the following disclaimer in
 *              the documentation and/or other materials provided with the
 *              distribution.
 *            - Neither the name of Analog Devices, Inc. nor the names of its
 *              contributors may be used to endorse or promote products derived
 *              from this software without specific prior written permission.
 *            - The use of this software may or may not infringe the patent rights
 *              of one or more patent holders.  This license does not release you
 *              from the requirement that you obtain separate licenses from these
 *              patent holders to use this software.
 *            - Use of the software either in source or binary form, must be run
 *              on or directly connected to an Analog Devices Inc. component.
 *
 *           THIS SOFTWARE IS PROVIDED BY ANALOG DEVICES "AS IS" AND ANY EXPRESS OR
 *           IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, NON-INFRINGEMENT,
 *           MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 *           IN NO EVENT SHALL ANALOG DEVICES BE LIABLE FOR ANY DIRECT, INDIRECT,
 *           INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *           LIMITED TO, INTELLECTUAL PROPERTY RIGHTS, PROCUREMENT OF SUBSTITUTE GOODS OR
 *           SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *           CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 *           OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *           OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef _INCLUDE_FIFO_ADI_NEW_H_
#define _INCLUDE_FIFO_ADI_NEW_H_


#define ADI_NEW_RX 0
#define ADI_NEW_TX 1


/* ADC COMMON */
#define ADI_NEW_RX_REG_PCORE_VER        0x0000
#define ADI_NEW_RX_REG_PCORE_ID         0x0004

#define ADI_NEW_RX_REG_RSTN             0x0040
#define ADI_NEW_RX_RSTN                 (1 << 0)
#define ADI_NEW_RX_MMCM_RSTN            (1 << 1)

#define ADI_NEW_RX_REG_CNTRL            0x0044
#define ADI_NEW_RX_R1_MODE              (1 << 2)
#define ADI_NEW_RX_DDR_EDGESEL          (1 << 1)
#define ADI_NEW_RX_PIN_MODE             (1 << 0)

#define ADI_NEW_RX_REG_STATUS           0x005C
#define ADI_NEW_RX_MUX_PN_ERR           (1 << 3)
#define ADI_NEW_RX_MUX_PN_OOS           (1 << 2)
#define ADI_NEW_RX_MUX_OVER_RANGE       (1 << 1)
#define ADI_NEW_RX_STATUS               (1 << 0)

#define ADI_NEW_RX_REG_DELAY_CNTRL      0x0060
#define ADI_NEW_RX_DELAY_SEL            (1 << 17)
#define ADI_NEW_RX_DELAY_RWN            (1 << 16)
#define ADI_NEW_RX_DELAY_ADDR(x)        (((x) & 0x000000FF) << 8)
#define ADI_NEW_RX_TO_DELAY_ADDR(x)     (((x) >> 8) & 0x000000FF)
#define ADI_NEW_RX_DELAY_WDATA(x)       (((x) & 0x0000001F) << 0)
#define ADI_NEW_RX_TO_DELAY_WDATA(x)    (((x) >> 0) & 0x0000001F)

#define ADI_NEW_RX_REG_DELAY_STATUS     0x0064
#define ADI_NEW_RX_DELAY_LOCKED         (1 << 9)
#define ADI_NEW_RX_DELAY_STATUS         (1 << 8)
#define ADI_NEW_RX_TO_DELAY_RDATA(x)    (((x) >> 0) & 0x0000001F)

#define ADI_NEW_RX_REG_DMA_CNTRL        0x0080
#define ADI_NEW_RX_DMA_STREAM           (1 << 1)
#define ADI_NEW_RX_DMA_START            (1 << 0)

#define ADI_NEW_RX_REG_DMA_COUNT        0x0084
#define ADI_NEW_RX_DMA_COUNT(x)         (((x) & 0xFFFFFFFF) << 0)
#define ADI_NEW_RX_TO_DMA_COUNT(x)      (((x) >> 0) & 0xFFFFFFFF)

#define ADI_NEW_RX_REG_DMA_STATUS       0x0088
#define ADI_NEW_RX_DMA_OVF              (1 << 2)
#define ADI_NEW_RX_DMA_UNF              (1 << 1)
#define ADI_NEW_RX_DMA_STATUS           (1 << 0)

#define ADI_NEW_RX_REG_DMA_BUSWIDTH     0x008C
#define ADI_NEW_RX_DMA_BUSWIDTH(x)      (((x) & 0xFFFFFFFF) << 0)
#define ADI_NEW_RX_TO_DMA_BUSWIDTH(x)   (((x) >> 0) & 0xFFFFFFFF)

/* ADC CHANNEL */
#define ADI_NEW_RX_REG_CHAN_CNTRL(c)    (0x0400 + (c) * 0x40)
#define ADI_NEW_RX_PN_SEL               (1 << 10)
#define ADI_NEW_RX_IQCOR_ENB            (1 << 9)
#define ADI_NEW_RX_DCFILT_ENB           (1 << 8)
#define ADI_NEW_RX_FORMAT_SIGNEXT       (1 << 6)
#define ADI_NEW_RX_FORMAT_TYPE          (1 << 5)
#define ADI_NEW_RX_FORMAT_ENABLE        (1 << 4)
#define ADI_NEW_RX_PN23_TYPE            (1 << 1)
#define ADI_NEW_RX_ENABLE               (1 << 0)

#define ADI_NEW_RX_REG_CHAN_STATUS(c)   (0x0404 + (c) * 0x40)
#define ADI_NEW_RX_PN_ERR               (1 << 2)
#define ADI_NEW_RX_PN_OOS               (1 << 1)
#define ADI_NEW_RX_OVER_RANGE           (1 << 0)

#define ADI_NEW_RX_REG_CHAN_CNTRL_1(c)  (0x0410 + (c) * 0x40)
#define ADI_NEW_RX_DCFILT_OFFSET(x)     (((x) & 0xFFFF) << 16)
#define ADI_NEW_RX_TO_DCFILT_OFFSET(x)  (((x) >> 16) & 0xFFFF)
#define ADI_NEW_RX_DCFILT_COEFF(x)      (((x) & 0xFFFF) << 0)
#define ADI_NEW_RX_TO_DCFILT_COEFF(x)   (((x) >> 0) & 0xFFFF)

#define ADI_NEW_RX_REG_CHAN_CNTRL_2(c)  (0x0414 + (c) * 0x40)
#define ADI_NEW_RX_IQCOR_COEFF_1(x)     (((x) & 0xFFFF) << 16)
#define ADI_NEW_RX_TO_IQCOR_COEFF_1(x)  (((x) >> 16) & 0xFFFF)
#define ADI_NEW_RX_IQCOR_COEFF_2(x)     (((x) & 0xFFFF) << 0)
#define ADI_NEW_RX_TO_IQCOR_COEFF_2(x)  (((x) >> 0) & 0xFFFF)


/* DAC COMMON */
#define ADI_NEW_TX_REG_PCORE_VER                0x0000
#define ADI_NEW_TX_REG_PCORE_ID                 0x0004

#define ADI_NEW_TX_REG_RSTN                     0x0040
#define ADI_NEW_TX_RSTN                         (1 << 0)

#define ADI_NEW_TX_REG_RATECNTRL                0x004C
#define ADI_NEW_TX_RATE(x)                      (((x) & 0xFF) << 0)
#define ADI_NEW_TX_TO_RATE(x)                   (((x) >> 0) & 0xFF)

#define ADI_NEW_TX_REG_CNTRL_1                  0x0044
#define ADI_NEW_TX_ENABLE                       (1 << 0)

#define ADI_NEW_TX_REG_CNTRL_2                  0x0048
#define ADI_NEW_TX_PAR_TYPE                     (1 << 7)
#define ADI_NEW_TX_PAR_ENB                      (1 << 6)
#define ADI_NEW_TX_R1_MODE                      (1 << 5)
#define ADI_NEW_TX_DATA_FORMAT                  (1 << 4)
#define ADI_NEW_TX_DATA_SEL(x)                  (((x) & 0xF) << 0)
#define ADI_NEW_TX_TO_DATA_SEL(x)               (((x) >> 0) & 0xF)

#define ADI_NEW_TX_REG_VDMA_FRMCNT              0x0084
#define ADI_NEW_TX_VDMA_FRMCNT(x)               (((x) & 0xFFFFFFFF) << 0)
#define ADI_NEW_TX_TO_VDMA_FRMCNT(x)            (((x) >> 0) & 0xFFFFFFFF)

#define ADI_NEW_TX_REG_VDMA_STATUS              0x0088
#define ADI_NEW_TX_VDMA_OVF                     (1 << 1)
#define ADI_NEW_TX_VDMA_UNF                     (1 << 0)

enum {
	ADI_NEW_TX_DATA_SEL_DDS,
	ADI_NEW_TX_DATA_SEL_SED,
	ADI_NEW_TX_DATA_SEL_DMA,
};


/* DAC CHANNEL */
#define ADI_NEW_TX_REG_CHAN_CNTRL_1_IIOCHAN(x)  (0x0400 + ((x) >> 1) * 0x40 + ((x) & 1) * 0x8)
#define ADI_NEW_TX_DDS_SCALE(x)                 (((x) & 0xF) << 0)
#define ADI_NEW_TX_TO_DDS_SCALE(x)              (((x) >> 0) & 0xF)

#define ADI_NEW_TX_REG_CHAN_CNTRL_2_IIOCHAN(x)  (0x0404 + ((x) >> 1) * 0x40 + ((x) & 1) * 0x8)
#define ADI_NEW_TX_DDS_INIT(x)                  (((x) & 0xFFFF) << 16)
#define ADI_NEW_TX_TO_DDS_INIT(x)               (((x) >> 16) & 0xFFFF)
#define ADI_NEW_TX_DDS_INCR(x)                  (((x) & 0xFFFF) << 0)
#define ADI_NEW_TX_TO_DDS_INCR(x)               (((x) >> 0) & 0xFFFF)


int fifo_adi_new_read  (int dev, int tx, unsigned long ofs, unsigned long *val);
int fifo_adi_new_write (int dev, int tx, unsigned long ofs, unsigned long  val);


#endif // _INCLUDE_FIFO_ADI_NEW_H_
