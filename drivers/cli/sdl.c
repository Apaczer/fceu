#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sdl.h"
#include "sdl-video.h"
#ifdef NETWORK
#include "unix-netplay.h"
#endif

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

static int docheckie[2]={0,0};
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

static void SetDefaults(void)
{
 _xres=320;
 _yres=240;
 _fullscreen=0;
 _sound=48000;
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
 int x,y;
 uint32 t;

 t=SDL_GetMouseState(&x,&y);
 d[2]=0;
 if(t&SDL_BUTTON(1))
  d[2]|=1;
 if(t&SDL_BUTTON(3))
  d[2]|=2;
 t=PtoV(x,y); 
 d[0]=t&0xFFFF;
 d[1]=(t>>16)&0xFFFF;
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
 SDL_PumpEvents();
 return(SDL_GetKeyState(0));
}
#include "unix-basedir.h"

int main(int argc, char *argv[])
{
        puts("\nStarting FCE Ultra "VERSION_STRING"...\n");
	if(SDL_Init(0))
	{
	 printf("Could not initialize SDL: %s.\n", SDL_GetError());
	 return(-1);
	}
	SetDefaults();

	#ifdef BROKEN
        if(getenv("SDL_VIDEODRIVER"))
	{
	 if((_fshacksave=malloc(strlen(getenv("SDL_VIDEODRIVER"))+1)))
	  strcpy(_fshacksave,getenv("SDL_VIDEODRIVER"));
	}
        else
         _fshacksave=0;
	#endif

	{
	 int ret=CLImain(argc,argv);
	 SDL_Quit();
	 return(ret);
	}
}

