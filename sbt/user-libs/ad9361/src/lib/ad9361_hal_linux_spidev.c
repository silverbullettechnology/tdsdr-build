/** \file      src/lib/ad9361_hal_linux_spidev.c
 *  \brief     Linux/POSIX HAL implementation
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
#include <sys/ioctl.h>

// quash a warning about using kernel headers in userspace
#define __EXPORTED_HEADERS__
#include <linux/spi/spidev.h>


int ad9361_hal_linux_spidev_txn (int fd, int len, void *txb, void *rxb, int spd)
{
	struct spi_ioc_transfer xfr;

	xfr.tx_buf        = (unsigned long)txb;
	xfr.rx_buf        = (unsigned long)rxb;
    xfr.len           = len;
	xfr.speed_hz      = spd;
	xfr.bits_per_word = 8;
	xfr.cs_change     = 0;
	xfr.delay_usecs   = 0;

	return ioctl(fd, SPI_IOC_MESSAGE(1), &xfr);
}


int ad9361_hal_linux_spidev_init (int fd)
{
	unsigned char  v8;
	unsigned long  v32;

	// setup bus here with ioctl, if it fails abort.  Mode 0 (CPOL 0, CPHA 0), MSB first,
	// 8-bit words, 250kHz max speed
	v8 = 1;
	if ( ioctl(fd, SPI_IOC_WR_MODE, &v8) )
		return -1;

	v8 = 0;
	if ( ioctl(fd, SPI_IOC_WR_LSB_FIRST, &v8) )
		return -1;

	v8 = 8;
	if ( ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &v8) )
		return -1;

	v32 = 5200000;
	if ( ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &v32) )
		return -1;

	return 0;
}

