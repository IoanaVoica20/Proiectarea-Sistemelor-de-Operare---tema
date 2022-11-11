CC = gcc
.PHONY: all clean

all: build

build: libso_stdio.so

libso_stdio.so: libso_stdio.o
		$(CC) libso_stdio.o  -shared -o libso_stdio.so

libso_stdio.o: libso_stdio.c
		$(CC) -c libso_stdio.c -fPIC -o libso_stdio.o

clean:
		rm -f *.o libso_stdio.so

