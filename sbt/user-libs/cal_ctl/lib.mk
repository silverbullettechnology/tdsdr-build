# Shared-library Makefile for CAL_CTL userspace tool
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
	src/common/util.o \
	src/lib/cal_ctl_hal_linux.o \
	src/lib/cal_ctl_hal_linux_spidev.o \
	src/lib/cal_ctl_hal_sim.o \
	src/lib/cal_ctl_hal.o \
	src/lib/cal_ctl_lib.o \
	src/lib/common.o

ADI_CFLAGS := \
	-DPROFILING_ENABLED \
	-I$(ADI)/Include \
	-I$(ADI)/Common/Include \

LIB_CFLAGS := \
	-Iinclude/common \
	-Iinclude/lib

KERNEL_INCLUDE := $(PETALINUX)/software/$(CONFIG_LINUXDIR)/include

PRE_CFLAGS  := -O0 -Wall -Werror -fpic $(LIB_CFLAGS) $(ADI_CFLAGS) -I$(KERNEL_INCLUDE)
PRE_LFLAGS  := -shared -s
POST_CFLAGS := -Iinclude/post
POST_LFLAGS :=


all: install

.PHONY: install
install: $(BIN)/$(ANAME) $(BIN)/$(SHARED) $(BIN)/$(LDNAME) $(BIN)/$(SONAME)
	# Install libraries to stage dir - shared and static
	mkdir -p $(STAGEDIR)/usr/lib
	cp -a $(BIN)/$(SHARED) $(STAGEDIR)/usr/lib
	cp -a $(BIN)/$(SONAME) $(STAGEDIR)/usr/lib
	cp -a $(BIN)/$(LDNAME) $(STAGEDIR)/usr/lib
	cp -a $(BIN)/$(ANAME)  $(STAGEDIR)/usr/lib
	# Install includes to stage dir
	mkdir -p $(STAGEDIR)/usr/include/$(NAME)
	cp -a $(NAME).h                         $(STAGEDIR)/usr/include
	cp -a include/app/$(NAME)/main.h        $(STAGEDIR)/usr/include/$(NAME)
	cp -a include/post/api_types.h          $(STAGEDIR)/usr/include/$(NAME)
	cp -a include/lib/cal_ctl_hal*.h       $(STAGEDIR)/usr/include/$(NAME)
	cp -a include/lib/cal_ctl_lib*.h       $(STAGEDIR)/usr/include/$(NAME)
	cp -a include/common/common.h           $(STAGEDIR)/usr/include/$(NAME)

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
	$(CC) -c $(PRE_CFLAGS) $(CFLAGS) $(POST_CFLAGS) -o $@ $<
	
romfs: install
	# Install shared libraries to rootfs
	mkdir -p "$(ROMFSDIR)/usr/lib"
	cp -a $(STAGEDIR)/usr/lib/$(SHARED) "$(ROMFSDIR)/usr/lib"
	cp -a $(STAGEDIR)/usr/lib/$(SONAME) "$(ROMFSDIR)/usr/lib"
	cp -a $(STAGEDIR)/usr/lib/$(LDNAME) "$(ROMFSDIR)/usr/lib"
ifeq ($(CONFIG_USER_LIBS_CAL_CTL_STATIC),y)
	cp -a $(STAGEDIR)/usr/lib/$(ANAME)  "$(ROMFSDIR)/usr/lib"
endif
ifeq ($(CONFIG_USER_LIBS_CAL_CTL_INCLUDES),y)
	mkdir -p "$(ROMFSDIR)/usr/include"
	cp -a $(STAGEDIR)/usr/include/$(NAME) "$(ROMFSDIR)/usr/include"
endif
	true

clean:
	rm -f $(OBJS)
	rm -f $(BIN)/$(SHARED)
	rm -f $(BIN)/$(LDNAME)
	rm -f $(BIN)/$(SONAME)
	rm -f $(BIN)/$(ANAME)

image:

