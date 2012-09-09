#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../driver.h"
#include "../common/config.h"
#include "../common/args.h"
#include "../common/platform.h"
#include "../common/settings.h"
#include "../common/revision.h"
#include "gp2x-video.h"
#ifdef NETWORK
#include "unix-netplay.h"
#endif

#include "minimal.h"
#include "cpuctrl.h"
#include "squidgehack.h"

extern uint8 PAL;

char *DriverUsage=
"-mmuhack x      Enable squidge's MMU hack if x is nonzero (GP2X).\n\
-ramtimings x   Enable RAM overclocking if x is nonzero (GP2X).\n\
";

ARGPSTRUCT DriverArgs[]={
         {"-mmuhack",0,&Settings.mmuhack,0},
         {"-ramtimings",0,&Settings.ramtimings,0},
         {"-menu",0,&ext_menu,0x4001},
         {"-menustate",0,&ext_state,0x4001},
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
 Settings.sound_rate = 22050;
 Settings.turbo_rate_add = (8*2 << 24) / 60 + 1; // 8Hz turbofire
 Settings.gamma = 100;
 Settings.sstate_confirm = 1;
 // default controls, RLDU SEBA
 Settings.KeyBinds[ 0] = 0x010; // GP2X_UP
 Settings.KeyBinds[ 4] = 0x020; // GP2X_DOWN
 Settings.KeyBinds[ 2] = 0x040; // GP2X_LEFT
 Settings.KeyBinds[ 6] = 0x080; // GP2X_RIGHT
 Settings.KeyBinds[13] = 0x001; // GP2X_B
 Settings.KeyBinds[14] = 0x002; // GP2X_X
 Settings.KeyBinds[12] = 0x100; // GP2X_A
 Settings.KeyBinds[15] = 0x200; // GP2X_Y
 Settings.KeyBinds[ 8] = 0x008; // GP2X_START
 Settings.KeyBinds[ 9] = 0x004; // GP2X_SELECT
 Settings.KeyBinds[10] = 0x80000000; // GP2X_L
 Settings.KeyBinds[11] = 0x40000000; // GP2X_R
 Settings.KeyBinds[27] = 0xc0000000; // GP2X_PUSH
}

void DoDriverArgs(void)
{
}


int mmuhack_status = 0;


void platform_init(void)
{
        printf("Starting GPFCE " REV " (" __DATE__ ")\n");
        puts("Based on FCE Ultra "VERSION_STRING" and 0.98.1x versions");
        puts("Original port by Zheng Zhu");
        puts("Menu/optimization/misc work by notaz\n");

    	gp2x_init();
	cpuctrl_init();

        SetDefaults();
}

void platform_finish(void)
{
        // unscale the screen, in case it is bad.
        gp2x_video_RGB_setscaling(320, 240);

	if (mmuhack_status > 0)
		mmuunhack();

	set_gamma(100);
	cpuctrl_deinit();
        gp2x_deinit();

        return(ret?0:-1);
}

void platform_set_volume(int val) // 0-100
{
	platform_set_volume(val, val);
}

/* optional GP2X stuff to be done after config is loaded */
void platform_late_init(void)
{
	if (Settings.mmuhack) {
		int ret = mmuhack();
		printf("squidge hack code finished and returned %s\n", ret > 0 ? "ok" : "fail");
		fflush(stdout);
		mmuhack_status = ret;
	}
	if (Settings.ramtimings) {
		printf("setting RAM timings.. "); fflush(stdout);
		// craigix: --trc 6 --tras 4 --twr 1 --tmrd 1 --trfc 1 --trp 2 --trcd 2
		set_RAM_Timings(6, 4, 1, 1, 1, 2, 2);
		printf("done.\n"); fflush(stdout);
	}
}

void platform_apply_config(void)
{
	static int prev_cpuclock = 200, prev_gamma = 100, prev_vsync = 0, prev_pal = 0;
	if (Settings.cpuclock != 0 && Settings.cpuclock != prev_cpuclock)
	{
		set_FCLK(Settings.cpuclock);
		prev_cpuclock = Settings.cpuclock;
	}

	if (Settings.gamma != 0 && Settings.gamma != prev_gamma)
	{
		set_gamma(Settings.gamma);
		prev_gamma = Settings.gamma;
	}

	if (Settings.perfect_vsync != prev_vsync || (Settings.perfect_vsync && prev_pal != PAL))
	{
		if (Settings.perfect_vsync)
		{
			set_LCD_custom_rate(PAL ? LCDR_100_02 : LCDR_120_20);
			prev_pal = PAL;
		}
		else
		{
			unset_LCD_custom_rate();
		}
		prev_vsync = Settings.perfect_vsync;
	}

	gp2x_video_changemode(Settings.scaling == 3 ? 15 : 8);
	switch (Settings.scaling & 3) {
		case 0: gp2x_video_set_offs(0);  gp2x_video_RGB_setscaling(320, 240); break;
		case 1: gp2x_video_set_offs(32); gp2x_video_RGB_setscaling(256, 240); break;
		case 2: gp2x_video_set_offs(32+srendline*320); gp2x_video_RGB_setscaling(256, erendline-srendline); break;
		case 3: gp2x_video_set_offs(32); gp2x_video_RGB_setscaling(320, 240); break;
	}
}


