# Makefile for demo tools
#
# Copyright 2015 Silver Bullet Technology
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may not use this
# file except in compliance with the License.  You may obtain a copy of the License at:
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software distributed under
# the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied.  See the License for the specific language governing
# permissions and limitations under the License.

include $(PETALINUX)/software/petalinux-dist/tools/user-commons.mk
include $(LOGGING_MK)
APPS := \
	demo-sample \
	demo-transmit

DEMO_OBJS := \
	enum.o \
	access.o \
	release.o \
	common.o \
	st_term.o

DEMO_LIBS := \
	-lrt \
	-ldsm \
	-lformat \
	-lpipe_dev \
	-lv49_message \
	-lv49_client \
	-lsbt_common \
	-lconfig


CFLAGS  += -I$(PETALINUX)/software/user-modules/dma_streamer_mod
CFLAGS  += -I$(PETALINUX)/software/user-modules/pipe_dev
CFLAGS  += -Wall -Werror
LDFLAGS += -s


all: $(APPS)

demo-transmit: $(DEMO_OBJS) demo-transmit.o
	$(CC) $(LDFLAGS) -o $@ $^ $(DEMO_LIBS)

demo-sample: $(DEMO_OBJS) demo-sample.o
	$(CC) $(LDFLAGS) -o $@ $^ $(DEMO_LIBS)

clean:
	-rm -f $(APPS) *.o

.PHONY: romfs image

romfs: all $(DO_STRIP)
	mkdir -p $(ROMFSDIR)/bin
	install -m755 $(APPS) $(ROMFSDIR)/bin

image: romfs
	make -C ${PETALINUX}/software/petalinux-dist image

%.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $<

