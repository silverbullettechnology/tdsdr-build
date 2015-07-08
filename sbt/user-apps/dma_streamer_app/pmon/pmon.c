
/***************************** Include Files ********************************/

#include "xaxipmon.h"
#include "xstatus.h"
#include "stdio.h"

/************************** Constant Definitions ****************************/

/*
 * The following constants map to the XPAR parameters created in the
 * xparameters.h file. They are defined here such that a user can easily
 * change all the needed parameters in one place.
 */

static XAxiPmon AxiPmonInst;      /* System Monitor driver instance */


int Init_Pmon (void)
{
	int Status;
	XAxiPmon_Config *ConfigPtr;
	XAxiPmon *AxiPmonInstPtr = &AxiPmonInst;

	/*
	 * Initialize the AxiPmon driver.
	 */
	ConfigPtr = XAxiPmon_LookupConfig(0);
	if (ConfigPtr == NULL) {
		return XST_FAILURE;
	}

	XAxiPmon_CfgInitialize(AxiPmonInstPtr, ConfigPtr, 0);

	/*
	 * Self Test the System Monitor/ADC device
	 */
	Status = XAxiPmon_SelfTest(AxiPmonInstPtr);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	return XST_SUCCESS;
}


void Reset_Pmon (void)
{
	XAxiPmon_ResetFifo(&AxiPmonInst);
	XAxiPmon_ResetMetricCounter(&AxiPmonInst);
}

void SetPmonAXIS (void)
{
	XAxiPmon *AxiPmonInstPtr = &AxiPmonInst;
	u8 AXIS_Slot = 0x0;
	u32 ClkCntHigh, ClkCntLow;

	XAxiPmon_SetMetrics(AxiPmonInstPtr, AXIS_Slot, XAPM_METRIC_SET_16, XAPM_METRIC_COUNTER_0); // transfer cycle count
	XAxiPmon_SetMetrics(AxiPmonInstPtr, AXIS_Slot, XAPM_METRIC_SET_17, XAPM_METRIC_COUNTER_1); // packet count
	XAxiPmon_SetMetrics(AxiPmonInstPtr, AXIS_Slot, XAPM_METRIC_SET_18, XAPM_METRIC_COUNTER_2); // data byte count
	XAxiPmon_SetMetrics(AxiPmonInstPtr, AXIS_Slot, XAPM_METRIC_SET_19, XAPM_METRIC_COUNTER_3); // position byte count
	XAxiPmon_SetMetrics(AxiPmonInstPtr, AXIS_Slot, XAPM_METRIC_SET_20, XAPM_METRIC_COUNTER_4); // null byte count
	XAxiPmon_SetMetrics(AxiPmonInstPtr, AXIS_Slot, XAPM_METRIC_SET_21, XAPM_METRIC_COUNTER_5); // slave idle count
	XAxiPmon_SetMetrics(AxiPmonInstPtr, AXIS_Slot, XAPM_METRIC_SET_22, XAPM_METRIC_COUNTER_6); // master idle count

	XAxiPmon_GetGlobalClkCounter(AxiPmonInstPtr, &ClkCntHigh, &ClkCntLow);
	xil_printf("global clock %x %x\n", ClkCntHigh, ClkCntLow);

	XAxiPmon_EnableMetricsCounter(AxiPmonInstPtr);
	XAxiPmon_EnableGlobalClkCounter(AxiPmonInstPtr);

	XAxiPmon_GetGlobalClkCounter(AxiPmonInstPtr, &ClkCntHigh, &ClkCntLow);
	xil_printf("global clock %x %x\n", ClkCntHigh, ClkCntLow);

}

