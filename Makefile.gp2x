CC	= arm-linux-gcc 
TFLAGS  =  -mcpu=arm920t -O3 -Izlib  -DGP2X=1 -DLSB_FIRST -DSDL -DUNIX -DPSS_STYLE=1 -DZLIB  -D_REENTRANT 
RM	= rm -f
B	= drivers/pc/

all:		fceu
	mv fceu gpfce

include zlib/Makefile

OBJDRIVER	= ${B}minimal.o ${B}sdl.o ${B}main.o ${B}throttle.o ${B}unix-netplay.o ${B}sdl-sound.o ${B}sdl-video.o ${B}sdl-joystick.o drivers/common/cheat.o drivers/common/config.o drivers/common/args.o drivers/common/vidblit.o ${UNZIPOBJS} ppu.o
LDRIVER		= -L /mnt/sd/lib  -L/mnt/sd/gp2x/usr/lib  -lm  -lpthread -lz -static 
#		     `arm-linux-sdl-config --libs` 

include Makefile.base

${B}sdl-joystick.o:	${B}sdl-joystick.c
${B}main.o:		${B}main.c ${B}main.h ${B}usage.h ${B}input.c ${B}keyscan.h
${B}sdl.o:		${B}sdl.c ${B}sdl.h
${B}sdl-video.o:	${B}sdl-video.c
${B}sdl-video.o:	${B}minimal.c
${B}sdl-sound.o:	${B}sdl-sound.c
#${B}sdl-netplay.o:	${B}sdl-netplay.c
${B}unix-netplay.o:	${B}unix-netplay.c
${B}throttle.o:         ${B}throttle.c ${B}main.h ${B}throttle.h
ppu.o:			ppu.c ppu.h

include Makefile.common
