#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <linux/input.h>
#include <linux/omapfb.h>

#include "../common/platform.h"
#include "../common/input.h"
#include "../common/settings.h"
#include "../common/main.h"
#include "../common/args.h"
#include "../../video.h"
#include "../arm/asmutils.h"
#include "../arm/neon_scale2x.h"
#include "../arm/neon_eagle2x.h"
#include "../libpicofe/input.h"
#include "../libpicofe/plat.h"
#include "../libpicofe/menu.h"
#include "../libpicofe/linux/in_evdev.h"
#include "../libpicofe/linux/fbdev.h"
#include "../libpicofe/linux/xenv.h"

static int g_layer_x, g_layer_y, g_layer_w, g_layer_h;
static struct vout_fbdev *main_fb, *layer_fb;
static void *layer_buf;
static int bounce_buf[320 * 241 / 4];
static unsigned short pal[256];
static unsigned int pal2[256]; // for neon_*2x

enum scaling {
	SCALING_1X = 0,
	SCALING_PROPORTIONAL,
	SCALING_4_3,
	SCALING_FULLSCREEN,
};

enum sw_filter {
	SWFILTER_NONE = 0,
	SWFILTER_SCALE2X,
	SWFILTER_EAGLE2X,
};

static const struct in_default_bind in_evdev_defbinds[] = {
  { KEY_UP,       IN_BINDTYPE_PLAYER12, NKEYB_UP },
  { KEY_DOWN,     IN_BINDTYPE_PLAYER12, NKEYB_DOWN },
  { KEY_LEFT,     IN_BINDTYPE_PLAYER12, NKEYB_LEFT },
  { KEY_RIGHT,    IN_BINDTYPE_PLAYER12, NKEYB_RIGHT },
  { KEY_PAGEDOWN, IN_BINDTYPE_PLAYER12, NKEYB_B },
  { KEY_END,      IN_BINDTYPE_PLAYER12, NKEYB_A },
  { KEY_HOME,     IN_BINDTYPE_PLAYER12, NKEYB_B_TURBO },
  { KEY_PAGEUP,   IN_BINDTYPE_PLAYER12, NKEYB_A_TURBO },
  { KEY_LEFTCTRL, IN_BINDTYPE_PLAYER12, NKEYB_SELECT },
  { KEY_LEFTALT,  IN_BINDTYPE_PLAYER12, NKEYB_START },
  { KEY_1,        IN_BINDTYPE_EMU, EACTB_SAVE_STATE },
  { KEY_2,        IN_BINDTYPE_EMU, EACTB_LOAD_STATE },
  { KEY_3,        IN_BINDTYPE_EMU, EACTB_PREV_SLOT },
  { KEY_4,        IN_BINDTYPE_EMU, EACTB_NEXT_SLOT },
  { KEY_5,        IN_BINDTYPE_EMU, EACTB_FDS_INSERT },
  { KEY_6,        IN_BINDTYPE_EMU, EACTB_FDS_SELECT },
  { KEY_7,        IN_BINDTYPE_EMU, EACTB_INSERT_COIN },
  { KEY_SPACE,    IN_BINDTYPE_EMU, EACTB_ENTER_MENU },
  { 0, 0, 0 }
};

