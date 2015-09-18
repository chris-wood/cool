CC=gcc
CFLAGS=-std=c99 -Wall -ftest-coverage -fprofile-arcs
LIBRARIES=-ledit -lgmp -lm

INTERNAL_SOURCE=internal/actor.c internal/channel.c

all: cool cooli

cool: cool.c mpc.c
	$(CC) $(CFLAGS) -shared -o cool.so cool.c mpc.c $(INTERNAL_SOURCE) $(LIBRARIES)

cooli: cooli.o
	$(CC) $(CFLAGS) -o cooli cooli.c cool.so $(LIBRARIES)

clean:
	rm -rf *.so *.o
