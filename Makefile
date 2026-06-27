CC := gcc
CFLAGS := -std=c11 -Wall -Wextra -pedantic

.PHONY: default grade clean

default: spireslayer

spireslayer: src/main.c
	$(CC) $(CFLAGS) -o $@ $<

grade:
	python3 test/grader.py ./spireslayer test-cases

clean:
	rm -f spireslayer
