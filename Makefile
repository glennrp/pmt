# Sample makefile for pngcrush using gcc and GNU make.
# Revised to build with INTEL_SSE2 and ARM_NEON support
# Glenn Randers-Pehrson
# Last modified:  3 October 2016
#
# Invoke this makefile from a shell prompt in the usual way; for example:
#
#	make -f Makefile [OPTIONS=-Dsomething]
#
# This makefile builds a statically linked executable, using the bundled
# libpng and zlib code.

# macros --------------------------------------------------------------------

CC = gcc
LD = $(CC)
RM = rm -f

# On some platforms you might need to comment this out:
CFLAGS += -std=c90 

CFLAGS += -O3 -funroll-loops -fomit-frame-pointer

# use unified libpng:
CPPFLAGS = -DLIBPNG_UNIFIED

CPPFLAGS += ${OPTIONS} -I.

# We don't need these:
CPPFLAGS += -DNO_GZ

# Enable high resolution timers:
# CPPFLAGS += -DPNGCRUSH_TIMERS=11 -DPNGCRUSH_USE_CLOCK_GETTIME=1
# If you get a linking error with clock_gettime() you might need this:
# LIBS += -lrt

# Cannot use this with libpng15 and later.
# CPPFLAGS += -DINFLATE_ALLOW_INVALID_DISTANCE_TOOFAR_ARRR

LDFLAGS =
O = .o
E =

PNGCRUSH  = pngcrush

LIBS += -lm

# uncomment these 4 lines only if you are NOT using an external copy of zlib:
ZHDR = zlib.h
ZOBJS  = adler32$(O) compress$(O) crc32$(O) deflate$(O) \
	 infback$(O) inffast$(O) inflate$(O) inftrees$(O) \
	 trees$(O) uncompr$(O) zutil$(O)

# Enable ARM_NEON support
CPPFLAGS += -DPNGCRUSH_USE_ARM_NEON

# Enable MIPS-NSA support
CPPFLAGS += -DPNGCRUSH_USE_MPS_MSA

# Enable INTEL SSE support
CPPFLAGS += -DPNGCRUSH_USE_INTEL_SSE -DPNG_INTEL_SSE

# unified libpng with separate zlib *.o
OBJS  = pngcrush$(O) $(ZOBJS)

EXES = $(PNGCRUSH)$(E)

# implicit make rules -------------------------------------------------------

.c$(O): png.h pngconf.h pngcrush.h cexcept.h pngpriv.h pnglibconf.h $(ZHDR)
	$(CC) -c $(CPPFLAGS) $(CFLAGS) $<


# dependencies --------------------------------------------------------------

all:  $(EXES)

deflate$(O): deflate.c
	$(CC) -c -DTOO_FAR=32767 $(CPPFLAGS) $(CFLAGS) $<

pngcrush$(O): pngcrush.c png.h pngconf.h pngcrush.h pnglibconf.h cexcept.h \
	$(ZHDR)
	$(CC) -c $(CPPFLAGS) $(CFLAGS) $<

$(PNGCRUSH)$(E): $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

# maintenance ---------------------------------------------------------------

clean:
	$(RM) $(EXES) $(OBJS)
