IDIR = ../include
CC = gcc
CFALGS = -I$(IDIR) -Wall

LDIR = ../lib
DEPS = utils.h $(LDIR)/libfec.so

all: test-encode test-decode

test-encode: test-encode.c $(DEPS)
	$(CC) -o $@ $< -L$(LDIR) -lfec $(CFALGS)

test-decode: test-decode.c $(DEPS)
	$(CC) -o $@ $< -L$(LDIR) -lfec $(CFALGS)

.PHONY: clean

clean:
	rm -f test-encode test-decode