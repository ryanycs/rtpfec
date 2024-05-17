IDIR = ../include
CC = gcc
CFALGS = -I$(IDIR) -fPIC

LDIR = ../lib

all: test-encode

test-encode: test-encode.c $(LDIR)/libfec.so
	$(CC) -o $@ $< -L$(LDIR) -lfec $(CFALGS)

.PHONY: clean

clean:
	rm -f test-encode