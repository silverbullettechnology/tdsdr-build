# File    : Makefile
# Descript: Configuration file parser
# License : Copyright (C) 2010,2011 Morgan Hughes <morgan.hughes@silver-bullet-tech.com>
# vim:ts=4:noexpandtab

LIBS   = 
OBJS   = src/config_buffer.o src/config_parse.o

# Setup CC/CFLAGS/LFLAGS to allow override
CC          := $(CROSS)gcc
PRE_CFLAGS  := -O2 -Iinclude
PRE_LFLAGS  :=
POST_CFLAGS := -Wall -Werror -fpic
POST_LFLAGS := -shared -s

# fail on any error 
PRE_CFLAGS += -Werror
PRE_CFLAGS += -DNDEBUG

# version numbers
MAJOR_VERSION := 0
MINOR_VERSION := 1
PATCH_VERSION := 0

# output names
NAME   = libconfig
ANAME  = $(NAME).a
LDNAME = $(NAME).so
SONAME = $(LDNAME).$(MAJOR_VERSION)
OUTPUT = $(SONAME).$(MINOR_VERSION).$(PATCH_VERSION)


# normal build targets
all: $(ANAME) $(LDNAME) $(SONAME) $(OUTPUT)

$(ANAME): $(OBJS)
	$(AR) rcs $@ $^

$(OUTPUT): $(OBJS)
	$(CC) $(PRE_LFLAGS) $(LFLAGS) $(POST_LFLAGS) -Wl,-soname,$(SONAME) -Wl,--version-script,src/config.v -o$(OUTPUT) $(OBJS) $(LIBS)

$(SONAME): $(OUTPUT)
	ln -sf $(OUTPUT) $(SONAME)

$(LDNAME): $(OUTPUT)
	ln -sf $(OUTPUT) $(LDNAME)

%.o: %.c
	$(CC) $(PRE_CFLAGS) $(CFLAGS) $(POST_CFLAGS) -c $< -o $@

# unit tests
test: $(ANAME)
	$(MAKE) -C test

# install and clean targets
install: all
	mkdir -p $(DESTDIR)$(PREFIX)/lib
	install -m755 $(OUTPUT) $(SONAME) $(LDNAME) $(DESTDIR)$(PREFIX)/lib
	ln -sf $(OUTPUT) $(DESTDIR)$(PREFIX)/lib/$(SONAME)
	ln -sf $(OUTPUT) $(DESTDIR)$(PREFIX)/lib/$(LDNAME) 

clean:
	rm -f $(OBJS) 
	rm -f $(ANAME)
	rm -f $(OUTPUT)
	rm -f $(SONAME)
	rm -f $(LDNAME)
	$(MAKE) -C test clean

.PHONY: all test install clean

