CC=gcc
CFLAGS=-std=c99 -Wall
LIBRARIES=-ledit -lm

all: cool cooli

cool: cool.c mpc.c
	$(CC) $(CFLAGS) -shared -o cool.so cool.c mpc.c 

cooli: cooli.o
	$(CC) $(CFLAGS) -o cooli cooli.c cool.so $(LIBRARIES)
