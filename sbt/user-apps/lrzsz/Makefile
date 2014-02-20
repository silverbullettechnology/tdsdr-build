include $(PETALINUX)/software/petalinux-dist/tools/user-commons.mk
include $(LOGGING_MK)

VER=lrzsz-0.12.20

CONF_OPTS=--prefix=$(PREFIX)

ifeq ($(CONFIG_PETALINUX_MMU),y)
CONF_OPTS += --disable-static
CFLAGS+=-fPIC
else
CONF_OPTS += --disable-shared --enable-static
endif

HOST_NCPU:=1
include $(ROOTDIR)/tools/autotools.mk

romfs: all
	# Use $(ROMFSINST) to copy data into the target
	# E.g. there is lrzsz user app in the build-install/usr/bin directory, and I want
	# to copy it into the target "/bin" directory:
	install -m755 build-install/bin/lrz $(ROMFSDIR)/usr/bin/rz
	install -m755 build-install/bin/lsz $(ROMFSDIR)/usr/bin/sz

image: romfs
	make -C ${PETALINUX}/software/petalinux-dist image

