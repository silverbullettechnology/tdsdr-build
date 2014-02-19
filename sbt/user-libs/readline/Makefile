include $(PETALINUX)/software/petalinux-dist/tools/user-commons.mk
include $(LOGGING_MK)

PREFIX=/usr
VER=readline-5.2

CONF_OPTS=--prefix=$(PREFIX)

ifeq ($(CONFIG_PETALINUX_MMU),y)
CONF_OPTS += --disable-static
else
CONF_OPTS += --disable-shared --enable-static
endif

HOST_NCPU:=1
include $(ROOTDIR)/tools/autotools.mk

romfs:
	(cd $(INSTALLDIR); cp -fav * "$(ROMFSDIR)")

image:

