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

	if (mi.size < 640*512*3*3) {
		mi.size = 640*512*3*3;
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

	w = 640;
	h = 512;
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
	omap_enable_layer(1);

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
	pal[index] = ((r & 0xf8) << 8) | ((g & 0xfc) << 3) | (b >> 3);
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

	if (Settings.accurate_mode)
	{
		int i, *p = bounce_buf + 32/4;
		if (srendline > 0)
			for (i = srendline; i > 0; i--, p += 320/4)
				memset(p, 0, 256);
		if (erendline < 239)
		{
			int *p = bounce_buf + erendline*320/4 + 32/4;
			for (i = 239-srendline; i > 0; i--, p += 320/4)
				memset(p, 0, 256);
		}
	}
}

void BlitScreen(int skip)
{
	char *s = (char *)bounce_buf + 32;
	short *d = (short *)layer_buf;
	int i;

	if (skip)
		return;

	for (i = 0; i < 239; i++, d += 256, s += 320)
		do_clut(d, s, pal, 256);
	
	layer_buf = vout_fbdev_flip(layer_fb);
}

void platform_apply_config(void)
{
}

void platform_set_volume(int val)
{
}

void plat_video_menu_enter(int is_rom_loaded)
{
	omap_enable_layer(0);
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

	omap_enable_layer(1);
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
	omap_enable_layer(0);
	vout_fbdev_finish(layer_fb);
	vout_fbdev_finish(main_fb);
	xenv_finish();
}
