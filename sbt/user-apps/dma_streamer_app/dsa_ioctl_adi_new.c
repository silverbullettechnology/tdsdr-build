/** \file      dsa_ioctl_adi_new.c
 *  \brief     implementation of new ADI DAC/ADC controls
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>

#include <dma_streamer_mod.h>

#include "dsa_main.h"
#include "dsa_ioctl_adi_new.h"

#include "log.h"
LOG_MODULE_STATIC("ioctl_adi_new", LOG_LEVEL_INFO);


int dsa_ioctl_adi_new_read (int dev, int tx, unsigned long ofs, unsigned long *val)
{
	struct dsm_new_adi_regs  regs;
	int                      ret;

	regs.adi = dev;
	regs.tx  = tx;
	regs.ofs = ofs;

	// on failure print same error as kernelspace
	if ( (ret = ioctl(dsa_dev, DSM_IOCG_ADI_NEW_REG, &regs)) )
		printf("DSM_IOCG_ADI_NEW_REG AD%c %cX OFS %04lx READ %08lx: %d: %s\n",
		       (unsigned char)regs.adi + '1', regs.tx ? 'T' : 'R', regs.ofs, regs.val,
		       ret, strerror(errno));

	// on success return value if user's provided a buffer
	else if ( val )
		*val = regs.val;

	return ret;
}


int dsa_ioctl_adi_new_write (int dev, int tx, unsigned long ofs, unsigned long val)
{
	struct dsm_new_adi_regs  regs;
	int                      ret;

	regs.adi = dev;
	regs.tx  = tx;
	regs.ofs = ofs;
	regs.val = val;

	// on failure print same error as kernelspace
	if ( (ret = ioctl(dsa_dev, DSM_IOCS_ADI_NEW_REG_RB, &regs)) )
		printf("DSM_IOCS_ADI_NEW_REG_RB AD%c %cX OFS %04lx WRITE %08lx: %d: %s\n",
		       (unsigned char)regs.adi + '1', regs.tx ? 'T' : 'R', regs.ofs, regs.val,
		       ret, strerror(errno));

	return ret;
}

int dsa_ioctl_adi_new_stop (void)
{
	int  ret;

	if ( (ret = ioctl(dsa_dev, DSM_IOCS_STOP, 0)) )
		printf("DSM_IOCS_STOP: %d: %s\n", ret, strerror(errno));

	return ret;
}

int dsa_ioctl_adi_new_start (void)
{
	int  ret;

	if ( (ret = ioctl(dsa_dev, DSM_IOCS_START, 0)) )
		printf("DSM_IOCS_START: %d: %s\n", ret, strerror(errno));

	return ret;
}



