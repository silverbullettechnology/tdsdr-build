#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include <errno.h>

#include "lib.h"
#include "util.h"
#include "api_types.h"
#include "cal_ctl_hal.h"
#include "cal_ctl_lib.h"
#include "cal_ctl_hal_linux.h"

void cal_test_function(){
	printf("test function message \r\n");
}

int cal_ctl_spiwritearray(UINT16 *wr_data, UINT16 size){
	HAL_SPIWriteByteArray(wr_data, size);

	return 0;
}

int cal_ctl_spitransaction(UINT16 *wr_data, UINT16 *rd_data, UINT16 size){
	HAL_SPIExchangeByteArray(wr_data, rd_data, size);
	return 0;
}

int cal_ctl_dev_reopen (int num, int reinit)
{
	cal_ctl_hal_linux_spi_done();
	cal_ctl_hal_linux_gpio_done();
	cal_ctl_hal_linux_spi_init("/dev/spidev1.3");
	
	//return dev_reopen(num,reinit);
	return 0;
}

