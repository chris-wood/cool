CC=gcc
CFLAGS=-std=c99 -Wall -ftest-coverage -fprofile-arcs
LIBRARIES=-ledit -lm -lcmocka -lgmp

BINARIES=test_cool_value test_cool_environment

all: $(BINARIES)

test_cool_value: test_cool_value.c
	$(CC) $(CFLAGS) test_cool_value.c ../libcool.so -o test_cool_value $(LIBRARIES)

test_cool_environment: test_cool_environment.c
	$(CC) $(CFLAGS) test_cool_environment.c ../libcool.so -o test_cool_environment $(LIBRARIES)

check:
	./check.sh

coverage:
	./check.sh
	./coverage.sh

clean:
	rm -rf $(BINARIES)
