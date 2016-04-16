include $(PETALINUX)/software/petalinux-dist/tools/user-commons.mk
include $(LOGGING_MK)

LIB := pmon.a

OBJS := \
	pmon.o \
	xaxipmon.o \
	xaxipmon_g.o \
	xaxipmon_selftest.o \
	xaxipmon_sinit.o

CFLAGS += -I$(CURDIR)
CFLAGS += -I$(PETALINUX)/software/user-modules/fifo_dev
CFLAGS += -Dxil_printf=printf

# Allow user to specify location of xparameters.h et al
ifneq ($(CONFIG_USER_MODULES_FIFO_DEV_BSP),)
	BOARD_DIR := $(CONFIG_USER_MODULES_FIFO_DEV_BSP)
else
	BOARD_DIR := $(ROOTDIR)/vendors/$(CONFIG_VENDOR)/$(CONFIG_PRODUCT)
endif
CFLAGS += -I$(BOARD_DIR)

CFLAGS += -Wall -Werror

all: $(LIB)

$(LIB): $(OBJS)
	$(AR) rcs $@ $^

clean:
	-rm -f $(LIB) $(OBJS)

