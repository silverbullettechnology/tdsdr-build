# Main source Makefile
#
# Copyright 2014,2015 Silver Bullet Technology
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may not use this
# file except in compliance with the License.  You may obtain a copy of the License at:
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software distributed under
# the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied.  See the License for the specific language governing
# permissions and limitations under the License.
#
# vim:ts=4:noexpandtab

# output targets
BINS := \
	../bin/vita49-agent \
	../bin/test-stub \
	../bin/vita49-agent-worker \
	../bin/vita49-agent-daemon


# Daemon core
DAEMON_OBJS += \
	daemon/worker.o \
	daemon/control.o \
	daemon/config.o \
	daemon/message.o \
	daemon/main.o

# Control classes for user control
DAEMON_OBJS += \
	daemon/control/srio.o \
	daemon/control/local.o \
	daemon/control/dummy.o

# Worker classes for worker threads
DAEMON_OBJS += \
	daemon/worker/child.o \
	daemon/worker/dummy.o

# Manager 
DAEMON_OBJS += \
	daemon/resource.o \
	daemon/manager/config.o \
	daemon/manager/command.o \
	daemon/manager/control.o \
	daemon/manager/message.o

# Daemon library code - never cleaned 
DAEMON_LIBS := \
	-lsbt_common \
	-lv49_message \
	-lrt \
	-lconfig


# Worker core
WORKER_OBJS += \
	worker/config.o  \
	worker/command.o \
	worker/context.o \
	worker/message.o \
	worker/main.o

# Worker library code - never cleaned 
WORKER_LIBS := \
	-lsbt_common \
	-lv49_message \
	-lrt \
	-lconfig


# Test stub
TEST_STUB_OBJS := \
	tool/test-stub.o


# Control tool
CONTROL_OBJS := \
	tool/control/config.o \
	tool/control/main.o

CONTROL_OBJS += tool/control/sequence/disco.o
CONTROL_OBJS += tool/control/sequence/enum.o
CONTROL_OBJS += tool/control/sequence/access.o
CONTROL_OBJS += tool/control/sequence/release.o

CONTROL_LIBS := \
	-lsbt_common \
	-lv49_client \
	-lv49_message \
	-lrt \
	-lconfig


# Regular built objects to scan for unit-test code
BUILT_OBJS := $(sort $(CONTROL_OBJS) $(DAEMON_OBJS) $(TEST_STUB_OBJS) $(WORKER_OBJS))

# Dedicated objects for unit-test which don't link into an output bin
BUILT_OBJS += \
	common/vita49/unit-tests/parse.o


CFLAGS += -DLOG_CALL
CFLAGS += -D_LARGEFILE64_SOURCE
export CC CFLAGS MAKE

.PHONY: all install clean tests

all: $(BINS)
	@echo 

tests: $(BINS)
	@set -e; for d in $(patsubst %.o,%.c,$(BUILT_OBJS)); do \
		$(SHELL) tests/harness.sh $$d; \
	done

../bin/vita49-agent-daemon: $(DAEMON_OBJS)
	mkdir -p ../bin
	$(CC) $(CFLAGS) $(LDFLAGS) $^ $(DAEMON_LIBS) -o $@

../bin/vita49-agent-worker: $(WORKER_OBJS)
	mkdir -p ../bin
	$(CC) $(CFLAGS) $(LDFLAGS) $^ $(WORKER_LIBS) -o $@

../bin/vita49-agent: $(CONTROL_OBJS)
	mkdir -p ../bin
	$(CC) $(CFLAGS) $(LDFLAGS) $^ $(CONTROL_LIBS) -o $@

../bin/test-stub: $(TEST_STUB_OBJS)
	mkdir -p ../bin
	$(CC) $(CFLAGS) $(LDFLAGS) $^ $(TEST_STUB_LIBS) -o $@


install:
	mkdir -p $(DESTDIR)/usr/bin $(DESTDIR)/usr/sbin
	install -m755 ../bin/vita49-agent-daemon  $(DESTDIR)/usr/sbin
	install -m755 ../bin/vita49-agent-worker  $(DESTDIR)/usr/bin
	install -m755 ../bin/vita49-agent         $(DESTDIR)/usr/bin

clean:
	rm -f $(BUILT_OBJS)
	rm -f $(BINS)
	$(MAKE) -f tests/Makefile clean

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

