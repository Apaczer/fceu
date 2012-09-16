/*
 * (C) Gražvydas "notaz" Ignotas, 2010-2011
 *
 * This work is licensed under the terms of any of these licenses
 * (at your option):
 *  - GNU GPL, version 2 or later.
 *  - GNU LGPL, version 2.1 or later.
 * See the COPYING file in the top-level directory.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>

#include "main.h"
#include "menu.h"
#include "input.h"
#include "settings.h"
#include "config.h"
#include "args.h"
#include "dface.h"
#include "../libpicofe/plat.h"
#include "../libpicofe/input.h"
#include "../../state.h"
#include "../../general.h"
#include "../../input.h"
#include "../../palette.h"
#include "revision.h"

#define array_size(x) (sizeof(x) / sizeof(x[0]))

#define state_slot CurrentState
extern uint8 Exit;

typedef enum
{
	MA_NONE = 1,
	MA_CTRL_PLAYER1,
	MA_CTRL_PLAYER2,
	MA_CTRL_EMU,
	MA_CTRL_DEADZONE,
	MA_OPT_SAVECFG,
	MA_OPT_SAVECFG_GAME,
	MA_CTRL_DEV_FIRST,
	MA_CTRL_DEV_NEXT,
	MA_OPT_NTSC_COLOR,
	MA_OPT_NTSC_TINT,
	MA_OPT_NTSC_HUE,
	MA_OPT_SREND_N,
	MA_OPT_EREND_N,
	MA_OPT_SREND_P,
	MA_OPT_EREND_P,
	MA_OPT_CLIP,
	MA_OPT_NO8LIM,
	MA_OPT_GG,
	MA_OPT_SHOWFPS,
	MA_OPT_FSKIP,
	MA_OPT_SWFILTER,
	MA_OPT_HWFILTER,
	MA_OPT_SCALING,
	MA_OPT_RENDERER,
	MA_OPT_SOUNDON,
	MA_OPT_SOUNDRATE,
	MA_OPT_REGION,
	MA_OPT_TURBO,
	MA_OPT_SSTATE,
	MA_OPT_SSLOT,
	MA_OPT_GAMMA,
	MA_MAIN_RESUME_GAME,
	MA_MAIN_SAVE_STATE,
	MA_MAIN_LOAD_STATE,
	MA_MAIN_RESET_GAME,
	MA_MAIN_LOAD_ROM,
	MA_MAIN_CHEATS,
	MA_MAIN_CREDITS,
	MA_MAIN_EXIT,
} menu_id;

void emu_make_path(char *buff, const char *end, int size)
{
	int pos, end_len;

	end_len = strlen(end);
	pos = plat_get_root_dir(buff, size);
	strncpy(buff + pos, end, size - pos);
	buff[size - 1] = 0;
	if (pos + end_len > size - 1)
		printf("Warning: path truncated: %s\n", buff);
}

static int emu_check_save_file(int slot, int *time)
{
	struct stat status;
	char *fname;
	FILE *st;
	int retval = 0;
	int ret;
	
	fname = FCEU_MakeFName(FCEUMKF_STATE, slot, 0);
	st = fopen(fname,"rb");
	if (st == NULL)
		goto out;
	fclose(st);

	retval = 1;
	if (time == NULL)
		goto out;

	ret = stat(fname, &status);
	if (ret != 0)
		goto out;

	if (status.st_mtime < 1347000000)
		goto out; // probably bad rtc like on some Caanoos

	*time = status.st_mtime;

out:
	free(fname);
	return retval;
}

static int emu_save_load_game(int load, int unused)
{
	if (load)
		FCEUI_LoadState();
	else
		FCEUI_SaveState();

	return 0;
}

// rrrr rggg gggb bbbb
static unsigned short fname2color(const char *fname)
{
	static const char *rom_exts[]   = { ".zip", ".nes", ".fds", ".unf",
					    ".nez", ".unif" };
	static const char *other_exts[] = { ".nsf", ".ips", ".fcm" };
	const char *ext = strrchr(fname, '.');
	int i;

	if (ext == NULL)
		return 0xffff;
	for (i = 0; i < array_size(rom_exts); i++)
		if (strcasecmp(ext, rom_exts[i]) == 0)
			return 0xbdff;
	for (i = 0; i < array_size(other_exts); i++)
		if (strcasecmp(ext, other_exts[i]) == 0)
			return 0xaff5;
	return 0xffff;
}

static const char *filter_exts[] = {
	".txt", ".srm", ".pnd"
};

#define MENU_ALIGN_LEFT
#ifdef __ARM_ARCH_7A__ // assume hires device
#define MENU_X2 1
#else
#define MENU_X2 0
#endif

#include "../libpicofe/menu.c"

static void draw_savestate_bg(int slot)
{
}

static void debug_menu_loop(void)
{
}

// ------------ patch/gg menu ------------

extern void *cheats;
static int cheat_count, cheat_start, cheat_pos;

static int countcallb(char *name, uint32 a, uint8 v, int compare, int s, int type, void *data)
{
	cheat_count++;
	return 1;
}

static int clistcallb(char *name, uint32 a, uint8 v, int compare, int s, int type, void *data)
{
	int pos;

	pos = cheat_start + cheat_pos;
	cheat_pos++;
	if (pos < 0) return 1;
	if (pos >= g_menuscreen_h / me_sfont_h) return 0;

	smalltext_out16(14, pos * me_sfont_h, s ? "ON " : "OFF", 0xffff);
	smalltext_out16(14 + me_sfont_w*4, pos * me_sfont_h, type ? "S" : "R", 0xffff);
	smalltext_out16(14 + me_sfont_w*6, pos * me_sfont_h, name, 0xffff);

	return 1;
}

static void draw_patchlist(int sel)
{
	int pos, max_cnt;

	menu_draw_begin(1, 1);

	max_cnt = g_menuscreen_h / me_sfont_h;
	cheat_start = max_cnt / 2 - sel;
	cheat_pos = 0;
	FCEUI_ListCheats(clistcallb, 0);

	pos = cheat_start + cheat_pos;
	if (pos < max_cnt)
		smalltext_out16(14, pos * me_sfont_h, "done", 0xffff);

	text_out16(5, max_cnt / 2 * me_sfont_h, ">");
	menu_draw_end();
}

void patches_menu_loop(void)
{
	static int menu_sel = 0;
	int inp;

	cheat_count = 0;
	FCEUI_ListCheats(countcallb, 0);

	for (;;)
	{
		draw_patchlist(menu_sel);
		inp = in_menu_wait(PBTN_UP|PBTN_DOWN|PBTN_LEFT|PBTN_RIGHT|PBTN_L|PBTN_R
				|PBTN_MOK|PBTN_MBACK, NULL, 33);
		if (inp & PBTN_UP  ) { menu_sel--; if (menu_sel < 0) menu_sel = cheat_count; }
		if (inp & PBTN_DOWN) { menu_sel++; if (menu_sel > cheat_count) menu_sel = 0; }
		if (inp &(PBTN_LEFT|PBTN_L))  { menu_sel-=10; if (menu_sel < 0) menu_sel = 0; }
		if (inp &(PBTN_RIGHT|PBTN_R)) { menu_sel+=10; if (menu_sel > cheat_count) menu_sel = cheat_count; }
		if (inp & PBTN_MOK) { // action
			if (menu_sel < cheat_count)
				FCEUI_ToggleCheat(menu_sel);
			else 	break;
		}
		if (inp & PBTN_MBACK)
			break;
	}
}

// -------------- key config --------------

// b_turbo,a_turbo  RLDU SEBA
me_bind_action me_ctrl_actions[] =
{
	{ "UP     ", NKEY_UP      },
	{ "DOWN   ", NKEY_DOWN    },
	{ "LEFT   ", NKEY_LEFT    },
	{ "RIGHT  ", NKEY_RIGHT   },
	{ "A      ", NKEY_A       },
	{ "B      ", NKEY_B       },
	{ "A TURBO", NKEY_A_TURBO },
	{ "B TURBO", NKEY_B_TURBO },
	{ "START  ", NKEY_START   },
	{ "SELECT ", NKEY_SELECT  },
	{ NULL, 0 }
};

me_bind_action emuctrl_actions[] =
{
	{ "Save State       ", EACT_SAVE_STATE },
	{ "Load State       ", EACT_LOAD_STATE },
	{ "Next State Slot  ", EACT_NEXT_SLOT },
	{ "Prev State Slot  ", EACT_PREV_SLOT },
	{ "FDS Insert/Eject ", EACT_FDS_INSERT },
	{ "FDS Select Disk  ", EACT_FDS_SELECT },
	{ "VSUni Insert Coin", EACT_INSERT_COIN },
	{ "Enter Menu       ", EACT_ENTER_MENU },
	{ NULL,                0 }
};

static int key_config_loop_wrap(int id, int keys)
{
	switch (id) {
		case MA_CTRL_PLAYER1:
			key_config_loop(me_ctrl_actions, array_size(me_ctrl_actions) - 1, 0);
			break;
		case MA_CTRL_PLAYER2:
			key_config_loop(me_ctrl_actions, array_size(me_ctrl_actions) - 1, 1);
			break;
		case MA_CTRL_EMU:
			key_config_loop(emuctrl_actions, array_size(emuctrl_actions) - 1, -1);
			break;
		default:
			break;
	}
	return 0;
}

static const char *mgn_dev_name(int id, int *offs)
{
	const char *name = NULL;
	static int it = 0;

	if (id == MA_CTRL_DEV_FIRST)
		it = 0;

	for (; it < IN_MAX_DEVS; it++) {
		name = in_get_dev_name(it, 1, 1);
		if (name != NULL)
			break;
	}

	it++;
	return name;
}

static const char *mgn_saveloadcfg(int id, int *offs)
{
	return "";
}

static void config_commit(void);

static int mh_savecfg(int id, int keys)
{
	const char *fname = NULL;
	if (id == MA_OPT_SAVECFG_GAME)
		fname = lastLoadedGameName;

	config_commit();
	if (SaveConfig(fname) == 0)
		menu_update_msg("config saved");
	else
		menu_update_msg("failed to write config");

	return 1;
}

static int mh_input_rescan(int id, int keys)
{
	//menu_sync_config();
	in_probe();
	menu_update_msg("rescan complete.");

	return 0;
}

static menu_entry e_menu_keyconfig[] =
{
	mee_handler_id("Player 1",              MA_CTRL_PLAYER1,    key_config_loop_wrap),
	mee_handler_id("Player 2",              MA_CTRL_PLAYER2,    key_config_loop_wrap),
	mee_handler_id("Emulator controls",     MA_CTRL_EMU,        key_config_loop_wrap),
	mee_label     (""),
//	mee_range     ("Analog deadzone",   MA_CTRL_DEADZONE,   analog_deadzone, 1, 99),
	mee_cust_nosave("Save global config",       MA_OPT_SAVECFG,      mh_savecfg, mgn_saveloadcfg),
	mee_cust_nosave("Save cfg for loaded game", MA_OPT_SAVECFG_GAME, mh_savecfg, mgn_saveloadcfg),
	mee_handler   ("Rescan devices:",  mh_input_rescan),
	mee_label     (""),
	mee_label_mk  (MA_CTRL_DEV_FIRST, mgn_dev_name),
	mee_label_mk  (MA_CTRL_DEV_NEXT,  mgn_dev_name),
	mee_label_mk  (MA_CTRL_DEV_NEXT,  mgn_dev_name),
	mee_label_mk  (MA_CTRL_DEV_NEXT,  mgn_dev_name),
	mee_label_mk  (MA_CTRL_DEV_NEXT,  mgn_dev_name),
	mee_label_mk  (MA_CTRL_DEV_NEXT,  mgn_dev_name),
	mee_label_mk  (MA_CTRL_DEV_NEXT,  mgn_dev_name),
	mee_end,
};

static int menu_loop_keyconfig(int id, int keys)
{
	static int sel = 0;

	me_loop(e_menu_keyconfig, &sel);
	return 0;
}

// --------- FCEU options ----------

extern int ntsccol,ntschue,ntsctint;
extern int srendlinev[2];
extern int erendlinev[2];
extern int eoptions;
extern char *cpalette;
extern void LoadCPalette(void);

static menu_entry e_menu_fceu_options[] =
{
	//gp2x_text_out15(tl_x,  y,      "Custom palette: %s", cpal);
	mee_onoff     ("NTSC Color Emulation",      MA_OPT_NTSC_COLOR, ntsccol, 1),
	mee_range     ("  Tint (default: 56)",      MA_OPT_NTSC_TINT, ntsctint, 0, 128),
	mee_range     ("  Hue  (default: 72)",      MA_OPT_NTSC_HUE, ntschue, 0, 128),
	mee_range     ("First visible line (NTSC)", MA_OPT_SREND_N, srendlinev[0], 0, 239),
	mee_range     ("Last visible line (NTSC)",  MA_OPT_EREND_N, erendlinev[0], 0, 239),
	mee_range     ("First visible line (PAL)",  MA_OPT_SREND_P, srendlinev[1], 0, 239),
	mee_range     ("Last visible line (PAL)",   MA_OPT_EREND_P, erendlinev[1], 0, 239),
	mee_onoff     ("Clip 8 left/right columns", MA_OPT_CLIP, eoptions, EO_CLIPSIDES),
	mee_onoff     ("Disable 8 sprite limit",    MA_OPT_NO8LIM, eoptions, EO_NO8LIM),
	mee_onoff     ("Enable authentic GameGenie",MA_OPT_GG, eoptions, EO_GG),
	mee_end,
};

static int menu_loop_fceu_options(int id, int keys)
{
	static int sel = 0;
	int i;

	FCEUI_GetNTSCTH(&ntsctint, &ntschue);

	me_loop(e_menu_fceu_options, &sel);

	for(i = 0; i < 2; i++)
	{
		if (srendlinev[i] < 0 || srendlinev[i] > 239)
			srendlinev[i] = 0;
		if (erendlinev[i] < srendlinev[i] || erendlinev[i] > 239)
			erendlinev[i] = 239;
	}
	FCEUI_SetNTSCTH(ntsccol, ntsctint, ntschue);
	FCEUI_SetRenderedLines(srendlinev[0],erendlinev[0],srendlinev[1],erendlinev[1]);
	FCEUI_DisableSpriteLimitation(eoptions&EO_NO8LIM);
	FCEUI_SetGameGenie(eoptions&EO_GG);
	//if (cpalette) LoadCPalette();
	//else 
	FCEUI_SetPaletteArray(0); // set to default
	FCEU_ResetPalette();

	return 0;
}

// -------------- options --------------

static const char *men_frameskip[] = { "Auto", "0", "1", "2", "3", "4", NULL };
static const char *men_swfilter[] = { "none", "Scale2x", "Eagle2x", NULL };
static const char *men_scaling[] = { "1x", "proportional", "4:3 scaled", "fullscreen", NULL };
static const char *men_rates[]   = { "8000", "11025", "16000", "22050", "44100", NULL };
static const int   men_rates_i[] = {  8000 ,  11025 ,  16000 ,  22050 ,  44100 };
static const char *men_region[] = { "Auto", "NTSC", "PAL", NULL };
static const char *men_sstate[] = { "OFF", "writes", "loads", "both", NULL };
static const char h_renderer[] = "ROM reload required for this\n"
				 "setting to take effect";

static int sndrate_i;
static int sndon;
static int turbo_i;
static int frameskip_i;

static void config_commit(void)
{
	Settings.sound_rate = men_rates_i[sndrate_i];
	soundvol = sndon ? 50 : 0;
	Settings.turbo_rate_add = (turbo_i * 2 << 24) / 60 + 1;
	Settings.frameskip = frameskip_i - 1;

	if (Settings.region_force)
		FCEUI_SetVidSystem(Settings.region_force - 1);
}

static menu_entry e_menu_options[] =
{
//	mee_onoff      ("Show FPS",                MA_OPT_SHOWFPS, Settings.showfps, 1),
	mee_enum       ("Frameskip",               MA_OPT_FSKIP, frameskip_i, men_frameskip),
	mee_enum       ("Softwere filter",         MA_OPT_SWFILTER, Settings.sw_filter, men_swfilter),
	mee_enum       ("Hardware filter",         MA_OPT_HWFILTER, Settings.hw_filter, NULL),
	mee_enum       ("Scaling",                 MA_OPT_SCALING, Settings.scaling, men_scaling),
	mee_onoff_h    ("Accurate renderer (slow)",MA_OPT_RENDERER, Settings.accurate_mode, 1, h_renderer),
	mee_onoff      ("Enable sound",            MA_OPT_SOUNDON, sndon, 1),
	mee_enum       ("Sound Rate",              MA_OPT_SOUNDRATE, sndrate_i, men_rates),
	mee_enum       ("Region",                  MA_OPT_REGION, Settings.region_force, men_region),
	mee_range      ("Turbo rate (Hz)",         MA_OPT_TURBO, turbo_i, 1, 30),
	mee_enum       ("Confirm savestate",       MA_OPT_SSTATE, Settings.sstate_confirm, men_sstate),
	mee_range      ("Save slot",               MA_OPT_SSLOT, CurrentState, 0, 9),
//	mee_range      ("Gamma correction",        MA_OPT_GAMMA, Settings.gamma, 0, 300),
	mee_handler    ("[FCE Ultra options]",     menu_loop_fceu_options),
	mee_cust_nosave("Save global config",      MA_OPT_SAVECFG,      mh_savecfg, mgn_saveloadcfg),
	mee_cust_nosave("Save cfg for loaded game",MA_OPT_SAVECFG_GAME, mh_savecfg, mgn_saveloadcfg),
	mee_end,
};

static int menu_loop_options(int id, int keys)
{
	static int sel = 0;
	int oldrate;
	int i;

	i = me_id2offset(e_menu_options, MA_OPT_HWFILTER);
	e_menu_options[i].data = plat_target.filters;
	me_enable(e_menu_options, MA_OPT_HWFILTER, plat_target.filters != NULL);

	oldrate = Settings.sound_rate;
	for (i = 0; i < array_size(men_rates_i); i++) {
		if (Settings.sound_rate == men_rates_i[i]) {
			sndrate_i = i;
			break;
		}
	}
	sndon = soundvol != 0;
	turbo_i = (Settings.turbo_rate_add * 60 / 2) >> 24;
	frameskip_i = Settings.frameskip + 1;

	me_loop(e_menu_options, &sel);

	config_commit();
	if (oldrate != Settings.sound_rate)
		InitSound();

	return 0;
}

// -------------- root menu --------------

static const char credits_text[] = 
	"GPFCE " REV "\n"
	"(c) notaz, 2007,2012\n\n"
	"Based on FCE Ultra versions\n"
	"0.81 and 0.98.1x\n\n"
	"         - Credits -\n"
	"Bero: FCE\n"
	"Xodnizel: FCE Ultra\n"
	"FCA author: 6502 core\n"
	"M-HT: NEON scalers\n";

static void draw_frame_credits(void)
{
	smalltext_out16(4, 1, "build: " __DATE__ " " __TIME__ " " REV, 0xe7fc);
}

static int romsel_run(void)
{
	const char *fname;

	fname = menu_loop_romsel(lastLoadedGameName, sizeof(lastLoadedGameName));
	if (fname == NULL)
		return -1;

	printf("selected file: %s\n", fname);
	//keys_load_all(cfg);

	strcpy(lastLoadedGameName, rom_fname_reload);
	return 0;
}

static int menu_loop_ret;

static int main_menu_handler(int id, int keys)
{
	switch (id)
	{
	case MA_MAIN_RESUME_GAME:
		if (fceugi)
			return 1;
		break;
	case MA_MAIN_SAVE_STATE:
		if (fceugi) {
			return menu_loop_savestate(0);
		}
		break;
	case MA_MAIN_LOAD_STATE:
		if (fceugi) {
			return menu_loop_savestate(1);
		}
		break;
	case MA_MAIN_RESET_GAME:
		if (fceugi) {
			FCEU_DoSimpleCommand(FCEUNPCMD_RESET);
			return 1;
		}
		break;
	case MA_MAIN_LOAD_ROM:
		if (romsel_run() == 0) {
			menu_loop_ret = 2;
			return 1;
		}
		break;
	case MA_MAIN_CREDITS:
		draw_menu_message(credits_text, draw_frame_credits);
		in_menu_wait(PBTN_MOK|PBTN_MBACK, NULL, 70);
		break;
	case MA_MAIN_EXIT:
		menu_loop_ret = 1;
		return 1;
	default:
		lprintf("%s: something unknown selected\n", __FUNCTION__);
		break;
	}

	return 0;
}

static menu_entry e_menu_main[] =
{
	mee_handler_id("Resume game",        MA_MAIN_RESUME_GAME, main_menu_handler),
	mee_handler_id("Save State",         MA_MAIN_SAVE_STATE,  main_menu_handler),
	mee_handler_id("Load State",         MA_MAIN_LOAD_STATE,  main_menu_handler),
	mee_handler_id("Reset game",         MA_MAIN_RESET_GAME,  main_menu_handler),
	mee_handler_id("Load ROM",           MA_MAIN_LOAD_ROM,    main_menu_handler),
	mee_handler   ("Options",            menu_loop_options),
	mee_handler   ("Controls",           menu_loop_keyconfig),
	mee_handler_id("Cheats",             MA_MAIN_CHEATS,      main_menu_handler),
	mee_handler_id("Credits",            MA_MAIN_CREDITS,     main_menu_handler),
	mee_handler_id("Exit",               MA_MAIN_EXIT,        main_menu_handler),
	mee_end,
};

// ----------------------------

int menu_loop(void)
{
	static int sel = 0;

	menu_loop_ret = 0;

	me_enable(e_menu_main, MA_MAIN_RESUME_GAME, fceugi != NULL);
	me_enable(e_menu_main, MA_MAIN_SAVE_STATE,  fceugi != NULL);
	me_enable(e_menu_main, MA_MAIN_LOAD_STATE,  fceugi != NULL);
	me_enable(e_menu_main, MA_MAIN_RESET_GAME,  fceugi != NULL);
	me_enable(e_menu_main, MA_MAIN_CHEATS,      fceugi && cheats);

	plat_video_menu_enter(fceugi != NULL);
	memcpy(g_menubg_ptr, g_menubg_src_ptr, g_menuscreen_w * g_menuscreen_h * 2);
	in_set_config_int(0, IN_CFG_BLOCKING, 1);

	do {
		me_loop_d(e_menu_main, &sel, NULL, NULL);
	} while (!fceugi && menu_loop_ret == 0);

	/* wait until menu, ok, back is released */
	while (in_menu_wait_any(NULL, 50) & (PBTN_MENU|PBTN_MOK|PBTN_MBACK))
		;

	in_set_config_int(0, IN_CFG_BLOCKING, 0);
	plat_video_menu_leave();

	Exit = 0;
	return menu_loop_ret;
}

void menu_init(void)
{
	char buff[256];

	g_border_style = 1;
	menu_init_base();

	//menu_load_config(0);

	g_menubg_src_ptr = calloc(g_menuscreen_w * g_menuscreen_h * 2, 1);
	g_menubg_ptr = calloc(g_menuscreen_w * g_menuscreen_h * 2, 1);
	if (g_menubg_src_ptr == NULL || g_menubg_ptr == NULL) {
		fprintf(stderr, "OOM\n");
		exit(1);
	}

	emu_make_path(buff, "skin/background.png", sizeof(buff));
	readpng(g_menubg_src_ptr, buff, READPNG_BG, g_menuscreen_w, g_menuscreen_h);
}

void menu_update_msg(const char *msg)
{
	strncpy(menu_error_msg, msg, sizeof(menu_error_msg));
	menu_error_msg[sizeof(menu_error_msg) - 1] = 0;

	menu_error_time = plat_get_ticks_ms();
	lprintf("msg: %s\n", menu_error_msg);
}

