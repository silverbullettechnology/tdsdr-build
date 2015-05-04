# File    : tests/harness.sh
# Descript: Unit test harness
# Project : 
# Version : v0.0
# License : 
# vim:ts=4:noexpandtab

# check source file has a unit test to try
grep -qw 'UNIT_TEST' $1 || exit 0
grep -qw 'main'      $1 || exit 1

# make test dir, since git doesn't record empty dirs
mkdir -p tests/$1

# build unit test, abort on failure
$MAKE -f tests/Makefile tests/$1/bin || exit $?

# Execute filter-type tests
find tests/$1 -type f -name '[0-9]*.want' | while read want; do
	test="`echo -n $want | rev | cut -d. -f2- | rev`.test"
	cat $test | tests/$1/bin >tests/$1/out 2>tests/$1/err
	cmp $tests/$1/out $want || exit $?
done

# Execute any custom scripts found there
find tests/$1 -type f -name '[0-9]*.sh' | while read script; do
	sh $script || exit $?
done

