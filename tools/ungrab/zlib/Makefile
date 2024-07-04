# Makefile for zlib, derived from Makefile.dj2.
# Modified for mingw32 by C. Spieler, 6/16/98.
# Updated for zlib 1.2.x by Christian Spieler and Cosmin Truta, Mar-2003.
# Last updated: 1-Aug-2003.
# Tested under Cygwin and MinGW.

# Copyright (C) 1995-2003 Jean-loup Gailly.
# For conditions of distribution and use, see copyright notice in zlib.h

# To compile, or to compile and test, type:
#
#   make -fmakefile.gcc;  make test testdll -fmakefile.gcc
#
# To use the asm code, type:
#   cp contrib/asm?86/match.S ./match.S
#   make LOC=-DASMV OBJA=match.o -fmakefile.gcc
#
# To install libz.a, zconf.h and zlib.h in the system directories, type:
#
#   make install -fmakefile.gcc

# Note:
# If the platform is *not* MinGW (e.g. it is Cygwin or UWIN),
# the DLL name should be changed from "zlib1.dll".

STATICLIB = libz.a

#
# Set to 1 if shared object needs to be installed
#
SHARED_MODE=0

#LOC = -DASMV
#LOC = -DDEBUG -g

PREFIX =
CC = $(PREFIX)gcc
CFLAGS = $(LOC) -O3 -Wall
EXTRA_CFLAGS = -DNO_VIZ

AS = $(CC)
ASFLAGS = $(LOC) -Wall

LD = $(CC)
LDFLAGS = $(LOC)

AR = $(PREFIX)ar
ARFLAGS = rcs


STRIP = $(PREFIX)strip

CP = cp -fp
# If GNU install is available, replace $(CP) with install.
INSTALL = $(CP)
RM = rm -f

prefix = /usr/local
exec_prefix = $(prefix)

OBJS = adler32.o compress.o uncompr.o crc32.o deflate.o \
       inffast.o inflate.o inftrees.o trees.o trees.o zutil.o gzio.o
OBJA =

all: $(STATICLIB)

test: example.exe minigzip.exe
	./example
	echo hello world | ./minigzip | ./minigzip -d

testdll: example_d.exe minigzip_d.exe
	./example_d
	echo hello world | ./minigzip_d | ./minigzip_d -d

.c.o:
	$(CC) $(CFLAGS) $(EXTRA_CFLAGS) -c -o $@ $<

.S.o:
	$(AS) $(ASFLAGS) -c -o $@ $<

$(STATICLIB): $(OBJS) $(OBJA)
	$(AR) $(ARFLAGS) $@ $(OBJS) $(OBJA)


# BINARY_PATH, INCLUDE_PATH and LIBRARY_PATH must be set.

.PHONY: install uninstall clean


clean:
	-$(RM) $(STATICLIB)
	-$(RM) *.o
	-$(RM) *.exe
	-$(RM) foo.gz

adler32.o: zlib.h zconf.h
compress.o: zlib.h zconf.h
crc32.o: crc32.h zlib.h zconf.h
deflate.o: deflate.h zutil.h zlib.h zconf.h
example.o: zlib.h zconf.h
gzclose.o: zlib.h zconf.h
gzlib.o: zlib.h zconf.h
gzread.o: zlib.h zconf.h
gzwrite.o: zlib.h zconf.h
inffast.o: zutil.h zlib.h zconf.h inftrees.h inflate.h inffast.h
inflate.o: zutil.h zlib.h zconf.h inftrees.h inflate.h inffast.h
infback.o: zutil.h zlib.h zconf.h inftrees.h inflate.h inffast.h
inftrees.o: zutil.h zlib.h zconf.h inftrees.h
minigzip.o: zlib.h zconf.h
trees.o: deflate.h zutil.h zlib.h zconf.h trees.h
uncompr.o: zlib.h zconf.h
zutil.o: zutil.h zlib.h zconf.h
gzio.o: zlib.h zlib.h zconf.h
