# Makefile options for filesystem tweaks
#
# Copyright 2013,2014 Silver Bullet Technology
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may not use this
# file except in compliance with the License.  You may obtain a copy of the License at:
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software distributed under
# the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied.  See the License for the specific language governing
# permissions and limitations under the License.
#

include $(PETALINUX)/software/petalinux-dist/tools/user-commons.mk
include $(PETALINUX)/software/petalinux-dist/config/.config

export ROMFSDIR

all:

clean:

.PHONY: romfs image

romfs:
	rm   -rf $(ROMFSDIR)/media/
	mkdir -p $(ROMFSDIR)/media/card
	mkdir -p $(ROMFSDIR)/media/emmc
ifdef CONFIG_USER_APPS_ROMFS_HACKS_FSTAB_MICROSD
	$(SHELL) add-fstab-entry.sh /dev/mmcblk0p1 /media/card auto defaults 0 0
endif
ifdef CONFIG_USER_APPS_ROMFS_HACKS_FSTAB_EMMC
	$(SHELL) add-fstab-entry.sh /dev/mmcblk1p1 /media/emmc auto noauto 0 0
endif
ifdef CONFIG_DEFAULTS_SERCOMM_SDRDC_CUT1
	$(SHELL) add-path-entry.sh cut1
endif
ifdef CONFIG_DEFAULTS_SERCOMM_SDRDC_CUT2
	$(SHELL) add-path-entry.sh cut2
endif
ifdef CONFIG_DEFAULTS_SILVERBULLETTECH_SDRDC
	$(SHELL) add-path-entry.sh
endif


image: 
	echo image

