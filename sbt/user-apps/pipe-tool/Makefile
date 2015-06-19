include $(PETALINUX)/software/petalinux-dist/tools/user-commons.mk
include $(LOGGING_MK)
APP      := pipe-sample pipe-transmit

PIPE_TRANSMIT_OBJS := pipe-transmit.o common.o
PIPE_SAMPLE_OBJS   := pipe-sample.o common.o

PIPE_TRANSMIT_LIBS := -lfifo_dev -lpipe_dev
PIPE_SAMPLE_LIBS   := -lfifo_dev -lpipe_dev

CFLAGS += -I$(PETALINUX)/software/user-modules/fifo_dev
CFLAGS += -I$(PETALINUX)/software/user-modules/pipe_dev
CFLAGS += -I$(PETALINUX)/software/user-modules/srio_dev

CFLAGS += -Wall -Werror
LDFLAGS += -s 


all: $(APP)

pipe-transmit: $(PIPE_TRANSMIT_OBJS)
	$(CC) $(LDFLAGS) -o $@ $(PIPE_TRANSMIT_OBJS) $(PIPE_TRANSMIT_LIBS)

pipe-sample: $(PIPE_SAMPLE_OBJS)
	$(CC) $(LDFLAGS) -o $@ $(PIPE_SAMPLE_OBJS) $(PIPE_SAMPLE_LIBS)

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

