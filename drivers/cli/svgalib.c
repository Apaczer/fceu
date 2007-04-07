#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vga.h>
#include <vgamouse.h>
#include <vgakeyboard.h>

#include "../../driver.h"
#include "../common/args.h"
#include "../common/config.h"
#include "../common/unixdsp.h"

#include "svgalib.h"
#include "svga-video.h"
#include "lnx-joystick.h"
#include "unix-netplay.h"

static int soundo=48000;
static int f8bit=0;
static int sfragsize=7,snfrags=8;

int doptions=0;

CFGSTRUCT DriverConfig[]={
        NAC("sound",soundo),
	AC(doptions),
        AC(f8bit),
        AC(vmode),
        NACA("joybmap",joyBMap),
        ACA(joy),
        AC(snfrags),
        AC(sfragsize),
	ENDCFGSTRUCT
};


char *DriverUsage=
"-vmode x        Select video mode(all are 8 bpp).\n\
                 1 = 256x240                 5 = 640x480(\"1 per 4\")\n\
                 2 = 256x256                 6 = 256x224(with scanlines)\n\
                 3 = 256x256(with scanlines) 7 = 320x240\n\
                 4 = 640x480(with scanlines) 8 = 256x224\n\
-vsync x        Wait for the screen's vertical retrace before updating the\n\
                screen.  Refer to the documentation for caveats.\n\
                 0 = Disabled.\n\
                 1 = Enabled.\n\
-joyx y         Joystick mapped to virtual joystick x(1-4).\n\
                 0 = Disabled, reset configuration.\n\
                 Otherwise, y(1-inf) = joystick number.\n\
-sound x        Sound.\n\
                 0 = Disabled.\n\
                 Otherwise, x = playback rate.\n\
-sfragsize x    Set sound fragment size to 2^x samples.\n\
-snfrags x      Set number of sound fragments to x.\n\
-f8bit x        Force 8-bit sound.\n\
                 0 = Disabled.\n\
                 1 = Enabled.\n\
-connect s      Connect to server 's' for TCP/IP network play.\n\
-server         Be a host/server for TCP/IP network play.\n\
-netport x      Use TCP/IP port x for network play.";


static int docheckie[2]={0,0};
ARGPSTRUCT DriverArgs[]={
         {"-joy1",0,&joy[0],0},{"-joy2",0,&joy[1],0},
         {"-joy3",0,&joy[2],0},{"-joy4",0,&joy[3],0},
         {"-snfrags",0,&snfrags,0},{"-sfragsize",0,&sfragsize,0},
	 {"-vmode",0,&vmode,0},
         {"-vsync",0,&doptions,0x8000|DO_VSYNC},
 	 {"-sound",0,&soundo,0},
         {"-f8bit",0,&f8bit,0},
         {"-connect",&docheckie[0],&netplayhost,0x4001},
         {"-server",&docheckie[1],0,0},
         {"-netport",0,&Port,0},
	 {0,0,0,0}
};

void DoDriverArgs(void)
{
	int x;

        if(docheckie[0])
         netplay=2;
        else if(docheckie[1])
         netplay=1;

        if(netplay)
         FCEUI_SetNetworkPlay(netplay);

        for(x=0;x<4;x++)
         if(!joy[x]) memset(joyBMap[x],0,4*sizeof(int));
}

int InitSound(void)
{
        if(soundo)
        {
         int rate;
         if(soundo==1)
          soundo=48000;
         rate=soundo;
         if(InitUNIXDSPSound(&rate,f8bit?0:1,sfragsize,snfrags))
	 {
	  FCEUI_Sound(rate);
	  return(1);
	 }
        }
	return(0);
}

void WriteSound(int32 *Buffer, int Count, int NoWaiting)
{
	WriteUNIXDSPSound(Buffer,Count,NoWaiting);
}

void KillSound(void)
{
	KillUNIXDSPSound();
}

int InitMouse(void)
{
    vga_setmousesupport(1);
    mouse_setxrange(0,260);
    mouse_setyrange(0,260);
    mouse_setscale(1);
    return(1);
}

void KillMouse(void)
{
 mouse_close();
}

void GetMouseData(uint32 *MouseData)
{
 int z;
 mouse_update();
 MouseData[0]=mouse_getx();
 MouseData[1]=mouse_gety();
 z=mouse_getbutton();
 MouseData[2]=((z&MOUSE_LEFTBUTTON)?1:0)|((z&MOUSE_RIGHTBUTTON)?2:0);
}

#include "unix-basedir.h"

int InitKeyboard(void)
{
  if(keyboard_init()==-1)
  {
   puts("Error initializing keyboard.");
   return 0;
  }
  keyboard_translatekeys(TRANSLATE_CURSORKEYS | TRANSLATE_DIAGONAL);
  return 1;
}

int UpdateKeyboard(void)
{
 return(keyboard_update());
}

char *GetKeyboard(void)
{
 return(keyboard_getstate());
}

void KillKeyboard(void)
{
 keyboard_close();
}

int main(int argc, char *argv[])
{
        puts("\nStarting FCE Ultra "VERSION_STRING"...\n");
        vga_init();
	return(CLImain(argc,argv));
}
