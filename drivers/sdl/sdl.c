#include <SDL.h>
#include "../common/platform.h"
#include "../common/args.h"
#include "../common/settings.h"
#include "../common/input.h"
#include "../common/main.h"
#include "../libpicofe/menu.h"
#include "../libpicofe/input.h"
#include "../libpicofe/in_sdl.h"
#include "../libpicofe/plat.h"
#include "../../video.h"

static const struct in_default_bind in_sdl_defbinds[] = {
  { SDLK_UP,     IN_BINDTYPE_PLAYER12, NKEYB_UP },
  { SDLK_DOWN,   IN_BINDTYPE_PLAYER12, NKEYB_DOWN },
  { SDLK_LEFT,   IN_BINDTYPE_PLAYER12, NKEYB_LEFT },
  { SDLK_RIGHT,  IN_BINDTYPE_PLAYER12, NKEYB_RIGHT },
  { SDLK_z,      IN_BINDTYPE_PLAYER12, NKEYB_B },
  { SDLK_x,      IN_BINDTYPE_PLAYER12, NKEYB_A },
  { SDLK_a,      IN_BINDTYPE_PLAYER12, NKEYB_B_TURBO },
  { SDLK_s,      IN_BINDTYPE_PLAYER12, NKEYB_A_TURBO },
  { SDLK_c,      IN_BINDTYPE_PLAYER12, NKEYB_SELECT },
  { SDLK_v,      IN_BINDTYPE_PLAYER12, NKEYB_START },
  { SDLK_ESCAPE, IN_BINDTYPE_EMU, EACTB_ENTER_MENU },
  { 0, 0, 0 }
};

static SDL_Surface *screen;
static char fps_str[32];
static SDL_Color psdl[256];

struct plat_target plat_target;

static int changemode(int bpp)
{
	const SDL_VideoInfo *vinf;
	int flags = 0;

	vinf = SDL_GetVideoInfo();

	if(vinf->hw_available)
		flags |= SDL_HWSURFACE;

	if (bpp == 8)
		flags |= SDL_HWPALETTE;

	screen = SDL_SetVideoMode(320, 240, bpp, flags);
	if (!screen)
	{
		puts(SDL_GetError());
		return -1;
	}

	SDL_WM_SetCaption("FCE Ultra", "FCE Ultra");
	return 0;
}

void platform_init(void)
{
	memset(&Settings, 0, sizeof(Settings));
	Settings.frameskip = -1; // auto
	Settings.sound_rate = 44100;
	Settings.turbo_rate_add = (8*2 << 24) / 60 + 1; // 8Hz turbofire
	Settings.gamma = 100;
	Settings.sstate_confirm = 1;

	if (SDL_Init(SDL_INIT_NOPARACHUTE | SDL_INIT_VIDEO))
	{
		printf("Could not initialize SDL: %s.\n", SDL_GetError());
		exit(1);
	}

	g_menuscreen_w = 320;
	g_menuscreen_h = 240;
	if (changemode(8) != 0)
		exit(1);
}

void platform_late_init(void)
{
	in_sdl_init(in_sdl_defbinds);
	in_probe();
}

// video
static void flip(void)
{
	SDL_Flip(screen);
	XBuf = screen->pixels;
}

void CleanSurface(void)
{
	memset(screen->pixels, 0, 320 * 240 * screen->format->BytesPerPixel);
	flip();
}

void KillVideo(void)
{
}

int InitVideo(void)
{
	fps_str[0] = 0;

	CleanSurface();

	srendline = 0;
	erendline = 239;

	return 1;
}

void FCEUD_SetPalette(uint8 index, uint8 r, uint8 g, uint8 b)
{
	psdl[index].r = r;
	psdl[index].g = g;
	psdl[index].b = b;

	SDL_SetPalette(screen, SDL_PHYSPAL, &psdl[index], index, 1);
}

void FCEUD_GetPalette(uint8 index, uint8 * r, uint8 * g, uint8 * b)
{
	// dummy, will break snapshots
	*r = *g = *b = 0;
}

void BlitPrepare(int skip)
{
	if (skip) {
		return;
	}

	if (eoptions & EO_CLIPSIDES)
	{
		int i, *p = (int *) ((char *)screen->pixels + 32);
		for (i = 240; i; i--, p += 320/4)
		{
			p[0] = p[1] = p[62] = p[63] = 0;
		}
	}

	if (Settings.accurate_mode && Settings.scaling < 2)
	{
		int i, *p = (int *)screen->pixels + 32/4;
		if (srendline > 0)
			for (i = srendline; i > 0; i--, p += 320/4)
				memset(p, 0, 256);
		if (erendline < 239)
		{
			int *p = (int *)screen->pixels + erendline*320/4 + 32/4;
			for (i = 239-srendline; i > 0; i--, p += 320/4)
				memset(p, 0, 256);
		}
	}
}

void BlitScreen(int skip)
{
	if (!skip)
		flip();
}

void platform_apply_config(void)
{
}

void platform_set_volume(int val)
{
}

void plat_video_menu_enter(int is_rom_loaded)
{
	changemode(16);
}

void plat_video_menu_begin(void)
{
	g_menuscreen_ptr = screen->pixels;
}

void plat_video_menu_end(void)
{
	SDL_Flip(screen);
}

void plat_video_menu_leave(void)
{
	changemode(8);
	SDL_SetPalette(screen, SDL_PHYSPAL, psdl, 0, 256);
}

char *DriverUsage="";

ARGPSTRUCT DriverArgs[]={
         {0,0,0,0}
};

void DoDriverArgs(void)
{
}

void GetBaseDirectory(char *BaseDirectory)
{
	strcpy(BaseDirectory, "fceultra");
}

void platform_finish(void)
{
	SDL_Quit();
}
