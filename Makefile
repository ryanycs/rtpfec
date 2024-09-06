IDIR = include
CC = gcc
CFALGS = -I$(IDIR) -fPIC -Wall

ODIR = build
LDIR = lib

DEPS = $(IDIR)/fec.h $(IDIR)/galois.h
OBJ = $(ODIR)/fec.o $(ODIR)/galois.o
TARGET = libfec.so

all: $(TARGET)

$(TARGET): $(OBJ)
	[ -d $(LDIR) ] || mkdir $(LDIR)
	$(CC) -shared -o $(LDIR)/$@ $^

$(ODIR)/%.o: src/%.c $(DEPS)
	[ -d $(ODIR) ] || mkdir $(ODIR)
	$(CC) -c -o $@ $< $(CFALGS)

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o $(LDIR)/$(TARGET)
	rm -rf $(ODIR) $(LDIR)