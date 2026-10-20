# Build md5sum.exe for 16-bit MS-DOS with Open Watcom.
#   make WATCOM=~/opt/watcom

WATCOM ?= $(HOME)/opt/watcom
export WATCOM
export INCLUDE := $(WATCOM)/h
export PATH := $(WATCOM)/binl64:$(PATH)

CC     = wcl
CFLAGS = -q -bt=dos -ms -0 -os -s -wx -we -zq

SRCS = md5sum.c md5.c

md5sum.exe: $(SRCS) md5.h
	$(CC) $(CFLAGS) -fe=$@ $(SRCS)

clean:
	rm -f *.o *.obj *.err md5sum.exe

.PHONY: clean
