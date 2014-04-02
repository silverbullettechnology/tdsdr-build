#include <assert.h>

//#include <asfe_ctl/main.h>

#include "api_types.h"
//#include "main.h"

#ifndef _INCLUDE_LIB_LIB_H_
#define _INCLUDE_LIB_LIB_H_

void asfe_test_function();

int asfe_ctl_spiwritearray(UINT16 *wr_data, UINT16 size);

int asfe_ctl_spitransaction(UINT16 *wr_data, UINT16 *rd_data, UINT16 size);

int asfe_ctl_dev_reopen (int num, int reinit);

int asfe_ctl_asfe_spare_1(int val);

int asfe_ctl_asfe_spare_2(int val);

int asfe_ctl_asfe_rstn(int val);

int asfe_ctl_adi1_tx_en(int val);

int asfe_ctl_asfe_reserve1(int val);

int asfe_ctl_asfe_reserve2(int val);

int asfe_ctl_asfe_reserve4(int val);

int asfe_ctl_asfe_reserve3(int val);

int asfe_ctl_adi2_tx_en(int val);

int asfe_ctl_asfe_spare_3(int val);

int asfe_ctl_asfe_spare_4(int val);

#endif /* _INCLUDE_LIB_LIB_H_ */
