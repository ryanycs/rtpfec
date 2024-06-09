IDIR = ../include
CC = gcc
CFALGS = -I$(IDIR) -Wall

LDIR = ../lib
DEPS = utils.h $(LDIR)/libfec.so

all: server client ex1_server ex1_client

server: server.c
	$(CC) -o server server.c -L$(LDIR) -lfec $(CFALGS)

client: client.c
	$(CC) -o client client.c -L$(LDIR) -lfec $(CFALGS)

ex1_server: ex1_server.c
	$(CC) -o ex1_server ex1_server.c -L$(LDIR) -lfec $(CFALGS)

ex1_client: ex1_client.c
	$(CC) -o ex1_client ex1_client.c -L$(LDIR) -lfec $(CFALGS)

.PHONY: clean

clean:
	rm -f server client ex1_server ex1_client