#include <SDL.h>
#include "../../driver.h"
#include "../common/args.h"
#include "../common/config.h"
#include "main.h"

typedef struct {
        int xres;
        int yres;
	int xscale,yscale;
	int xscalefs,yscalefs;
	int efx,efxfs;
        int fullscreen;
	int sound;
	#ifdef DSPSOUND
	int f8bit;
	#else
	int lbufsize,ebufsize;
	#endif
	int joy[4];
	int joyAMap[4][2];
	int joyBMap[4][4];
	char *fshack;
	char *fshacksave;
} DSETTINGS;

extern DSETTINGS Settings;

#define _xres Settings.xres
#define _yres Settings.yres
#define _fullscreen Settings.fullscreen
#define _sound Settings.sound
#define _f8bit Settings.f8bit
#define _xscale Settings.xscale
#define _yscale Settings.yscale
#define _xscalefs Settings.xscalefs
#define _yscalefs Settings.yscalefs
#define _efx Settings.efx
#define _efxfs Settings.efxfs
#define _ebufsize Settings.ebufsize
#define _lbufsize Settings.lbufsize
#define _fshack Settings.fshack
#define _fshacksave Settings.fshacksave

#define joyAMap Settings.joyAMap
#define joyBMap Settings.joyBMap
#define joy	Settings.joy
