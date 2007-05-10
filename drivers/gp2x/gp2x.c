#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../driver.h"
#include "../common/config.h"
#include "../common/args.h"
#include "gp2x.h"
#include "gp2x-video.h"
#ifdef NETWORK
#include "unix-netplay.h"
#endif

#include "minimal.h"
#include "cpuctrl.h"
#include "squidgehack.h"

int CLImain(int argc, char *argv[]);

//#define SOUND_RATE 44100
#define SOUND_RATE 22050
#define GP2X_PORT_VERSION "0.4"

DSETTINGS Settings;
CFGSTRUCT DriverConfig[]={
        AC(Settings.sound),
        ACA(Settings.joyBMap),
	ACA(Settings.joyAMap),
        ACA(Settings.joy),
	AC(Settings.showfps),
	AC(Settings.scaling),
	AC(Settings.frameskip),
	AC(Settings.sstate_confirm),
	AC(Settings.region_force),
	AC(Settings.cpuclock),
	AC(Settings.mmuhack),
	AC(Settings.ramtimings),
	// TODO
        ENDCFGSTRUCT
};


char *DriverUsage=
"-joyx   y       Use joystick y as virtual joystick x.\n\
-sound x        Sound.\n\
                 0 = Disabled.\n\
                 Otherwise, x = playback rate.\n\
-showfps x      Display fps counter if x is nonzero\n\
-mmuhack x      Enable squidge's MMU hack if x is nonzero (GP2X).\n\
-ramtimings x   Enable RAM overclocking if x is nonzero (GP2X).\n\
"
#ifdef NETWORK
"-connect s      Connect to server 's' for TCP/IP network play.\n\
-server         Be a host/server for TCP/IP network play.\n\
-netport x      Use TCP/IP port x for network play."
#endif
;

#ifdef NETWORK
static int docheckie[2]={0,0};
#endif
ARGPSTRUCT DriverArgs[]={
         {"-joy1",0,&Settings.joy[0],0},{"-joy2",0,&Settings.joy[1],0},
         {"-joy3",0,&Settings.joy[2],0},{"-joy4",0,&Settings.joy[3],0},
         {"-sound",0,&Settings.sound,0},
         {"-showfps",0,&Settings.showfps,0},
         {"-mmuhack",0,&Settings.mmuhack,0},
         {"-ramtimings",0,&Settings.ramtimings,0},
         {"-menu",0,&ext_menu,0x4001},
         {"-menustate",0,&ext_state,0x4001},
	 #ifdef NETWORK
         {"-connect",&docheckie[0],&netplayhost,0x4001},
         {"-server",&docheckie[1],0,0},
         {"-netport",0,&Port,0},
	 #endif
         {0,0,0,0}
};



void GetBaseDirectory(char *BaseDirectory)
{
 strcpy(BaseDirectory, "fceultra");
}

static void SetDefaults(void)
{
 memset(&Settings,0,sizeof(Settings));
 Settings.cpuclock = 150;
 Settings.frameskip = -1; // auto
 Settings.mmuhack = 1;
 Settings.sound=SOUND_RATE;
}

void DoDriverArgs(void)
{
        int x;

	#ifdef NETWORK
        if(docheckie[0])
         netplay=2;
        else if(docheckie[1])
         netplay=1;

        if(netplay)
         FCEUI_SetNetworkPlay(netplay);
	#endif

        for(x=0;x<4;x++)
         if(!Settings.joy[x])
	 {
	  memset(Settings.joyBMap[x],0,sizeof(Settings.joyBMap[0]));
	  memset(Settings.joyAMap[x],0,sizeof(Settings.joyAMap[0]));
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

extern int swapbuttons; // TODO: rm
char **g_argv;


// TODO: cleanup
int main(int argc, char *argv[])
{
	int ret;
	g_argv = argv;

        puts("Starting GPFCE - Port version " GP2X_PORT_VERSION " (" __DATE__ ")");
        puts("Based on FCE Ultra "VERSION_STRING"...");
        puts("Ported by Zheng Zhu");
        puts("Additional optimization/misc work by notaz\n");

    	gp2x_init();
	cpuctrl_init();

        // unscale the screen, in case this is bad.
	gp2x_video_changemode(8);
        gp2x_video_RGB_setscaling(0, 320, 240);

        SetDefaults();

        ret = CLImain(argc,argv);

        // unscale the screen, in case this is bad.
        gp2x_video_RGB_setscaling(0, 320, 240);

	cpuctrl_deinit();
        gp2x_deinit();

        return(ret?0:-1);
}


int mmuhack_status = 0;

/* optional GP2X stuff to be done after config is loaded */
void gp2x_opt_setup(void)
{
	if (Settings.mmuhack) {
		int ret = mmuhack();
		printf("squidge hack code finished and returned %i\n", ret); fflush(stdout);
		mmuhack_status = ret;
	}
	if (Settings.ramtimings) {
		printf("setting RAM timings.. "); fflush(stdout);
		// craigix: --trc 6 --tras 4 --twr 1 --tmrd 1 --trfc 1 --trp 2 --trcd 2
		set_RAM_Timings(6, 4, 1, 1, 1, 2, 2);
		printf("done.\n"); fflush(stdout);
	}
}

