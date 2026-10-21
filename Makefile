# Build md5sum.exe for 16-bit MS-DOS with Open Watcom.
#   make WATCOM=~/opt/watcom

WATCOM ?= $(HOME)/opt/watcom
export WATCOM
export INCLUDE := $(WATCOM)/h
export PATH := $(WATCOM)/binl64:$(WATCOM)/binl:$(PATH)

VERSION := $(shell cat VERSION)

CC     = wcl
CFLAGS = -q -bt=dos -ms -0 -os -s -wx -we -zq '-DVERSION="$(VERSION)"'

SRCS = md5sum.c md5.c

md5sum.exe: $(SRCS) md5.h VERSION
	$(CC) $(CFLAGS) -fe=$@ $(SRCS)

test: md5sum.exe
	sh tests/run.sh $(CURDIR)/md5sum.exe

clean:
	rm -f *.o *.obj *.err md5sum.exe

.PHONY: test clean
