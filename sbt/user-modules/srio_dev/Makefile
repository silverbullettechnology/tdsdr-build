# 
# Makefile template for out of tree kernel modules
#

# PetaLinux-related stuff
ifndef PETALINUX
$(error You must source the petalinux/settings.sh script before working with PetaLinux)
endif

# Point to default PetaLinux root directory
ifndef ROOTDIR
ROOTDIR=$(PETALINUX)/software/petalinux-dist
endif

PATH:=$(PATH):$(ROOTDIR)/tools

UCLINUX_BUILD_USER = 1
-include $(ROOTDIR)/.config
-include $(ROOTDIR)/config/.config
-include $(ROOTDIR)/$(CONFIG_LINUXDIR)/.config
-include $(ROOTDIR)/config.arch

# Allow user to specify location of xparameters.h et al
ifneq ($(CONFIG_USER_MODULES_SRIO_DEV_BSP),)
	BOARD_DIR := $(CONFIG_USER_MODULES_SRIO_DEV_BSP)
else
	BOARD_DIR := $(ROOTDIR)/vendors/$(CONFIG_VENDOR)/$(CONFIG_PRODUCT)
endif


# We need NOPEDANTIC, else KBUILD complains
KBUILD_NOPEDANTIC=1

# Pick up the special CPU flags from config.arch
EXTRA_CFLAGS=$(ARCH_CPUFLAGS)

export ROMFSDIR=$(ROOTDIR)/romfs
export ROMFSINST=$(ROOTDIR)/tools/romfs-inst.sh
export STAGEDIR=$(ROOTDIR)/stage

export KSRC=${ROOTDIR}/$(CONFIG_LINUXDIR)
export KMISC=${ROMFSDIR}/lib/modules/kernel/drivers/misc

## END OF PETALINUX STUFF

ifneq ('$(notdir $(if $(realpath /), $(realpath ${SHELL}), ${SHELL}))','bash')
$(warning )
$(warning WARNING: $$SHELL not set to bash.)
$(warning If you experience build errors, try)
$(warning 'make SHELL=/bin/bash'.)
$(warning )
endif

ifndef CONFIG_MODULES
$(error Your PetaLinux kernel is not configured for loadable modules - please fix!)
endif

LOCALPWD=$(shell pwd)
obj-m += srio.o

srio-y := srio_dev.o sd_xparameters.o sd_fifo.o sd_desc.o
#srio-y := sd_of.o sd_xparameters.o sd_fifo.o sd_desc.o

srio-$(CONFIG_USER_MODULES_SRIO_DEV_TEST_DSXX) += dsxx/test.o dsxx/proc.o dsxx/regs.o

srio-$(CONFIG_USER_MODULES_SRIO_DEV_TEST_LOOP) += loop/test.o loop/proc.o loop/gpio.o

EXTRA_CFLAGS += -I$(PWD)

#CFLAGS_srio_exp.o += -DDEBUG
#CFLAGS_sd_fifo.o  += -DDEBUG
#CFLAGS_sd_proc.o  += -DDEBUG
EXTRA_CFLAGS += -DDEBUG

CFLAGS_sd_xparameters.o += -I $(BOARD_DIR)

EXTRA_CFLAGS += -Werror

all:  modules

clean:
	$(MAKE) -C $(KSRC) M=$(LOCALPWD) clean
	

modules:
	set
	@if [ ! -f $(KSRC)/Module.symvers ]; then \
		echo "ERROR: Please compile the Linux kernel first by \"make\""; \
	else \
		$(MAKE) -C $(KSRC) M=$(LOCALPWD) modules; \
	fi

install: $(addprefix $(DIR),$(subst .o,.ko,$(obj-m)))
	$(MAKE) -C $(KSRC) M=$(LOCALPWD) INSTALL_MOD_PATH=${ROMFSDIR} modules_install

.PHONY: romfs image

romfs: install

image: romfs
	make -C ${PETALINUX}/software/petalinux-dist image


