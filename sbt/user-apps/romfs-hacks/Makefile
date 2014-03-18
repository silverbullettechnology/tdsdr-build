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
	mkdir -p $(ROMFSDIR)/media/
ifdef CONFIG_DEFAULTS_SERCOMM_SDRDC_CUT1
	$(SHELL) add-path-entry.sh cut1
endif
ifdef CONFIG_DEFAULTS_SERCOMM_SDRDC_CUT2
	$(SHELL) add-path-entry.sh cut2
endif
ifdef CONFIG_DEFAULTS_SILVERBULLETTECH_SDRDC
	$(SHELL) add-path-entry.sh
endif
	install -m755 switch.sh $(ROMFSDIR)/switch
	install -m755 scanall.sh $(ROMFSDIR)/etc/init.d/scanall
	rm -f $(ROMFSDIR)/etc/rcS.d/S30scanall
	ln -sv ../init.d/scanall $(ROMFSDIR)/etc/rcS.d/S30scanall
	$(SHELL) add-network.sh usb0

image: 
	echo image

