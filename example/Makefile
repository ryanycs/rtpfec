IDIR = ../include
CC = gcc
CFALGS = -I$(IDIR) -Wall -lm

LDIR = ../lib
DEPS = utils.h $(LDIR)/libfec.so

all: server client

server: server.c
	$(CC) -o server server.c -L$(LDIR) -lfec $(CFALGS)

client: client.c
	$(CC) -o client client.c -L$(LDIR) -lfec $(CFALGS)

.PHONY: clean

clean:
	rm -f server client