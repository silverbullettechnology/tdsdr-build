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

APPS := $(BIN)/$(NAME)

# in-tree objs - cleaned
OBJS := \
	src/common/util.o \
	src/app/config/config_buffer.o \
	src/app/config/config_parse.o \
	src/app/$(NAME)/format-gen.o \
	src/app/$(NAME)/parse-gen.o \
	src/app/$(NAME)/map-compatible.o \
	src/app/$(NAME)/map-adi-common.o \
	src/app/$(NAME)/map-adi-iio-sysfs.o \
	src/app/$(NAME)/map-adi-iio-debugfs.o \
	src/app/$(NAME)/map.o \
	src/app/$(NAME)/map-debug.o \
	src/app/$(NAME)/map-device.o \
	src/app/$(NAME)/main.o


ifdef ADI
OBJS += \
	$(ADI)/Application/TRX/main.o \
	src/app/$(NAME)/$(ADI)/format-adi.o \
	src/app/$(NAME)/$(ADI)/parse-adi.o \
	src/app/$(NAME)/$(ADI)/struct-adi.o \
	src/app/$(NAME)/$(ADI)/map-adi-api.o \
	src/app/$(NAME)/$(ADI)/map-adi-mw.o \
	src/app/$(NAME)/$(ADI)/map-adi-trx.o \
	src/app/$(NAME)/$(ADI)/map-bist-r279.o \
	src/app/$(NAME)/$(ADI)/map-profile.o

# debugging commands, not needed for production
#OBJS += \
	src/app/$(NAME)/$(ADI)/map-format.o \
	src/app/$(NAME)/$(ADI)/map-parse.o

ADI_CFLAGS := \
	-I$(ADI)/Include \
	-I$(ADI)/Common/Include \
	-I$(ADI)/TRX/Lib/Include \
	-I$(ADI)/Midlware/TRX/Include \
	-Iinclude/app/$(NAME)/$(ADI)
endif

APP_CFLAGS := \
	-Iinclude/app/config \
	-Iinclude/app/$(NAME) \
	-Iinclude/common \
	-Iinclude/lib

ifeq ($(CONFIG_USER_LIBS_AD9361_READLINE),y)
APP_CFLAGS += -DAD9361_USE_READLINE
LIBS       += -lcurses -lreadline
endif

ETC_DIR := /etc/$(NAME)
LIB_DIR := /usr/lib/$(NAME)
APP_CFLAGS += -DLIB_DIR=\"$(LIB_DIR)\" -DETC_DIR=\"$(ETC_DIR)\"

PRE_CFLAGS  := -g -O0 -Wall -Werror -fpic $(APP_CFLAGS) $(ADI_CFLAGS)
PRE_LFLAGS  := -L$(STAGEDIR)/lib -L$(STAGEDIR)/usr/lib 
POST_CFLAGS := $(REV_CFLAGS)
POST_LFLAGS :=

all: install

.PHONY: install
install: $(APPS)
	true

$(BIN)/$(NAME): $(OBJS)
	$(CC) $(PRE_LFLAGS) $(LFLAGS) $(POST_LFLAGS) \
		-o$@ $^ $(LIBS)

%.o: %.c
	$(CC) -c $(PRE_CFLAGS) $(CFLAGS) $(POST_CFLAGS) -o $@ $<

src/app/$(NAME)/main.o: src/app/$(NAME)/main.c
	$(CC) -c $(PRE_CFLAGS) $(CFLAGS) $(POST_CFLAGS) $(HOST_CFLAGS) -o $@ $<

$(ADI)/Application/TRX/main.o: $(ADI)/Application/TRX/main.c
	$(CC) -c $(PRE_CFLAGS) $(CFLAGS) $(POST_CFLAGS) -Dmain=trx_main -o $@ $<
	
romfs: $(APPS)
	mkdir -p $(ROMFSDIR)/usr/bin
	install -m755 $(BIN)/$(NAME) $(ROMFSDIR)/usr/bin
	mkdir -p $(ROMFSDIR)$(ETC_DIR) $(ROMFSDIR)$(LIB_DIR)
	-(cd script/etc && cp -a * $(ROMFSDIR)$(ETC_DIR))
	-(cd script/lib && cp -a * $(ROMFSDIR)$(LIB_DIR))
	chmod -R +x $(ROMFSDIR)$(LIB_DIR)/script
	install -m755 script/init/ad9361-init.sh $(ROMFSDIR)/etc/init.d
	rm -f $(ROMFSDIR)/etc/rcS.d/S95ad9361-init.sh
ifeq ($(CONFIG_USER_LIBS_AD9361_INIT_AUTOLOAD),y)
	ln -s ../init.d/ad9361-init.sh $(ROMFSDIR)/etc/rcS.d/S95ad9361-init.sh
endif

clean:
	rm -f $(OBJS)
	rm -f $(APPS)

image:

