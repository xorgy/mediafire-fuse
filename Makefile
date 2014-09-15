CFLAGS = --std=c99 
DEFS = -D_REENTRANT -D_GNU_SOURCE
LIBS = -lcurl -lssl -lcrypto -ljansson
PREFIX = /usr/local

HOSTARCH = $(shell arch)

ifeq ($(HOSTARCH),x86_64)
LIB_ARCH = lib64
else
LIB_ARCH = lib
endif

makefile: all

all: mfshell

mfshell:
	gcc $(CFLAGS) $(DEFS) -o mfshell  *.c $(LIBS)

clean:
	rm -f *.o
	rm -f *.so
	rm -f mfshell

install:
	cp -f mfshell ${PREFIX}/bin
	chmod 755 ${PREFIX}/bin/mfshell
	