void GetPmonAXIS (void)
{
	XAxiPmon *AxiPmonInstPtr = &AxiPmonInst;

	u32 cyc_cnt;
	u32 pkt_cnt;
	u32 dbyte_cnt;
	u32 posbyte_cnt;
	u32 nulbyte_cnt;
	u32 slv_idle_cnt;
	u32 mstr_idle_cnt;

	u32 ClkCntHigh, ClkCntLow;

	XAxiPmon_DisableGlobalClkCounter(AxiPmonInstPtr);
	XAxiPmon_DisableMetricsCounter(AxiPmonInstPtr);

	/* Get Metric Counter 0  */
	cyc_cnt       = XAxiPmon_GetMetricCounter(AxiPmonInstPtr, XAPM_METRIC_COUNTER_0);
	pkt_cnt       = XAxiPmon_GetMetricCounter(AxiPmonInstPtr, XAPM_METRIC_COUNTER_1);
	dbyte_cnt     = XAxiPmon_GetMetricCounter(AxiPmonInstPtr, XAPM_METRIC_COUNTER_2);
	posbyte_cnt   = XAxiPmon_GetMetricCounter(AxiPmonInstPtr, XAPM_METRIC_COUNTER_3);
	nulbyte_cnt   = XAxiPmon_GetMetricCounter(AxiPmonInstPtr, XAPM_METRIC_COUNTER_4);
	slv_idle_cnt  = XAxiPmon_GetMetricCounter(AxiPmonInstPtr, XAPM_METRIC_COUNTER_5);
	mstr_idle_cnt = XAxiPmon_GetMetricCounter(AxiPmonInstPtr, XAPM_METRIC_COUNTER_6);

	/* Get Global Clock Cycles Count in ClkCntHigh,ClkCntLow */
	XAxiPmon_GetGlobalClkCounter(AxiPmonInstPtr, &ClkCntHigh, &ClkCntLow);

	xil_printf("Performance Monitor (AXIS) report\n");
	xil_printf("---------------------------------\n");
	xil_printf("global clock %x %x\n", ClkCntHigh, ClkCntLow);
	xil_printf("transfer cycle count %d\n", cyc_cnt);
	xil_printf("packet count         %d\n", pkt_cnt);
	xil_printf("data byte count      %d\n", dbyte_cnt);
	xil_printf("position byte count  %d\n", posbyte_cnt);
	xil_printf("null byte count      %d\n", nulbyte_cnt);
	xil_printf("slave idle count     %d\n", slv_idle_cnt);
	xil_printf("master idle count    %d\n", mstr_idle_cnt);

}



void SetPmonAXI4 (void)
{
	XAxiPmon *AxiPmonInstPtr = &AxiPmonInst;
	u8 AXI4_Slot = 0x1;

	XAxiPmon_SetMetrics(AxiPmonInstPtr, AXI4_Slot, XAPM_METRIC_SET_0,  XAPM_METRIC_COUNTER_0); // write transaction count
	XAxiPmon_SetMetrics(AxiPmonInstPtr, AXI4_Slot, XAPM_METRIC_SET_2,  XAPM_METRIC_COUNTER_1); // write byte count
 	XAxiPmon_SetMetrics(AxiPmonInstPtr, AXI4_Slot, XAPM_METRIC_SET_7,  XAPM_METRIC_COUNTER_2); // slave write idle count
	XAxiPmon_SetMetrics(AxiPmonInstPtr, AXI4_Slot, XAPM_METRIC_SET_10, XAPM_METRIC_COUNTER_3); // number of wlast

	XAxiPmon_EnableMetricsCounter(AxiPmonInstPtr);
	XAxiPmon_EnableGlobalClkCounter(AxiPmonInstPtr);
}


void GetPmonAXI4 (void)
{
	XAxiPmon *AxiPmonInstPtr = &AxiPmonInst;

	u32 wr_tx_cnt;
	u32 wrbyte_cnt;
	u32 slv_wr_idle_cnt;
	u32 wlast_cnt;

	XAxiPmon_DisableGlobalClkCounter(AxiPmonInstPtr);
	XAxiPmon_DisableMetricsCounter(AxiPmonInstPtr);

	/* Get Metric Counter 0  */
	wr_tx_cnt       = XAxiPmon_GetMetricCounter(AxiPmonInstPtr, XAPM_METRIC_COUNTER_0);
	wrbyte_cnt      = XAxiPmon_GetMetricCounter(AxiPmonInstPtr, XAPM_METRIC_COUNTER_1);
	slv_wr_idle_cnt = XAxiPmon_GetMetricCounter(AxiPmonInstPtr, XAPM_METRIC_COUNTER_2);
	wlast_cnt       = XAxiPmon_GetMetricCounter(AxiPmonInstPtr, XAPM_METRIC_COUNTER_3);

	xil_printf("Performance Monitor (AXI4) report\n");
	xil_printf("---------------------------------\n");
	xil_printf("write transaction count %d\n", wr_tx_cnt);
	xil_printf("write byte count        %d\n", wrbyte_cnt);
	xil_printf("slave write idle count  %d\n", slv_wr_idle_cnt);
	xil_printf("number of wlast         %d\n", wlast_cnt);

}

