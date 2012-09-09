/* FCE Ultra - NES/Famicom Emulator
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "../../state.h"
#include "../../general.h"
#include "../../input.h"
#include "../../svga.h"
#include "../../video.h"
#include "../libpicofe/input.h"
#include "input.h"

/* UsrInputType[] is user-specified.  InputType[] is current
       (game loading can override user settings)
*/
static int UsrInputType[2]={SI_GAMEPAD,SI_GAMEPAD};
static int UsrInputTypeFC={SI_NONE};

static int InputType[2];
static int InputTypeFC;

static uint32 JSreturn;

static int powerpadsc[2][12];
static int powerpadside=0;

static uint32 MouseData[3];
static uint8 fkbkeys[0x48];

static uint32 combo_acts = 0, combo_keys = 0;
static uint32 prev_emu_acts = 0;


static void setsoundvol(int soundvolume)
{
	int soundvolIndex;
	static char soundvolmeter[24];
	static int prev_snd_on = 0;

	if ((!!soundvolume) ^ prev_snd_on)
	{
		FCEUI_Sound(Settings.sound_rate);
		prev_snd_on = !!soundvolume;
	}

	platform_set_volume(soundvolume);

	// draw on screen :D
	int meterval=soundvolume/5;
	for (soundvolIndex = 0; soundvolIndex < 20; soundvolIndex++)
	{
		if (soundvolIndex < meterval)
		{
			soundvolmeter[soundvolIndex]='*';
		}
		else
		{
			soundvolmeter[soundvolIndex]='_';
		}
	}
	soundvolmeter[20]=0;
	FCEU_DispMessage("|%s|", soundvolmeter);
}


static void do_emu_acts(uint32 acts)
{
	uint32 actsc = acts;
	acts &= acts ^ prev_emu_acts;
	prev_emu_acts = actsc;

	if (acts & (EACT_SAVE_STATE | EACT_LOAD_STATE))
	{
		int do_it = 1;
		int need_confirm = 0;
		const char *nm;
		char tmp[64];
		int keys, len;

		if (acts & EACT_LOAD_STATE)
		{
			if (Settings.sstate_confirm & 2)
			{
				need_confirm = 1;
			}
		}
		else
		{
			if (Settings.sstate_confirm & 1)
			{
				char *fname = FCEU_MakeFName(FCEUMKF_STATE,CurrentState,0);
				FILE *st = fopen(fname, "rb");
				free(fname);
				if (st)
				{
					fclose(st);
					need_confirm = 1;
				}
			}
		}

		if (need_confirm) {
			strcpy(tmp, (acts & EACT_LOAD_STATE) ?
				"LOAD STATE?" : "OVERWRITE SAVE?");
			len = strlen(tmp);
			nm = in_get_key_name(-1, -PBTN_MA3);
			snprintf(tmp + len, sizeof(tmp) - len, "(%s=yes, ", nm);
			len = strlen(tmp);
			nm = in_get_key_name(-1, -PBTN_MBACK);
			snprintf(tmp + len, sizeof(tmp) - len, "%s=no)", nm);

			FCEU_DispMessage(tmp);
			FCEU_PutImage();
			FCEUD_Update(XBuf+8, NULL, 0);

			in_set_config_int(0, IN_CFG_BLOCKING, 1);
			while (in_menu_wait_any(NULL, 50) & (PBTN_MA3|PBTN_MBACK))
				;
			while ( !((keys = in_menu_wait_any(NULL, 50)) & (PBTN_MA3|PBTN_MBACK)) )
				;
			if (keys & PBTN_MBACK)
				do_it = 0;
			while (in_menu_wait_any(NULL, 50) & (PBTN_MA3|PBTN_MBACK))
				;
			in_set_config_int(0, IN_CFG_BLOCKING, 0);

			FCEU_CancelDispMessage();
		}
		if (do_it) {
			if (acts & EACT_LOAD_STATE)
				FCEUI_LoadState();
			else
				FCEUI_SaveState();
		}

		RefreshThrottleFPS();
	}
	else if (acts & (EACT_NEXT_SLOT|EACT_PREV_SLOT))
	{
		FILE *st;
		char *fname;

		CurrentState += (acts & EACT_NEXT_SLOT) ? 1 : -1;
		if (CurrentState > 9) CurrentState = 0;
		if (CurrentState < 0) CurrentState = 9;

		fname = FCEU_MakeFName(FCEUMKF_STATE,CurrentState,0);
		st=fopen(fname,"rb");
		free(fname);
		FCEU_DispMessage("[%s] State Slot %i", st ? "USED" : "FREE", CurrentState);
		if (st) fclose(st);
	}
	else if (acts & EACT_FDS_INSERT)
	{
        	if(FCEUGameInfo.type == GIT_FDS)
			FCEU_DoSimpleCommand(FCEUNPCMD_FDSINSERT);
	}
	else if (acts & EACT_FDS_SELECT)
	{
        	if(FCEUGameInfo.type == GIT_FDS)
			FCEU_DoSimpleCommand(FCEUNPCMD_FDSSELECT);
	}
	else if (acts & EACT_INSERT_COIN)
	{
        	if(FCEUGameInfo.type == GIT_VSUNI)
			FCEU_DoSimpleCommand(FCEUNPCMD_VSUNICOIN);
	}
}


