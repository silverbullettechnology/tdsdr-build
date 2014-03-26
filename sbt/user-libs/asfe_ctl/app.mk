# Application Makefile for ASFE_CTL userspace tool
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
LIBS := -l$(NAME) -lm

APPS := $(BIN)/$(NAME)

# in-tree objs - cleaned
OBJS := \
	src/common/util.o \
	src/app/config/config_buffer.o \
	src/app/config/config_parse.o \
	src/app/$(NAME)/format.o \
	src/app/$(NAME)/parse.o \
	src/app/$(NAME)/map.o \
	src/app/$(NAME)/map-asfe-common.o \
	src/app/$(NAME)/map-device.o \
	src/app/$(NAME)/main.o

ADI_CFLAGS := \
	-I$(ADI)/Include \
	-I$(ADI)/Common/Include \

APP_CFLAGS := \
	-Iinclude/app/config \
	-Iinclude/app/$(NAME) \
	-Iinclude/common \
	-Iinclude/lib \

#ifeq ($(CONFIG_USER_LIBS_ASFE_CTL_READLINE),y)
APP_CFLAGS += -DASFE_CTL_USE_READLINE
LIBS       += -lcurses -lreadline
#endif

ETC_DIR := /etc/$(NAME)
LIB_DIR := /usr/lib/$(NAME)
APP_CFLAGS += -DLIB_DIR=\"$(LIB_DIR)\" -DETC_DIR=\"$(ETC_DIR)\"

PRE_CFLAGS  := -g -O0 -Wall -Werror -fpic $(APP_CFLAGS) $(ADI_CFLAGS)
PRE_LFLAGS  := -L$(STAGEDIR)/lib -L$(STAGEDIR)/usr/lib 
POST_CFLAGS := $(REV_CFLAGS) -Iinclude/post
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
	-chmod -R +x $(ROMFSDIR)$(LIB_DIR)/script

clean:
	rm -f $(OBJS)
	rm -f $(APPS)

image:

