ifdef MIYOO
CROSS_COMPILE ?= arm-linux-
endif
AS	= $(CROSS_COMPILE)as
CC	= $(CROSS_COMPILE)gcc
STRIP	= $(CROSS_COMPILE)strip
TFLAGS  += -Winline -Izlib -DLSB_FIRST -DUNIX -DPSS_STYLE=1 -DHAVE_ASPRINTF -DZLIB -DFRAMESKIP -D_REENTRANT `sdl-config --cflags`
TFLAGS  += -Wno-pointer-sign -Wno-parentheses
#TFLAGS += -DFPS
RM	= rm -f
A   = drivers/arm/
C	= drivers/common/
L	= drivers/libpicofe/

#DEBUG   = 1

ifdef DEBUG
TFLAGS	+= -ggdb
LDRIVER	+= -ggdb
NOSTRIP = 1
else
ifdef MIYOO
TFLAGS	+= -Ofast -fdata-sections -ffunction-sections -fsingle-precision-constant -fno-PIC -flto
LDRIVER	+= -Ofast #-pg -fno-omit-frame-pointer
else
TFLAGS	+= -O3 -ftracer -fstrength-reduce -funroll-loops -fomit-frame-pointer -fstrict-aliasing -ffast-math
LDRIVER	+= -O3 #-pg -fno-omit-frame-pointer
endif
endif

#NOSTRIP = 1


all:		fceu

include zlib/Makefile

OBJDRIVER = drivers/sdl/sdl.o drivers/sdl/throttle.o \
	${L}fonts.o ${L}readpng.o ${L}input.o ${L}config_file.o ${L}in_sdl.o \
	${L}linux/plat.o ${L}linux/sndout_oss.o \
	${C}main.o ${C}menu.o ${C}sound-oss.o \
	${C}cheat.o ${C}config.o ${C}args.o ${C}vidblit.o ${C}unix-netplay.o \
	${UNZIPOBJS} \
	ppu.o movie.o fceu098.o ppu098.o

ifdef MIYOO
OBJDRIVER += ${A}asmutils.o
endif
LDRIVER += -lm -lz -lpng `sdl-config --libs`

OBJDRIVER += x6502.o

x6502.o: x6502.c x6502.h ops.h fce.h sound.h dprintf.h

include Makefile.base

${C}menu.o:		${C}revision.h
ppu.o:			ppu.c ppu.h

drivers/sdl/sdl.o: ${L}menu.h
${L}menu.h:
	@echo "please run: git submodule init; git submodule update"
	@false

${C}revision.h: FORCE
	@(git describe || echo) | sed -e 's/.*/#define REV "\0"/' > $@_
	@diff -q $@_ $@ > /dev/null 2>&1 || cp $@_ $@
	@rm $@_
.PHONY: FORCE

include Makefile.common

# vim:filetype=make