#include <fifo_dev.h>
u32 Xil_In32 (u32 addr)
{
	unsigned long  val;
	assert(fifo_pmon_read(addr, &val) == 0);
	return val;
}

void Xil_Out32 (u32 addr, u32 data)
{
	unsigned long  val = data;
	assert(fifo_pmon_write(addr, val) == 0);
}

#if 0

/****************************************************************************/
/**
*
* This function runs a test on the AXI Performance Monitor device using the
* driver APIs.
* This function does the following tasks:
*	- Initiate the AXI Performance Monitor device driver instance
*	- Run self-test on the device
*	- Sets Metric Selector to select Slave Agent Number 1 and first set of
*	Metrics
*	- Enables Global Clock Counter and Metric Counters
*	- Calls Application for which Metrics need to be computed
*	- Disables Global Clock Counter and Metric Counters
*	- Read Global Clock Counter and Metric Counter 0
*
* @param	AxiPmonDeviceId is the XPAR_<AXIPMON_instance>_DEVICE_ID value
*		from xparameters.h.
* @param	Metrics is an user referece variable in which computed metrics
*			will be filled
* @param	ClkCntHigh is an user referece variable in which Higher 64 bits
*			of Global Clock Counter are filled
* @param	ClkCntLow is an user referece variable in which Lower 64 bits
*			of Global Clock Counter are filled
*
* @return
*		- XST_SUCCESS if the example has completed successfully.
*		- XST_FAILURE if the example has failed.
*
* @note   	None
*
****************************************************************************/
int AxiPmonPolledExample(u16 AxiPmonDeviceId, u32 *Metrics, u32 *ClkCntHigh,
							     u32 *ClkCntLow)
{
	int Status;
	XAxiPmon_Config *ConfigPtr;
	u8 SlotId = 0x0;
	u16 Range2 = 0x10;	/* Range 2 - 16 */
	u16 Range1 = 0x08;	/* Range 1 - 8 */
	XAxiPmon *AxiPmonInstPtr = &AxiPmonInst;

	/*
	 * Initialize the AxiPmon driver.
	 */
	ConfigPtr = XAxiPmon_LookupConfig(AxiPmonDeviceId);
	if (ConfigPtr == NULL) {
		return XST_FAILURE;
	}

	XAxiPmon_CfgInitialize(AxiPmonInstPtr, ConfigPtr,
				ConfigPtr->BaseAddress);

	/*
	 * Self Test the System Monitor/ADC device
	 */
	Status = XAxiPmon_SelfTest(AxiPmonInstPtr);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	/*
	 * Select Agent and required set of Metrics for a Counter.
	 * We can select another agent,Metrics for another counter by
	 * calling below function again.
	 */

	XAxiPmon_SetMetrics(AxiPmonInstPtr, SlotId, XAPM_METRIC_SET_0,
								XAPM_METRIC_COUNTER_0);

	/*
	 * Set Incrementer Ranges
	 */
	XAxiPmon_SetIncrementerRange(AxiPmonInstPtr, XAPM_INCREMENTER_0,
							Range2, Range1);
	/*
	 * Enable Metric Counters.
	 */
	XAxiPmon_EnableMetricsCounter(AxiPmonInstPtr);

	/*
	 * Enable Global Clock Counter Register.
	 */
	XAxiPmon_EnableGlobalClkCounter(AxiPmonInstPtr);

	/*
	 * Application for which Metrics has to be computed should be
	 * called here
	 */

	/*
	 * Disable Global Clock Counter Register.
	 */

	XAxiPmon_DisableGlobalClkCounter(AxiPmonInstPtr);

	/*
	 * Disable Metric Counters.
	 */
	XAxiPmon_DisableMetricsCounter(AxiPmonInstPtr);

	/* Get Metric Counter 0  */
	*Metrics = XAxiPmon_GetMetricCounter(AxiPmonInstPtr,
						XAPM_METRIC_COUNTER_0);

	/* Get Global Clock Cycles Count in ClkCntHigh,ClkCntLow */
	XAxiPmon_GetGlobalClkCounter(AxiPmonInstPtr, ClkCntHigh, ClkCntLow);


	return XST_SUCCESS;

}
#endif
