CC=gcc
CFLAGS=-std=c99 -Wall -ftest-coverage -fprofile-arcs
LIBRARIES=-ledit -lgmp -lm

INTERNAL_SOURCE=internal/actor.c internal/channel.c internal/signal.c

all: libcool cool

libcool: cool.c mpc.c
	$(CC) $(CFLAGS) -shared -o libcool.so cool.c mpc.c $(INTERNAL_SOURCE) $(LIBRARIES)

cool: cool.o
	$(CC) $(CFLAGS) -o cool cooli.c libcool.so $(LIBRARIES)

clean:
	rm -rf *.so *.o cool *.gcno
