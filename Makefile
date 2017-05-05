.SUFFIXES:

.PHONY:
run_tests: test
	./test

example: example.c jsonex.c
	$(CC) $(CFLAGS) -o $@ -lm $^

CFLAGS=-std=c99 -pedantic -Wall -Werror

test: $(shell git ls-files)
	$(CC) $(CFLAGS) -o $@ -lm -DDEBUG test.c jsonex.c
