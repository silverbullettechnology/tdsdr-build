/** \file      include/app/ad9361/profile.h
 *  \brief     
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
#ifndef _INCLUDE_APP_AD9361_PROFILE_H_
#define _INCLUDE_APP_AD9361_PROFILE_H_

#include <ad9361/api_types.h>


typedef struct 
{
	UINT8  RHB3;
	UINT8  RHB2;
	UINT8  RHB1;
	UINT8  RFIR;
	UINT8  THB3;
	UINT8  THB2;
	UINT8  THB1;
	UINT8  TFIR;
	UINT8  rxDecimation;
	UINT8  txInterpolation;
	UINT8  rxFIRGain;
	UINT8  txFIRGain;
	UINT8  RX_Already_run;
	UINT8  TX_Already_run;

	UINT8  BBPLL_scaler;
	UINT8  BBPLL_divider;
	UINT8  BBPLL_diver_exp;
	UINT8  dacClkHalved;
	UINT8  adcClkError;

	UINT8  calFinish69;
	UINT8  calFinish70;

	UINT8  TxFilterConfig;
	UINT8  RxFilterConfig;
	UINT8  ParallelPortConfigString[3];
	UINT8  ensmMode;
	UINT8  calSequnceState;
	UINT8  ensmState;
	UINT8  productID;
	UINT8  rssiSymbolUpper;
	UINT8  rssiSymbolLsb;
	UINT8  rssiPreambleUpper;
	UINT8  rssiPreambleLsb;
	UINT8  rxGain;
	UINT8  rxLpfGain;
	UINT8  rxDigGain;
	UINT8  rxFastAttackState;
	UINT8  rxSlowLoopState;

	UINT16 ch1TxAttenuation;
	UINT8  ch1TxUpdate;
	UINT16 ch2TxAttenuation;
	UINT8  txUpdate;

	UINT8  ch1GainIndex;
	UINT8  ch1LMTGainIndex;
	UINT8  ch1LPFGainIndex;
	UINT8  ch1DigitalGain;
	UINT8  ch2GainIndex;
	UINT8  ch2LMTGainIndex;
	UINT8  ch2LPFGainIndex;
	UINT8  ch2DigitalGain;

	FLOAT  RX_RF_BW;
	FLOAT  TX_RF_BW;
	DOUBLE Refclk_freq;
	DOUBLE BBPLL_freq;
	DOUBLE TX_LO_Freq;
	DOUBLE RX_LO_Freq;

	FLOAT  ADC_CLK;
	FLOAT  DAC_CLK;

	UINT8  txReadAlcWord;
	UINT8  txReadVcoTune;
	UINT8  rxReadAlcWord;
	UINT8  rxReadVcoTune;

	FLOAT  firOutputRateQuadCal;
	DOUBLE txNcoFrequencyQuad;

	UINT8 currentState;

	interfaceSetup portSetup;

	agcCommonSetting commonAGC;
	mgcCommonSetting commonMGC;
	slowAttackAGC slowAGC;
	fastAttackAGC fastAGC;
	gainTableType gainTable;

	pllSynthesizerType rxPllSynthesizerVar;
	pllSynthesizerType txPllSynthesizerVar;

	vcoPllType               rxVcoPll;
	vcoPllType               txVcoPll;
	pllWordsType             rxPllWords;
	pllWordsType             txPllWords;
	loopFilterParameterType  rxLoopFilterParam;
	loopFilterParameterType  txLoopFilterParam;
	vcoParameterType         rxVcoParam;
	vcoParameterType         txVcoParam;
}
profile_t;



#endif // _INCLUDE_APP_AD9361_PROFILE_H_
