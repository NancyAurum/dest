STATICLIB = libpng.a

#
# Set to 1 if shared object needs to be installed
#
SHARED_MODE=0

#LOC = -DASMV
#LOC = -DDEBUG -g

PREFIX =
CC = $(PREFIX)gcc
CFLAGS = $(LOC) -O3 -Wall -DBZ_STRICT_ANSI
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

OBJS = png.o       pngmem.o    pngrio.o    pngset.o    pngwrite.o \
       pngerror.o  pngpread.o  pngrtran.o  pngtrans.o  pngwtran.o \
       pngget.o    pngread.o   pngrutil.o  pngwio.o    pngwutil.o

OBJA =

all: $(STATICLIB)

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


