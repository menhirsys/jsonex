.SUFFIXES:

.PHONY:
run_tests: test
	./test

CFLAGS=-std=c99 -pedantic -Wall -Werror

test: $(shell git ls-files)
	$(CC) $(CFLAGS) -o $@ -lm -DDEBUG test.c jsonex.c
