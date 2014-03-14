# Makefile for stress-test application
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
# \copyright Copyright (C) 2013 Silver Bullet Technologies
# vim:ts=4:noexpandtab

ifneq ($(PETALINUX),)
include $(PETALINUX)/software/petalinux-dist/tools/user-commons.mk
include $(LOGGING_MK)
endif

APPS := stress-alu stress-fpu stress-mem stress-mmc stress-nor
LIBS := 

PRE_CFLAGS  := -g -O0 -Wall -Werror -fpic 
POST_CFLAGS := 

all: install

.PHONY: install
install: $(APPS)

stress-alu: stress-alu.c common.c
	$(CC) $(PRE_CFLAGS) $(CFLAGS) $(POST_CFLAGS) -o$@ $^ $(LIBS) -lm

stress-fpu: stress-fpu.c common.c
	$(CC) $(PRE_CFLAGS) $(CFLAGS) $(POST_CFLAGS) -o$@ $^ $(LIBS) -lm

stress-mem: stress-mem.c common.c
	$(CC) $(PRE_CFLAGS) $(CFLAGS) $(POST_CFLAGS) -o$@ $^ $(LIBS)

stress-mmc: stress-mmc.c common.c
	$(CC) $(PRE_CFLAGS) $(CFLAGS) $(POST_CFLAGS) -o$@ $^ $(LIBS)

stress-nor: stress-nor.c common.c
	$(CC) $(PRE_CFLAGS) $(CFLAGS) $(POST_CFLAGS) -o$@ $^ $(LIBS)

romfs: $(APPS)
	mkdir -p $(ROMFSDIR)/usr/bin
	install -m755 $(APPS) $(ROMFSDIR)/usr/bin
	install -m755 sdrdc-stress-test.sh $(ROMFSDIR)/usr/bin
	mkdir -p $(ROMFSDIR)/usr/lib/sst
	install -m644 kernel-errors $(ROMFSDIR)/usr/lib/sst
	rm -f "$(ROMFSDIR)/etc/rc5.d/S99sdr-stress-test"
	rm -f "$(ROMFSDIR)/etc/init.d/sdr-stress-test"

clean:
	rm -f $(APPS)

image:

