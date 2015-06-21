# File    : Makefile
# Descript: Configuration file parser
# License : Copyright (C) 2010,2011 Morgan Hughes <morgan.hughes@silver-bullet-tech.com>
# vim:ts=4:noexpandtab

ifneq ($(PETALINUX),)
include $(PETALINUX)/software/petalinux-dist/tools/user-commons.mk
include $(LOGGING_MK)
endif

LIBS   = 
OBJS   = src/config_buffer.o src/config_parse.o

CFLAGS += -Wall -Werror -fpic
CFLAGS += -Iinclude
LFLAGS += -shared -s

# fail on any error 
CFLAGS += -DNDEBUG

# version numbers
MAJOR_VERSION := 0
MINOR_VERSION := 1
PATCH_VERSION := 0

# output names
NAME   = config
ANAME  = lib$(NAME).a
LDNAME = lib$(NAME).so
SONAME = $(LDNAME).$(MAJOR_VERSION)
OUTPUT = $(SONAME).$(MINOR_VERSION).$(PATCH_VERSION)


# normal build targets
.PHONY: all install
all install: $(ANAME) $(LDNAME) $(SONAME) $(OUTPUT)
	# Install libraries to stage dir - shared and static
	mkdir -p $(STAGEDIR)/usr/lib
	cp -va $(OUTPUT) $(STAGEDIR)/usr/lib
	cp -va $(SONAME) $(STAGEDIR)/usr/lib
	cp -va $(LDNAME) $(STAGEDIR)/usr/lib
	cp -va $(ANAME)  $(STAGEDIR)/usr/lib
	# Install includes to stage dir
	mkdir -p $(STAGEDIR)/usr/include/$(NAME)
	cp -va include/$(NAME).h  $(STAGEDIR)/usr/include/$(NAME)


$(ANAME): $(OBJS)
	$(AR) rcs $@ $^

$(OUTPUT): $(OBJS)
	$(CC) $(LFLAGS) -Wl,-soname,$(SONAME) -Wl,--version-script,src/config.v \
		-o$(OUTPUT) $(OBJS) $(LIBS)

$(SONAME): $(OUTPUT)
	ln -sf $(OUTPUT) $(SONAME)

$(LDNAME): $(OUTPUT)
	ln -sf $(OUTPUT) $(LDNAME)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

romfs: install
	# Install shared libraries to rootfs
	mkdir -p "$(ROMFSDIR)/usr/lib"
	cp -a $(STAGEDIR)/usr/lib/$(SHARED) "$(ROMFSDIR)/usr/lib"
	cp -a $(STAGEDIR)/usr/lib/$(SONAME) "$(ROMFSDIR)/usr/lib"
	cp -a $(STAGEDIR)/usr/lib/$(LDNAME) "$(ROMFSDIR)/usr/lib"
ifeq ($(CONFIG_USER_LIBS_PIPE_DEV_STATIC),y)
	cp -a $(STAGEDIR)/usr/lib/$(ANAME)  "$(ROMFSDIR)/usr/lib"
endif
ifeq ($(CONFIG_USER_LIBS_PIPE_DEV_INCLUDES),y)
	mkdir -p "$(ROMFSDIR)/usr/include"
	cp -a $(NAME).h  "$(ROMFSDIR)/usr/include"
endif
	true

clean:
	rm -f $(OBJS) 
	rm -f $(ANAME)
	rm -f $(OUTPUT)
	rm -f $(SONAME)
	rm -f $(LDNAME)

.PHONY: all install clean