static int omap_setup_layer_(int fd, int enabled, int x, int y, int w, int h)
{
	struct omapfb_plane_info pi = { 0, };
	struct omapfb_mem_info mi = { 0, };
	int ret;

	ret = ioctl(fd, OMAPFB_QUERY_PLANE, &pi);
	if (ret != 0) {
		perror("QUERY_PLANE");
		return -1;
	}

	ret = ioctl(fd, OMAPFB_QUERY_MEM, &mi);
	if (ret != 0) {
		perror("QUERY_MEM");
		return -1;
	}

	/* must disable when changing stuff */
	if (pi.enabled) {
		pi.enabled = 0;
		ret = ioctl(fd, OMAPFB_SETUP_PLANE, &pi);
		if (ret != 0)
			perror("SETUP_PLANE");
	}

	if (mi.size < 256*2*240*2*3) {
		mi.size = 256*2*240*2*3;
		ret = ioctl(fd, OMAPFB_SETUP_MEM, &mi);
		if (ret != 0) {
			perror("SETUP_MEM");
			return -1;
		}
	}

	pi.pos_x = x;
	pi.pos_y = y;
	pi.out_width = w;
	pi.out_height = h;
	pi.enabled = enabled;

	ret = ioctl(fd, OMAPFB_SETUP_PLANE, &pi);
	if (ret != 0) {
		perror("SETUP_PLANE");
		return -1;
	}

	return 0;
}

static int omap_enable_layer(int enabled)
{
	return omap_setup_layer_(vout_fbdev_get_fd(layer_fb), enabled,
		g_layer_x, g_layer_y, g_layer_w, g_layer_h);
}

void platform_init(void)
{
	const char *main_fb_name, *layer_fb_name;
	int fd, ret, w, h;

	memset(&Settings, 0, sizeof(Settings));
	Settings.frameskip = 0;
	Settings.sound_rate = 44100;
	Settings.turbo_rate_add = (8*2 << 24) / 60 + 1; // 8Hz turbofire
	Settings.gamma = 100;
	Settings.sstate_confirm = 1;
	Settings.scaling = SCALING_4_3;

	main_fb_name = getenv("FBDEV_MAIN");
	if (main_fb_name == NULL)
		main_fb_name = "/dev/fb0";

	layer_fb_name = getenv("FBDEV_LAYER");
	if (layer_fb_name == NULL)
		layer_fb_name = "/dev/fb1";

	// must set the layer up first to be able to use it
	fd = open(layer_fb_name, O_RDWR);
	if (fd == -1) {
		fprintf(stderr, "%s: ", layer_fb_name);
		perror("open");
		exit(1);
	}

	g_layer_x = 80, g_layer_y = 0;
	g_layer_w = 640, g_layer_h = 480;

	ret = omap_setup_layer_(fd, 0, g_layer_x, g_layer_y, g_layer_w, g_layer_h);
	close(fd);
	if (ret != 0) {
		fprintf(stderr, "failed to set up layer, exiting.\n");
		exit(1);
	}

	xenv_init(NULL, "fceu");

	w = h = 0;
	main_fb = vout_fbdev_init(main_fb_name, &w, &h, 16, 2);
	if (main_fb == NULL) {
		fprintf(stderr, "couldn't init fb: %s\n", main_fb_name);
		exit(1);
	}

	g_menuscreen_w = w;
	g_menuscreen_h = h;
	g_menuscreen_ptr = vout_fbdev_flip(main_fb);

	w = 256*2;
	h = 240*2;
	layer_fb = vout_fbdev_init(layer_fb_name, &w, &h, 16, 3);
	if (layer_fb == NULL) {
		fprintf(stderr, "couldn't init fb: %s\n", layer_fb_name);
		goto fail0;
	}
	layer_buf = vout_fbdev_resize(layer_fb, 256, 240, 16,
		0, 0, 0, 0, 3);
	if (layer_buf == NULL) {
		fprintf(stderr, "couldn't get layer buf\n");
		goto fail1;
	}

	plat_target_init();

	XBuf = (void *)bounce_buf;

	return;

fail1:
	vout_fbdev_finish(layer_fb);
fail0:
	vout_fbdev_finish(main_fb);
	xenv_finish();
	exit(1);
}

void platform_late_init(void)
{
	in_evdev_init(in_evdev_defbinds);
	in_probe();
	plat_target_setup_input();
}

/* video */
void CleanSurface(void)
{
	memset(bounce_buf, 0, sizeof(bounce_buf));
	vout_fbdev_clear(layer_fb);
}

void KillVideo(void)
{
}

int InitVideo(void)
{
	CleanSurface();

	srendline = 0;
	erendline = 239;

	return 1;
}

// 16: rrrr rggg gg0b bbbb
void FCEUD_SetPalette(uint8 index, uint8 r, uint8 g, uint8 b)
{
	unsigned int v = ((r & 0xf8) << 8) | ((g & 0xfc) << 3) | (b >> 3);
	pal[index] = v;
	pal2[index] = v;
}

void FCEUD_GetPalette(uint8 index, uint8 *r, uint8 *g, uint8 *b)
{
	unsigned int v = pal[index];
	*r = (v >> 8) & 0xf8;
	*g = (v >> 3) & 0xfc;
	*b = v << 3;
}

void BlitPrepare(int skip)
{
	unsigned char *s;
	unsigned short *d;
	int *p, i;

	if (skip)
		return;

	if (eoptions & EO_CLIPSIDES)
	{
		int i, *p = bounce_buf + 32/4;
		for (i = 240; i; i--, p += 320/4)
		{
			p[0] = p[1] = p[62] = p[63] = 0;
		}
	}

	p = bounce_buf + 32/4;
	if (srendline > 0)
		for (i = srendline; i > 0; i--, p += 320/4)
			memset(p, 0, 256);
	if (erendline < 239)
	{
		p = bounce_buf + erendline*320/4 + 32/4;
		for (i = 239-erendline; i > 0; i--, p += 320/4)
			memset(p, 0, 256);
	}

	/* this is called before throttle, so we blit here too */
	s = (unsigned char *)bounce_buf + 32;
	d = (unsigned short *)layer_buf;

	switch (Settings.sw_filter)
	{
	case SWFILTER_SCALE2X:
		neon_scale2x_8_16(s, d, pal2, 256, 320, 256*2*2, 240);
		break;
	case SWFILTER_EAGLE2X:
		neon_eagle2x_8_16(s, d, pal2, 256, 320, 256*2*2, 240);
		break;
	case SWFILTER_NONE:
	default:
		for (i = 0; i < 239; i++, d += 256, s += 320)
			do_clut(d, s, pal, 256);
		break;
	}
	
	layer_buf = vout_fbdev_flip(layer_fb);
}

void BlitScreen(int skip)
{
}

/* throttle */
extern uint8 PAL;
extern int FSkip;
static struct timeval tv_expect;
static int skip_count = 0;
static int us_interval, us_interval1024;
#define MAX_LAG_FRAMES 3

void RefreshThrottleFPS(void)
{
	skip_count = 0;
	us_interval = PAL ? 20000 : 16667;
	us_interval1024 = PAL ? 20000*1024 : 17066667;

	vout_fbdev_wait_vsync(layer_fb);
	gettimeofday(&tv_expect, 0);
	tv_expect.tv_usec *= 1024;
}

void SpeedThrottle(void)
{
	struct timeval now;
	int diff;

	gettimeofday(&now, 0);

	// usec*1024 units to prevent drifting
	tv_expect.tv_usec += us_interval1024;
	if (tv_expect.tv_usec > 1000000 * 1024) {
		tv_expect.tv_usec -= 1000000 * 1024;
		tv_expect.tv_sec++;
	}

	diff = (tv_expect.tv_sec - now.tv_sec) * 1000000 +
		(tv_expect.tv_usec >> 10) - now.tv_usec;

	if (Settings.frameskip >= 0)
	{
		if (skip_count >= Settings.frameskip)
			skip_count = 0;
		else {
			skip_count++;
			FSkip = 1;
		}
	}
	else 
	{
		FSkip = diff < -us_interval;
	}

	if (diff > MAX_LAG_FRAMES * us_interval
	    || diff < -MAX_LAG_FRAMES * us_interval)
	{
		//printf("reset %d\n", diff);
		RefreshThrottleFPS();
		return;
	}
	else if (diff > us_interval)
	{
		usleep(diff - us_interval);
	}
}

/* called just before emulation */
void platform_apply_config(void)
{
	static int old_layer_w, old_layer_h;
	int fb_w = 256, fb_h = 240;
	float mult;

	if (Settings.sw_filter == SWFILTER_SCALE2X
	    || Settings.sw_filter == SWFILTER_EAGLE2X)
	{
		fb_w *= 2;
		fb_h *= 2;
	}

	if (fb_w != old_layer_w || fb_h != old_layer_h)
	{
		layer_buf = vout_fbdev_resize(layer_fb, fb_w, fb_h, 16,
				0, 0, 0, 0, 3);
		if (layer_buf == NULL) {
			fprintf(stderr, "fatal: mode change %dx%x -> %dx%d failed\n",
				old_layer_w, old_layer_h, fb_w, fb_h);
			exit(1);
		}

		old_layer_w = fb_w;
		old_layer_h = fb_h;
	}

	switch (Settings.scaling)
	{
	case SCALING_1X:
		g_layer_w = fb_w;
		g_layer_h = fb_h;
		break;
	case SCALING_PROPORTIONAL:
		// assumes screen width > height
		mult = (float)g_menuscreen_h / fb_h;
		g_layer_w = (int)(fb_w * mult);
		g_layer_h = g_menuscreen_h;
		break;
	case SCALING_FULLSCREEN:
		g_layer_w = g_menuscreen_w;
		g_layer_h = g_menuscreen_h;
		break;
	case SCALING_4_3:
	default:
		g_layer_w = g_menuscreen_h * 4 / 3;
		g_layer_h = g_menuscreen_h;
		break;
	}
	if (g_layer_w > g_menuscreen_w)
		g_layer_w = g_menuscreen_w;
	if (g_layer_h > g_menuscreen_h)
		g_layer_h = g_menuscreen_h;
	g_layer_x = g_menuscreen_w / 2 - g_layer_w / 2;
	g_layer_y = g_menuscreen_h / 2 - g_layer_h / 2;

	plat_target_set_filter(Settings.hw_filter);
	plat_target_set_lcdrate(PAL);

	// adjust since we run at 60Hz, and real NES doesn't
	FCEUI_Sound(Settings.sound_rate + Settings.sound_rate / 980);

	omap_enable_layer(1);
}

void platform_set_volume(int val)
{
}

void plat_video_menu_enter(int is_rom_loaded)
{
	unsigned short *d = g_menubg_src_ptr;
	unsigned char *s, *sr = (void *)bounce_buf;
	int u, v = 240 / 2;
	int x, y, w;

	omap_enable_layer(0);

	if (!fceugi)
		return;

	for (y = 0; y < g_menuscreen_h; y++)
	{
		s = sr + v * 320 + 32;
		u = 256 / 2;
		for (x = 0; x < g_menuscreen_w; )
		{
			w = g_menuscreen_w - x;
			if (w > 256 - u)
				w = 256 - u;
			do_clut(d + x, s + u, pal, w);
			u = (u + w) & 255;
			x += w;
		}
		d += x;
		v++;
		if (v > erendlinev[PAL])
			v = srendlinev[PAL];
	}
}

void plat_video_menu_begin(void)
{
}

void plat_video_menu_end(void)
{
	g_menuscreen_ptr = vout_fbdev_flip(main_fb);
}

void plat_video_menu_leave(void)
{
	memset(g_menuscreen_ptr, 0, g_menuscreen_w * g_menuscreen_h * 2);
	g_menuscreen_ptr = vout_fbdev_flip(main_fb);
	// layer enabled in platform_apply_config()
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

void platform_get_def_rompath(char *buf, int size)
{
	strcpy(buf, "/media");
}

void platform_finish(void)
{
	omap_enable_layer(0);
	vout_fbdev_finish(layer_fb);
	vout_fbdev_finish(main_fb);
	xenv_finish();
}
