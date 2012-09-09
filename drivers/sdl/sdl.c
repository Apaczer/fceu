#include <SDL.h>
#include "../common/platform.h"
#include "../common/args.h"
#include "../common/settings.h"
#include "../common/input.h"
#include "../libpicofe/menu.h"
#include "../libpicofe/input.h"
#include "../libpicofe/in_sdl.h"

static const struct in_default_bind in_sdl_defbinds[] = {
  { SDLK_UP,     IN_BINDTYPE_PLAYER12, NKEYB_UP },
  { SDLK_DOWN,   IN_BINDTYPE_PLAYER12, NKEYB_DOWN },
  { SDLK_LEFT,   IN_BINDTYPE_PLAYER12, NKEYB_LEFT },
  { SDLK_RIGHT,  IN_BINDTYPE_PLAYER12, NKEYB_RIGHT },
  { SDLK_z,      IN_BINDTYPE_PLAYER12, NKEYB_B },
  { SDLK_x,      IN_BINDTYPE_PLAYER12, NKEYB_A },
  { SDLK_a,      IN_BINDTYPE_PLAYER12, NKEYB_B_TURBO },
  { SDLK_s,      IN_BINDTYPE_PLAYER12, NKEYB_A_TURBO },
  { SDLK_ESCAPE, IN_BINDTYPE_EMU, EACTB_ENTER_MENU },
  { 0, 0, 0 }
};

SDL_Surface *screen;

void platform_init(void)
{
	memset(&Settings, 0, sizeof(Settings));
	Settings.frameskip = -1; // auto
	Settings.sound_rate = 44100;
	Settings.turbo_rate_add = (8*2 << 24) / 60 + 1; // 8Hz turbofire
	Settings.gamma = 100;
	Settings.sstate_confirm = 1;

	g_menuscreen_w = 320;
	g_menuscreen_h = 240;

	// tmp
	{
		extern void gp2x_init();
	    	gp2x_init();
	}
}

void platform_late_init(void)
{
	in_sdl_init(in_sdl_defbinds);
}

void platform_apply_config(void)
{
}

void platform_set_volume(int val)
{
}

void platform_finish(void)
{
}

void plat_video_menu_enter(int is_rom_loaded)
{
	screen = SDL_SetVideoMode(320, 240, 16, 0);
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
