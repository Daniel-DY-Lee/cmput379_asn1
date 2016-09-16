# http://stackoverflow.com/questions/1484817/how-do-i-make-a-simple-makefile-for-gcc-on-linux

TARGET = asn1
LIBS = 
CC = gcc
CFLAGS = -g -Wall -m32

.PHONY: default all clean

default: $(TARGET)
all: default

OBJECTS = $(patsubst %.c, %.o, $(wildcard *.c))
HEADERS = $(wildcard *.h)

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

.PRECIOUS: $(TARGET) $(OBJECTS)

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -Wall $(LIBS) -o $@

clean:
	-rm -f *.o
	-rm -f $(TARGET)
	-rm -f *~

run:
	./asn1

fun: 
	make clean
	make 
