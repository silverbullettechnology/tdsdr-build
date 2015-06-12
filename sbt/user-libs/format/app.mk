# Tools Makefile AD9361 userspace driver, library, and tool
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
# vim:ts=4:noexpandtab

ifneq ($(PETALINUX),)
include $(PETALINUX)/software/petalinux-dist/tools/user-commons.mk
include $(LOGGING_MK)
endif

# library deps and out-of-tree - not cleaned
LIBS := -l$(NAME) -lm

APPS := $(BIN)/dsa_format

# in-tree objs - cleaned
OBJS := \
	src/common/common.o \
	src/app/main.o

CFLAGS  := -O2 -Wall -Werror -fpic
CFLAGS  += -I$(PWD)/include
LDFLAGS := -L$(BIN)

all: install

.PHONY: install
install: $(APPS)
	true

$(BIN)/dsa_format: $(OBJS)
	$(CC) $(LDFLAGS) -o$@ $^ $(LIBS)

%.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $<

romfs: $(APPS)
	mkdir -p $(ROMFSDIR)/usr/bin
	install -m755 $(BIN)/dsa_format $(ROMFSDIR)/usr/bin

clean:
	rm -f $(OBJS)
	rm -f $(APPS)

image:

