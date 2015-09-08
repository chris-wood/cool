CC=gcc
CFLAGS=-std=c99 -Wall -ftest-coverage -fprofile-arcs
LIBRARIES=-ledit -lgmp -lm

all: cool cooli

cool: cool.c mpc.c
	$(CC) $(CFLAGS) -shared -o cool.so cool.c mpc.c $(LIBRARIES)

cooli: cooli.o
	$(CC) $(CFLAGS) -o cooli cooli.c cool.so $(LIBRARIES)

clean:
	rm -rf *.so *.o
