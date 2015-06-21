# Top-level Makefile
#
# Copyright 2014,2015 Silver Bullet Technology
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
# vim:ts=4:noexpandtab

ifneq ($(PETALINUX),)
include $(PETALINUX)/software/petalinux-dist/tools/user-commons.mk
include $(LOGGING_MK)
export DESTDIR := $(ROMFSDIR)
.PHONY: romfs image
else
include host.mk
endif

.PHONY: all install clean

# cross-compile friendly
CC     ?= $(CROSS_COMPILE)gcc
CFLAGS += -Iinclude -I../include -I../lib
CFLAGS += -I$(PETALINUX)/software/user-modules/srio_dev
CFLAGS += -I$(PETALINUX)/software/user-libs/sbt_common/include
LDFLAGS += -L../lib/config
LDFLAGS += -L$(STAGEDIR)/usr/lib

# Compile with debugging
CFLAGS += -g
#LDFLAGS += -s 

# Minimal JPL-style checking
CFLAGS += -Wall -Werror -std=gnu99
# -pedantic

export CC CFLAGS LDFLAGS

# quick and dirty path setup
ifneq ($(DESTDIR),)
DESTDIR := $(shell mkdir -p $(DESTDIR) && cd $(DESTDIR) && pwd)
endif
export DESTDIR

all install clean:
	set
	$(MAKE) -C lib       $@
	$(MAKE) -C src       $@
	$(MAKE) -C script    $@

tests:
	$(MAKE) -C lib       all
	$(MAKE) -C src       $@

ifneq ($(PETALINUX),)
romfs: all install

image: romfs
	make -C ${PETALINUX}/software/petalinux-dist image
endif

