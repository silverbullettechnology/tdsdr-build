#include <assert.h>

//#include <cal_ctl/main.h>

#include "api_types.h"
//#include "main.h"

#ifndef _INCLUDE_LIB_LIB_H_
#define _INCLUDE_LIB_LIB_H_

void cal_test_function();

int cal_ctl_spiwritearray(UINT16 *wr_data, UINT16 size);

int cal_ctl_spitransaction(UINT16 *wr_data, UINT16 *rd_data, UINT16 size);

int cal_ctl_dev_reopen (int num, int reinit);


#endif /* _INCLUDE_LIB_LIB_H_ */
