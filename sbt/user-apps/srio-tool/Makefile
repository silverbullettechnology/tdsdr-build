include $(PETALINUX)/software/petalinux-dist/tools/user-commons.mk
include $(LOGGING_MK)
APP      := srio-tool
APP_OBJS := st_main.o st_term.o

CFLAGS   += -I$(PETALINUX)/software/user-modules/srio_dev

CFLAGS += -Wall -Werror
LDLIBS += -s

all: $(APP)

srio-tool: $(APP_OBJS)
	$(CC) $(LDFLAGS) -o $@ $(APP_OBJS) $(LDLIBS)

clean:
	-rm -f $(APP) *.elf *.gdb *.o

.PHONY: romfs image

romfs: all $(DO_STRIP)
	mkdir -p $(ROMFSDIR)/bin
	install -m755 $(APP) $(ROMFSDIR)/bin

image: romfs
	make -C ${PETALINUX}/software/petalinux-dist image

%.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $<

