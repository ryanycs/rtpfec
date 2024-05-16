IDIR = include
CC = gcc
CFALGS = -I$(IDIR) -fPIC

DEPS = $(IDIR)/fec.h $(IDIR)/galois.h
OBJ = fec.o galois.o
TARGET = libfec.so

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) -shared -o $@ $^

%.o: src/%.c $(DEPS)
	$(CC) -c -o $@ $< $(CFALGS)

.PHONY: clean

clean:
	rm -f $(OBJ) $(TARGET)