# File    : Makefile
# Descript: Configuration file parser
# License : Copyright (C) 2010,2011 Morgan Hughes <morgan.hughes@silver-bullet-tech.com>
# vim:ts=4:noexpandtab


CFLAGS := -Wall -Werror -I../include


all: config_buffer.out config_parse.out config_lib.out
	cmp config_buffer.want config_buffer.out
	cmp config_parse.want  config_parse.out
	cmp config_lib.want    config_lib.out

clean:
	rm -f *.o *.bin *.out *.err


# config_buffer unit test
config_buffer.out: config_buffer.bin config_buffer.test
	./config_buffer.bin config_buffer.test > $@ 2>config_buffer.err

config_buffer.bin: ../src/config_buffer.c ../include/config.h
	$(CC) $(CFLAGS) -DUNIT_TEST ../src/config_buffer.c -o$@


# config_buffer unit test
config_parse.out: config_parse.bin config_parse.test
	./config_parse.bin config_parse.test > $@ 2>config_parse.err

config_parse.bin: ../src/config_parse.c ../include/config.h
	$(CC) $(CFLAGS) -c ../src/config_buffer.c
	$(CC) $(CFLAGS) -DUNIT_TEST ../src/config_parse.c config_buffer.o -o$@


# library system test
config_lib.out: config_lib.bin config_lib.test
	./config_lib.bin config_lib.test > $@ 2>config_lib.err

config_lib.bin: ../libconfig.a config_lib.c ../include/config.h
	$(CC) $(CFLAGS) config_lib.c ../libconfig.a -o$@


