CC=gcc
CFLAGS=-std=c99 -Wall
LIBRARIES=-ledit -lm -lcmocka

all: test_cool_value test_cool_environment

test_cool_value: test_cool_value.c
	$(CC) $(CFLAGS) test_cool_value.c -o test_cool_value $(LIBRARIES) 

test_cool_environment: test_cool_environment.c
	$(CC) $(CFLAGS) test_cool_environment.c -o test_cool_environment $(LIBRARIES) 	