static void do_fake_mouse(uint32 acts)
{
	static int x=256/2, y=240/2;
	int speed = 3;

	if (acts & NKEY_B_TURBO) speed = 1;
	if (acts & NKEY_A_TURBO) speed = 5;

	if (acts & NKEY_LEFT)
	{
		x -= speed;
		if (x < 0) x = 0;
	}
	else if (acts & NKEY_RIGHT)
	{
		x += speed;
		if (x > 255) x = 255;
	}

	if (acts & NKEY_UP)
	{
		y -= speed;
		if (y < 0) y = 0;
	}
	else if (acts & NKEY_DOWN)
	{
		y += speed;
		if (y > 239) y = 239;
	}

	MouseData[0] = x;
	MouseData[1] = y;
	MouseData[2] = 0;
	if (acts & NKEY_A) MouseData[2] |= 1;
	if (acts & NKEY_B) MouseData[2] |= 2;
}


static void FCEUD_UpdateInput(void)
{
	static int volpushed_frames = 0;
	static int turbo_rate_cnt_a[2] = {0,0}, turbo_rate_cnt_b[2] = {0,0};
	uint32 all_acts[2], emu_acts;
	int actions[IN_BINDTYPE_COUNT] = { 0, };

	in_update(actions);
	all_acts[0] = actions[IN_BINDTYPE_PLAYER12];
	all_acts[1] = actions[IN_BINDTYPE_PLAYER12] >> 16;
	emu_acts = actions[IN_BINDTYPE_EMU];

	if (emu_acts & EACT_ENTER_MENU)
	{
		Exit = 1;
		return;
	}
	else if (emu_acts & EACT_VOLUME_UP)
	{
		/* wait for at least 10 updates, because user may be just trying to enter menu */
		if (volpushed_frames++ > 10 && (volpushed_frames&1)) {
			soundvol++;
			if (soundvol > 100) soundvol=100;
			//FCEUI_SetSoundVolume(soundvol);
			setsoundvol(soundvol);
		}
	}
	else if (emu_acts & EACT_VOLUME_DOWN)
	{
		if (volpushed_frames++ > 10 && (volpushed_frames&1)) {
			soundvol-=1;
			if (soundvol < 0) soundvol=0;
			//FCEUI_SetSoundVolume(soundvol);
			setsoundvol(soundvol);
		}
	}
	else
	{
		volpushed_frames = 0;
	}

	JSreturn = 0; // RLDU SEBA

	if (InputType[1] != SI_GAMEPAD)
	{
		/* try to feed fake mouse there */
		do_fake_mouse(all_acts[0]);
	}

	// player 1
	JSreturn |= all_acts[0] & 0xff;
	if (all_acts[0] & NKEY_A_TURBO) {
		turbo_rate_cnt_a[0] += Settings.turbo_rate_add;
		JSreturn |= (turbo_rate_cnt_a[0] >> 24) & 1;
	}
	if (all_acts[0] & NKEY_B_TURBO) {
		turbo_rate_cnt_b[0] += Settings.turbo_rate_add;
		JSreturn |= (turbo_rate_cnt_b[0] >> 23) & 2;
	}

	// player 2
	JSreturn |= (all_acts[1] & 0xff) << 16;
	if (all_acts[1] & NKEY_A_TURBO) {
		turbo_rate_cnt_a[1] += Settings.turbo_rate_add;
		JSreturn |= (turbo_rate_cnt_a[1] >> 8) & 0x10000;
	}
	if (all_acts[1] & NKEY_B_TURBO) {
		turbo_rate_cnt_b[1] += Settings.turbo_rate_add;
		JSreturn |= (turbo_rate_cnt_b[1] >> 7) & 0x20000;
	}

	do_emu_acts(emu_acts);
}


