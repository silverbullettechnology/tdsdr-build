include $(PETALINUX)/software/petalinux-dist/tools/user-commons.mk
include $(LOGGING_MK)
APPS      := srio-tool srio-send srio-recv srio-loop
TOOL_OBJS := st_main.o st_term.o common.o
SEND_OBJS := send_main.o st_term.o common.o
RECV_OBJS := recv_main.o st_term.o common.o
LOOP_OBJS := loop_main.o st_term.o common.o

TOOL_LIBS := -lrt
SEND_LIBS := -ldsm -lfifo_dev -lpipe_dev -lformat
RECV_LIBS := -ldsm -lfifo_dev -lpipe_dev -lformat
LOOP_LIBS := -ldsm -lfifo_dev -lpipe_dev -lformat

CFLAGS += -I$(PETALINUX)/software/user-modules/pipe_dev
CFLAGS += -I$(PETALINUX)/software/user-modules/srio_dev
CFLAGS += -I$(PETALINUX)/software/user-modules/dma_streamer_mod

CFLAGS  += -Wall -Werror
LDFLAGS += -s


all: $(APPS)

srio-tool: $(TOOL_OBJS)
	$(CC) $(LDFLAGS) -o $@ $(TOOL_OBJS) $(TOOL_LIBS)

srio-send: $(SEND_OBJS)
	$(CC) $(LDFLAGS) -o $@ $(SEND_OBJS) $(SEND_LIBS)

srio-recv: $(RECV_OBJS)
	$(CC) $(LDFLAGS) -o $@ $(RECV_OBJS) $(RECV_LIBS)

srio-loop: $(LOOP_OBJS)
	$(CC) $(LDFLAGS) -o $@ $(LOOP_OBJS) $(LOOP_LIBS)

clean:
	-rm -f $(APPS) *.elf *.gdb *.o

.PHONY: romfs image

romfs: all $(DO_STRIP)
	mkdir -p $(ROMFSDIR)/bin
	install -m755 $(APPS) $(ROMFSDIR)/bin

image: romfs
	make -C ${PETALINUX}/software/petalinux-dist image

%.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $<

