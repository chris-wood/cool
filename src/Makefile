CC=gcc
CFLAGS=-std=c99 -Wall
LIBRARIES=-ledit -lm

all: cool cooli

cool: cool.c
	$(CC) $(CFLAGS) -c cool.c mpc.c 

cooli: cooli.c cool
	$(CC) $(CFLAGS) cooli.c cool.o mpc.o -o cooli $(LIBRARIES)