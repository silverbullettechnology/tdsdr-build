ifdef PETALINUX
include $(PETALINUX)/software/petalinux-dist/tools/user-commons.mk
include $(LOGGING_MK)
APP      := dma_streamer_app minimal
APP_OBJS := \
	common/common.o \
	common/log.o \
	dsa/main.o \
	dsa/channel.o \
	dsa/command.o

CFLAGS   += -I$(CURDIR)
CFLAGS   += -I$(PETALINUX)/software/user-modules/dma_streamer_mod
CFLAGS   += -I$(PETALINUX)/software/user-modules/fifo_dev
CFLAGS   += -I$(PETALINUX)/software/user-modules/pipe_dev

# pmon disabled for now
#CFLAGS   += -I$(CURDIR)/pmon -DDSA_USE_PMON
#APP_OBJS += \
	pmon/xaxipmon.o \
	pmon/xaxipmon_g.o \
	pmon/xaxipmon_selftest.o \
	pmon/xaxipmon_sinit.o \
	pmon/pmon.o
endif

CFLAGS  += -Wall -Werror
LDFLAGS += -L$(STAGEDIR)/usr/lib
LDLIBS  := -lad9361 -lfifo_dev -lpipe_dev -ldsm -lformat

all: $(APP)

dma_streamer_app: $(APP_OBJS)
	$(CC) $(LDFLAGS) $(LDFLAGS) -o $@ $(APP_OBJS) -lm $(LDLIBS)

minimal: minimal.c common/common.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

pmon/pmon.a:
	$(MAKE) -C pmon/

clean:
	-rm -f $(APP) $(APP_OBJS)
	-$(MAKE) -C pmon/ clean

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

