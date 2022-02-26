all: simple

simple: macros.h stats.h stats.c simple.c Makefile
	gcc --std=gnu2x -Wall -Wextra -g stats.c simple.c -o simple

clean:
	rm simple

.PHONY: clean
