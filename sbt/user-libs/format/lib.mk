# Shared-library Makefile for dma_streamer_mod
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

ifneq ($(PETALINUX),)
include $(PETALINUX)/software/petalinux-dist/tools/user-commons.mk
include $(LOGGING_MK)
endif

# library deps and out-of-tree - not cleaned
LIBS := 

# in-tree objs - cleaned
OBJS := \
	src/lib/bin.o \
	src/lib/bit.o \
	src/lib/dec.o \
	src/lib/hex.o \
	src/lib/iqw.o \
	src/lib/nul.o \
	src/lib/raw.o \
	src/lib/common.o

CFLAGS += -Wall -Werror -fpic
CFLAGS += -I$(PWD)/include
LFLAGS += -shared -s -lm

all: install

.PHONY: install
install: $(BIN)/$(ANAME) $(BIN)/$(SHARED) $(BIN)/$(LDNAME) $(BIN)/$(SONAME)
ifneq ($(STAGEDIR),)
	# Install libraries to stage dir - shared and static
	mkdir -p $(STAGEDIR)/usr/lib
	cp -a $(BIN)/$(SHARED) $(STAGEDIR)/usr/lib
	cp -a $(BIN)/$(SONAME) $(STAGEDIR)/usr/lib
	cp -a $(BIN)/$(LDNAME) $(STAGEDIR)/usr/lib
	cp -a $(BIN)/$(ANAME)  $(STAGEDIR)/usr/lib
	# Install includes to stage dir
	mkdir -p $(STAGEDIR)/usr/include/$(NAME)
	cp -a include/$(NAME).h  $(STAGEDIR)/usr/include
endif

$(BIN)/$(ANAME): $(OBJS)
	$(AR) rcs $@ $^

$(BIN)/$(SHARED): $(OBJS)
	$(CC) $(PRE_LFLAGS) $(LFLAGS) $(POST_LFLAGS) \
		-Wl,-soname,$(SONAME) -Wl,--version-script,$(NAME).v \
		-o$@ $^ $(LIBS)

$(BIN)/$(SONAME): $(BIN)/$(SHARED)
	ln -sf $(SHARED) $@ 

$(BIN)/$(LDNAME): $(BIN)/$(SHARED)
	ln -sf $(SHARED) $@ 

%.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $<
	
romfs: install
	# Install shared libraries to rootfs
	mkdir -p $(ROMFSDIR)/usr/lib
	cp -a $(BIN)/$(SHARED) $(ROMFSDIR)/usr/lib
	cp -a $(BIN)/$(SONAME) $(ROMFSDIR)/usr/lib
	cp -a $(BIN)/$(LDNAME) $(ROMFSDIR)/usr/lib
ifeq ($(CONFIG_USER_LIBS_PIPE_DEV_STATIC),y)
	cp -a $(BIN)/$(ANAME)  $(ROMFSDIR)/usr/lib
endif
ifeq ($(CONFIG_USER_LIBS_PIPE_DEV_INCLUDES),y)
	mkdir -p $(ROMFSDIR)/usr/include
	cp -a include/$(NAME).h  $(ROMFSDIR)/usr/include
endif
	true

clean:
	rm -f $(OBJS)
	rm -f $(BIN)/$(SHARED)
	rm -f $(BIN)/$(LDNAME)
	rm -f $(BIN)/$(SONAME)
	rm -f $(BIN)/$(ANAME)

image:
