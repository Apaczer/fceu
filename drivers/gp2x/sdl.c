#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "sdl.h"
#include "sdl-video.h"
#ifdef NETWORK
#include "unix-netplay.h"
#endif
#include "minimal.h"
//extern int soundvol;
int CLImain(int argc, char *argv[]);
extern int gp2x_in_sound_thread;
extern void pthread_yield(void);
extern void SetVideoScaling(int, int, int);

//#define SOUND_RATE 44100
#define SOUND_RATE 22050

DSETTINGS Settings;
CFGSTRUCT DriverConfig[]={
	AC(_xscale),
	AC(_yscale),
	AC(_xscalefs),
	AC(_yscalefs),
	AC(_efx),
	AC(_efxfs),
        AC(_sound),
	#ifdef DSPSOUND
        AC(_f8bit),
	#else
	AC(_ebufsize),
	AC(_lbufsize),
	#endif
	AC(_fullscreen),
        AC(_xres),
	AC(_yres),
        ACA(joyBMap),
	ACA(joyAMap),
        ACA(joy),
        //ACS(_fshack),
        ENDCFGSTRUCT
};

//-fshack x       Set the environment variable SDL_VIDEODRIVER to \"x\" when
//                entering full screen mode and x is not \"0\".

char *DriverUsage=
"-xres   x       Set horizontal resolution to x for full screen mode.\n\
-yres   x       Set vertical resolution to x for full screen mode.\n\
-xscale(fs) x	Multiply width by x.\n\
-yscale(fs) x	Multiply height by x.\n\
-efx(fs) x	Enable scanlines effect if x is non zero.  yscale must be >=2\n\
		and preferably a multiple of 2.\n\
-fs	 x      Select full screen mode if x is non zero.\n\
-joyx   y       Use joystick y as virtual joystick x.\n\
-sound x        Sound.\n\
                 0 = Disabled.\n\
                 Otherwise, x = playback rate.\n\
"
#ifdef DSPSOUND
"-f8bit x        Force 8-bit sound.\n\
                 0 = Disabled.\n\
                 1 = Enabled.\n\
"
#else
"-lbufsize x	Internal FCE Ultra sound buffer size. Size = 2^x samples.\n\
-ebufsize x	External SDL sound buffer size. Size = 2^x samples.\n\
"
#endif
"-connect s      Connect to server 's' for TCP/IP network play.\n\
-server         Be a host/server for TCP/IP network play.\n\
-netport x      Use TCP/IP port x for network play.";

#ifdef NETWORK
static int docheckie[2]={0,0};
#endif
ARGPSTRUCT DriverArgs[]={
         {"-joy1",0,&joy[0],0},{"-joy2",0,&joy[1],0},
         {"-joy3",0,&joy[2],0},{"-joy4",0,&joy[3],0},
	 {"-xscale",0,&_xscale,0},
	 {"-yscale",0,&_yscale,0},
	 {"-efx",0,&_efx,0},
         {"-xscalefs",0,&_xscalefs,0},
         {"-yscalefs",0,&_yscalefs,0},
         {"-efxfs",0,&_efxfs,0},
	 {"-xres",0,&_xres,0},
         {"-yres",0,&_yres,0},
         {"-fs",0,&_fullscreen,0},
         //{"-fshack",0,&_fshack,0x4001},
         {"-sound",0,&_sound,0},
	 #ifdef DSPSOUND
         {"-f8bit",0,&_f8bit,0},
	 #else
	 {"-lbufsize",0,&_lbufsize,0},
	 {"-ebufsize",0,&_ebufsize,0},
	 #endif
	 #ifdef NETWORK
         {"-connect",&docheckie[0],&netplayhost,0x4001},
         {"-server",&docheckie[1],0,0},
         {"-netport",0,&Port,0},
	 #endif
         {0,0,0,0}
};





void GetBaseDirectory(char *BaseDirectory)
{
 char *ol;

 ol="/mnt/sd/roms/nes";
 BaseDirectory[0]=0;
 if(ol)
 {
  strncpy(BaseDirectory,ol,2047);
  BaseDirectory[2047]=0;
  strcat(BaseDirectory,"/fceultra");
 }
}

static void SetDefaults(void)
{
 _xres=320;
 _yres=240;
 _fullscreen=0;
 _sound=SOUND_RATE; // 48000 wrong
 #ifdef DSPSOUND
 _f8bit=0;
 #else
 _lbufsize=10;
 _ebufsize=8;
 #endif
 _xscale=_yscale=_xscalefs=_yscalefs=1;
 _efx=_efxfs=0;
 //_fshack=_fshacksave=0;
 memset(joy,0,sizeof(joy));
}

void DoDriverArgs(void)
{
        int x;

	#ifdef BROKEN
        if(_fshack)
        {
         if(_fshack[0]=='0')
          if(_fshack[1]==0)
          {
           free(_fshack);
           _fshack=0;
          }
        }
	#endif

	#ifdef NETWORK
        if(docheckie[0])
         netplay=2;
        else if(docheckie[1])
         netplay=1;

        if(netplay)
         FCEUI_SetNetworkPlay(netplay);
	#endif

        for(x=0;x<4;x++)
         if(!joy[x])
	 {
	  memset(joyBMap[x],0,sizeof(joyBMap[0]));
	  memset(joyAMap[x],0,sizeof(joyAMap[0]));
	 }
}
int InitMouse(void)
{
 return(0);
}
void KillMouse(void){}
void GetMouseData(uint32 *d)
{
}

int InitKeyboard(void)
{
 return(1);
}

int UpdateKeyboard(void)
{
 return(1);
}

void KillKeyboard(void)
{

}

char *GetKeyboard(void)
{
 return NULL;
}

#include "unix-basedir.h"
extern int showfps;
extern int swapbuttons;

int main(int argc, char *argv[])
{

        puts("Starting GPFCE - Port version 0.2 05-29-2006");
        puts("Based on FCE Ultra "VERSION_STRING"...");
        puts("Ported by Zheng Zhu\n");

         //  stereo
    	 //gp2x_init (1000, 8, SOUND_RATE, 16, 1, 60);

    	 // mono 44khz
    	//gp2x_init (1000, 8, SOUND_RATE<<1, 16, 0, 60);
    	 // mono 22khz
    	gp2x_init (1000, 8, SOUND_RATE, 16, 0, 60);

        SetDefaults();
        int ret=CLImain(argc,argv);

        // unscale the screen, in case this is bad.
        SetVideoScaling(320, 320, 240);

        gp2x_deinit();
        // make sure sound thread has exited cleanly
        while (gp2x_in_sound_thread) pthread_yield();
        printf("Sound thread exited\n");
        printf("Exiting main().  terminated");
        if (showfps && swapbuttons)
        {
          execl("./selector","./selector","./gpfce_showfps_swapbuttons_config",NULL);
        }
        else if (showfps)
        {
          execl("./selector","./selector","./gpfce_showfps_config",NULL);
        }
        else if (swapbuttons)
        {
          execl("./selector","./selector","./gpfce_swapbuttons_config",NULL);
        }
        else
        {
          execl("./selector","./selector","./gpfce_config",NULL);
        }
        return(ret?0:-1);
}

