CC := gcc
CFLAGS := -std=c11 -Wall -Wextra -pedantic

.PHONY: default grade clean

default: spireslayer

spireslayer: src/main.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f spireslayer
