include $(PETALINUX)/software/petalinux-dist/tools/user-commons.mk
include $(LOGGING_MK)
APP      := devmem
APP_OBJS := devmem.o

CFLAGS += -Wall -Werror
LDLIBS += -s

all: $(APP)

devmem: $(APP_OBJS)
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

