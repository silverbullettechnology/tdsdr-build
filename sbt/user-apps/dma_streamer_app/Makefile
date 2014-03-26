ifdef PETALINUX
include $(PETALINUX)/software/petalinux-dist/tools/user-commons.mk
include $(LOGGING_MK)
APP      := dma_streamer_app dsa_format
APP_OBJS := dsa_main.o dsa_format.o dsa_channel.o dsa_command.o dsa_common.o log.o
APP_OBJS += dsa_ioctl.o dsa_ioctl_adi_old.o dsa_ioctl_adi_new.o
CFLAGS   += -I$(PETALINUX)/software/user-modules/dma_streamer_mod
else
APP      := dsa_format
endif

CFLAGS += -Wall -Werror
LDLIBS += -lm

all: $(APP)

dma_streamer_app: $(APP_OBJS)
	$(CC) $(LDFLAGS) -o $@ $(APP_OBJS) $(LDLIBS) -lad9361

dsa_format: dsa_format.c dsa_common.c log.c
	$(CC) $(CFLAGS) -DUNIT_TEST -o $@ $^ $(LDLIBS)

clean:
	-rm -f $(APP) *.elf *.gdb *.o

.PHONY: romfs image

# Optionally strip the final file
ifndef CONFIG_USER_DEBUG
DO_STRIP=do_strip
endif

do_strip: all
	$(STRIP) $(APP)

romfs: all $(DO_STRIP)
	mkdir -p $(ROMFSDIR)/usr/bin
	install -m755 $(APP) $(ROMFSDIR)/usr/bin

image: romfs
	make -C ${PETALINUX}/software/petalinux-dist image

%.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $<


# Targets for the required .config files - if they don't exist, the tree isn't
# configured.  Tell the user this, how to fix it, and exit.
${ROOTDIR}/config.arch ${ROOTDIR}/.config:
	@echo "Error: You must configure the PetaLinux tree before compiling your application"
	@echo ""
	@echo "Change directory to ../../petalinux-dist and 'make menuconfig' or 'make xconfig'"
	@echo ""
	@echo "Once the tree is configured, return to this directory, and re-run make."
	@echo ""
	@exit -1