static void InitOtherInput(void)
{

   void *InputDPtr;

   int t;
   int x;
   int attrib;

   printf("InitOtherInput: InputType[0]: %i, InputType[1]: %i, InputTypeFC: %i\n",
   	InputType[0], InputType[1], InputTypeFC);

   for(t=0,x=0;x<2;x++)
   {
    attrib=0;
    InputDPtr=0;
    switch(InputType[x])
    {
      //case SI_POWERPAD:InputDPtr=&powerpadbuf[x];break;
     case SI_GAMEPAD:InputDPtr=((uint8 *)&JSreturn)+(x<<1);break;
     case SI_ARKANOID:InputDPtr=MouseData;t|=1;break;
     case SI_ZAPPER:InputDPtr=MouseData;
                                t|=1;
                                attrib=1;
                                break;
    }
    FCEUI_SetInput(x,InputType[x],InputDPtr,attrib);
   }

   attrib=0;
   InputDPtr=0;
   switch(InputTypeFC)
   {
    case SIFC_SHADOW:InputDPtr=MouseData;t|=1;attrib=1;break;
    case SIFC_ARKANOID:InputDPtr=MouseData;t|=1;break;
    case SIFC_FKB:InputDPtr=fkbkeys;break;
   }

   FCEUI_SetInputFC(InputTypeFC,InputDPtr,attrib);
   FCEUI_DisableFourScore(eoptions&EO_NOFOURSCORE);

   inited|=16;
}


static void PrepareOtherInput(void)
{
	uint32 act;

	combo_acts = combo_keys = prev_emu_acts = 0;

	for (act = 0; act < 32; act++)
	{
		int u, keyc = 0, keyc2 = 0;
		if (act == 16 || act == 17) continue; // player2 flag
		if (act > 17)
		{
			for (u = 0; u < 32; u++)
				if (Settings.KeyBinds[u] & (1 << act)) keyc++;
		}
		else
		{
			for (u = 0; u < 32; u++)
				if ((Settings.KeyBinds[u] & 0x30000) == 0 && // pl. 1
					(Settings.KeyBinds[u] & (1 << act))) keyc++;
			for (u = 0; u < 32; u++)
				if ((Settings.KeyBinds[u] & 0x30000) == 1 && // pl. 2
					(Settings.KeyBinds[u] & (1 << act))) keyc2++;
			if (keyc2 > keyc) keyc = keyc2;
		}
		if (keyc > 1)
		{
			// loop again and mark those keys and actions as combo
			for (u = 0; u < 32; u++)
			{
				if (Settings.KeyBinds[u] & (1 << act)) {
					combo_keys |= 1 << u;
					combo_acts |= 1 << act;
				}
			}
		}
	}

	// printf("generated combo_acts: %08x, combo_keys: %08x\n", combo_acts, combo_keys);
}

