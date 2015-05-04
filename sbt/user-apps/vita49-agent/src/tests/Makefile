# Unit tests Makefile
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


TEST_DEPS := #tests/log-list.o
MAIN_DEPS := lib/log.o lib/clocks.o lib/util.o lib/descript.o lib/packlist.o 
DEPS := $(MAIN_DEPS) $(TEST_DEPS)


# Library code
tests/lib/average.c/bin: lib/average.c $(DEPS)
	$(CC) $(CFLAGS) -DUNIT_TEST $(filter-out $(patsubst %.c,%.o,$<),$^) -o$@

tests/lib/buffer.c/bin: lib/buffer.c $(DEPS)
	$(CC) $(CFLAGS) -DUNIT_TEST $(filter-out $(patsubst %.c,%.o,$<),$^) -o$@

tests/lib/child.c/bin: lib/child.c $(DEPS) 
	$(CC) $(CFLAGS) -DUNIT_TEST $(filter-out $(patsubst %.c,%.o,$<),$^) -o$@

tests/lib/descript.c/bin: lib/descript.c $(DEPS)
	$(CC) $(CFLAGS) -DUNIT_TEST $(filter-out $(patsubst %.c,%.o,$<),$^) -o$@
                                        
tests/lib/growlist.c/bin: lib/growlist.c $(DEPS)
	$(CC) $(CFLAGS) -DUNIT_TEST $(filter-out $(patsubst %.c,%.o,$<),$^) -o$@
                                        
tests/lib/modlist.c/bin: lib/modlist.c $(DEPS) lib/buffer.o
	$(CC) $(CFLAGS) -DUNIT_TEST $(filter-out $(patsubst %.c,%.o,$<),$^) -o$@

tests/lib/packlist.c/bin: lib/packlist.c $(DEPS)
	$(CC) $(CFLAGS) -DUNIT_TEST $(filter-out $(patsubst %.c,%.o,$<),$^) -o$@

tests/lib/timer.c/bin: lib/timer.c $(DEPS) lib/clocks.o
	$(CC) $(CFLAGS) $(LFLAGS) -DUNIT_TEST $(filter-out $(patsubst %.c,%.o,$<),$^) -o$@ -lrt

tests/lib/util.c/bin: lib/util.c $(DEPS)
	$(CC) $(CFLAGS) -DUNIT_TEST $(filter-out $(patsubst %.c,%.o,$<),$^) -o$@

tests/lib/genlist.c/bin: lib/genlist.c $(DEPS)
	$(CC) $(CFLAGS) -DUNIT_TEST $(filter-out $(patsubst %.c,%.o,$<),$^) -o$@

tests/lib/mbuf.c/bin: lib/mbuf.c $(DEPS) lib/genlist.o
	$(CC) $(CFLAGS) -DUNIT_TEST $(filter-out $(patsubst %.c,%.o,$<),$^) -o$@

tests/lib/mqueue.c/bin: lib/mqueue.c $(DEPS) lib/genlist.o lib/mbuf.o
	$(CC) $(CFLAGS) -DUNIT_TEST $(filter-out $(patsubst %.c,%.o,$<),$^) -o$@

tests/lib/uuid.c/bin: lib/uuid.c $(DEPS) 
	$(CC) $(CFLAGS) -DUNIT_TEST $(filter-out $(patsubst %.c,%.o,$<),$^) -o$@


# Common code
tests/common/vita49/unit-tests/parse.c/bin: common/vita49/unit-tests/parse.c $(DEPS) \
	lib/mqueue.o lib/genlist.o lib/growlist.o lib/mbuf.o lib/uuid.o common/resource.o \
	common/vita49/common.o common/vita49/command.o common/vita49/context.o \
	common/vita49/types.o ../lib/config/libconfig.a
	$(CC) $(CFLAGS) -DUNIT_TEST $(filter-out $(patsubst %.c,%.o,$<),$^) -o$@ 


clean:
	find tests -type f -name 'bin' -exec rm {} +
	find tests -type f -name 'err' -exec rm {} +
	find tests -type f -name 'out' -exec rm {} +
	rm -f $(TEST_DEPS)
