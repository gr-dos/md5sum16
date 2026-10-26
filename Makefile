# Build md5sum.exe for 16-bit MS-DOS with Open Watcom and libc16.
#   make                         fetch and build the pinned libc16 into
#                                build/libc16, then build md5sum.exe
#   make LIBC16_ROOT=../libc16   use a libc16 checkout built with its make
#
# md5sum.exe is compiled and linked against libc16 only (its ABI is in
# libc16's docs/ABI.md), with the Open Watcom toolchain libc16 was built
# with, named in $(LIBC16_ROOT)/build/tools.env.

LIBC16_ROOT ?= build/libc16
LIBC16      := $(LIBC16_ROOT)/build/LIBC16.LIB
WATCOM_ENV   = . $(LIBC16_ROOT)/build/tools.env && PATH=$$WATCOM/binl64:$$PATH

VERSION := $(shell cat VERSION)

CC     = wcc
CFLAGS = -q -zq -zl -zld -x -ms -0 -os -s -bt=dos -wx -we \
         -I$(LIBC16_ROOT)/include '-DVERSION="$(VERSION)"'

SRCS = md5sum.c md5.c
OBJS = $(SRCS:.c=.obj)

md5sum.exe: $(OBJS) $(LIBC16)
	$(WATCOM_ENV) && wlink option quiet, nodefaultlibs, map=md5sum.map \
	  format dos name $@ $(foreach o,$(OBJS),file $(o)) library $(LIBC16)

%.obj: %.c md5.h VERSION $(LIBC16)
	$(WATCOM_ENV) && $(CC) $(CFLAGS) -fo=$@ $<

# The pinned libc16 (scripts/fetch-libc16). A LIBC16_ROOT given on the
# command line must already be built.
ifeq ($(LIBC16_ROOT),build/libc16)
$(LIBC16): scripts/fetch-libc16
	scripts/fetch-libc16 $(LIBC16_ROOT)
endif

test: md5sum.exe
	sh tests/run.sh $(CURDIR)/md5sum.exe

clean:
	rm -f *.o *.obj *.err *.map md5sum.exe

.PHONY: test clean
