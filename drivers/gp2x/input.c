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

	// draw on screen :D
	gp2x_sound_volume(soundvolume, soundvolume);
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

	if (acts & (3 << 30))
	{
		if (acts & (1 << 30))
		{
			FCEUI_LoadState();
		}
		else
		{
			FCEUI_SaveState();
		}
	}
	else if (acts & (3 << 28)) // state slot next/prev
	{
		FILE *st;
		char *fname;

		CurrentState += (acts & (1 << 29)) ? 1 :  -1;
		if (CurrentState > 9) CurrentState = 0;
		if (CurrentState < 0) CurrentState = 9;

		fname = FCEU_MakeFName(FCEUMKF_STATE,CurrentState,0);
		st=fopen(fname,"rb");
		free(fname);
		FCEU_DispMessage("[%s] State Slot %i", st ? "USED" : "FREE", CurrentState);
		if (st) fclose(st);
	}
	else if (acts & (1 << 27)) // FDS insert/eject
	{
        	if(FCEUGameInfo.type == GIT_FDS)
			FCEU_DoSimpleCommand(FCEUNPCMD_FDSINSERT);
	}
	else if (acts & (1 << 26)) // FDS select
	{
        	if(FCEUGameInfo.type == GIT_FDS)
			FCEU_DoSimpleCommand(FCEUNPCMD_FDSSELECT);
	}
	else if (acts & (1 << 25)) // VS Unisystem insert coin
	{
        	if(FCEUGameInfo.type == GIT_VSUNI)
			FCEU_DoSimpleCommand(FCEUNPCMD_VSUNICOIN);
	}
}


#define down(b) (keys & GP2X_##b)
static void do_fake_mouse(unsigned long keys)
{
	static int x=256/2, y=240/2;
	int speed = 3;

	if (down(A)) speed = 1;
	if (down(Y)) speed = 5;

	if (down(LEFT))
	{
		x -= speed;
		if (x < 0) x = 0;
	}
	else if (down(RIGHT))
	{
		x += speed;
		if (x > 255) x = 255;
	}

	if (down(UP))
	{
		y -= speed;
		if (y < 0) y = 0;
	}
	else if (down(DOWN))
	{
		y += speed;
		if (y > 239) y = 239;
	}

	MouseData[0] = x;
	MouseData[1] = y;
	MouseData[2] = 0;
	if (down(B)) MouseData[2] |= 1;
	if (down(X)) MouseData[2] |= 2;
}


void FCEUD_UpdateInput(void)
{
	static int volpushed_frames = 0;
	static int turbo_rate_cnt_a = 0, turbo_rate_cnt_b = 0;
	unsigned long keys = gp2x_joystick_read(0);
	uint32 all_acts = 0;
	int i;

	if ((down(VOL_DOWN) && down(VOL_UP)) || (keys & (GP2X_L|GP2X_L|GP2X_START)) == (GP2X_L|GP2X_L|GP2X_START))
	{
		Exit = 1;
		return;
	}
	else if (down(VOL_UP))
	{
		/* wait for at least 10 updates, because user may be just trying to enter menu */
		if (volpushed_frames++ > 10) {
			soundvol++;
			if (soundvol > 100) soundvol=100;
			//FCEUI_SetSoundVolume(soundvol);
			setsoundvol(soundvol);
		}
	}
	else if (down(VOL_DOWN))
	{
		if (volpushed_frames++ > 10) {
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
		do_fake_mouse(keys);
	}

	for (i = 0; i < 32; i++)
	{
		if (keys & (1 << i))
		{
			uint32 acts, u = 32;
			acts = Settings.KeyBinds[i];
			if (!acts) continue;
			if ((1 << i) & combo_keys)
			{
				// combo key detected, try to find if other is pressed
				for (u = i+1; u < 32; u++)
				{
					if ((keys & (1 << u)) && (Settings.KeyBinds[u] & acts))
					{
						keys &= ~(1 << u);
						break;
					}
				}
			}
			if (u != 32) acts &=  combo_acts; // other combo key pressed
			else         acts &= ~combo_acts;
			all_acts |= acts;
		}
	}

	JSreturn |= all_acts & 0xff;
	if (all_acts & 0x100) {		// A turbo
		turbo_rate_cnt_a += Settings.turbo_rate_add;
		JSreturn |= (turbo_rate_cnt_a >> 24) & 1;
	}
	if (all_acts & 0x200) {		// B turbo
		turbo_rate_cnt_b += Settings.turbo_rate_add;
		JSreturn |= (turbo_rate_cnt_b >> 23) & 2;
	}

	do_emu_acts(all_acts);
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
	uint32 act, key, seen_acts;

	combo_acts = combo_keys = prev_emu_acts = seen_acts = 0;

	// find combo_acts
	for (act = 1; act; act <<= 1)
	{
		for (key = 1; key < 32; key++)
		{
			if (Settings.KeyBinds[key] & act)
			{
				if (seen_acts & act) combo_acts |= act;
				else seen_acts |= act;
			}
		}
	}

	// find combo_keys
	for (act = 1; act; act <<= 1)
	{
		for (key = 0; key < 32; key++)
		{
			if (Settings.KeyBinds[key] & combo_acts)
			{
				combo_keys |= 1 << key;
			}
		}
	}

	printf("generated combo_acts: %08x, combo_keys: %08x\n", combo_acts, combo_keys);
}

