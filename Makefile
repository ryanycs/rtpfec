IDIR = include
CC = gcc
CFALGS = -I$(IDIR) -fPIC -Wall

LDIR = lib

DEPS = $(IDIR)/fec.h $(IDIR)/galois.h
OBJ = fec.o galois.o
TARGET = libfec.so

all: $(TARGET)

$(TARGET): $(OBJ)
	[ -d $(LDIR) ] || mkdir $(LDIR)
	$(CC) -shared -o $(LDIR)/$@ $^

%.o: src/%.c $(DEPS)
	$(CC) -c -o $@ $< $(CFALGS)

.PHONY: clean

clean:
	rm -f $(OBJ) $(LDIR)/$(TARGET)