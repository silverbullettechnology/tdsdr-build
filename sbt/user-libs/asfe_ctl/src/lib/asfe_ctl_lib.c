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
#include "asfe_ctl_hal.h"
#include "asfe_ctl_lib.h"
#include "asfe_ctl_hal_linux.h"

void asfe_test_function(){
	printf("test function message \r\n");
}

int asfe_ctl_spiwritearray(UINT16 *wr_data, UINT16 size){
	HAL_SPIWriteByteArray(wr_data, size);

	return 0;
}

int asfe_ctl_spitransaction(UINT16 *wr_data, UINT16 *rd_data, UINT16 size){
	HAL_SPIExchangeByteArray(wr_data, rd_data, size);
	return 0;
}

int asfe_ctl_dev_reopen (int num, int reinit)
{
	asfe_ctl_hal_linux_spi_done();
	asfe_ctl_hal_linux_gpio_done();
	asfe_ctl_hal_linux_spi_init("/dev/spidev1.0");
	
	//return dev_reopen(num,reinit);
	return 0;
}

int asfe_ctl_asfe_spare_1(int val){
	
	int pin = 6;
	pin += 54; // EMIO -> GPIO offset, MIO are GPIO 0..53

	return asfe_ctl_hal_linux_set_gpio(pin,val);
}

int asfe_ctl_asfe_spare_2(int val){
	
	int pin = 7;
	pin += 54; // EMIO -> GPIO offset, MIO are GPIO 0..53

	return asfe_ctl_hal_linux_set_gpio(pin,val);
}

int asfe_ctl_asfe_rstn(int val){
	
	int pin = 8;
	pin += 54; // EMIO -> GPIO offset, MIO are GPIO 0..53

	return asfe_ctl_hal_linux_set_gpio(pin,val);
}

int asfe_ctl_adi1_tx_en(int val){
	
	int pin = 9;
	pin += 54; // EMIO -> GPIO offset, MIO are GPIO 0..53

	return asfe_ctl_hal_linux_set_gpio(pin,val);
}

int asfe_ctl_asfe_reserve1(int val){
	
	int pin = 10;
	pin += 54; // EMIO -> GPIO offset, MIO are GPIO 0..53

	return asfe_ctl_hal_linux_set_gpio(pin,val);
}

int asfe_ctl_asfe_reserve2(int val){
	
	int pin = 11;
	pin += 54; // EMIO -> GPIO offset, MIO are GPIO 0..53

	return asfe_ctl_hal_linux_set_gpio(pin,val);
}

int asfe_ctl_asfe_reserve4(int val){
	
	int pin = 12;
	pin += 54; // EMIO -> GPIO offset, MIO are GPIO 0..53

	return asfe_ctl_hal_linux_set_gpio(pin,val);
}

int asfe_ctl_asfe_reserve3(int val){
	
	int pin = 13;
	pin += 54; // EMIO -> GPIO offset, MIO are GPIO 0..53

	return asfe_ctl_hal_linux_set_gpio(pin,val);
}

int asfe_ctl_adi2_tx_en(int val){
	
	int pin = 14;
	pin += 54; // EMIO -> GPIO offset, MIO are GPIO 0..53

	return asfe_ctl_hal_linux_set_gpio(pin,val);
}

int asfe_ctl_asfe_spare_3(int val){
	
	int pin = 15;
	pin += 54; // EMIO -> GPIO offset, MIO are GPIO 0..53

	return asfe_ctl_hal_linux_set_gpio(pin,val);
}

int asfe_ctl_asfe_spare_4(int val){
	
	int pin = 16;
	pin += 54; // EMIO -> GPIO offset, MIO are GPIO 0..53

	return asfe_ctl_hal_linux_set_gpio(pin,val);
}


