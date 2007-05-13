/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2002 Ben Parnell
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

#define JOY_A   1
#define JOY_B   2
#define JOY_SELECT      4
#define JOY_START       8
#define JOY_UP  0x10
#define JOY_DOWN        0x20
#define JOY_LEFT        0x40
#define JOY_RIGHT       0x80

#include "minimal.h"

extern uint8 Exit; // exit emu loop


/* UsrInputType[] is user-specified.  InputType[] is current
	(game loading can override user settings)
*/
static int UsrInputType[2]={SI_GAMEPAD,SI_GAMEPAD};
static int InputType[2];

static int UsrInputTypeFC={SI_NONE};
static int InputTypeFC;

static uint32 JSreturn;
int NoWaiting=0;

static int powerpadsc[2][12];
static int powerpadside=0;


static uint32 MouseData[3];
static uint8 fkbkeys[0x48];
unsigned long lastpad=0;

char soundvolmeter[21];
int soundvolIndex=0;


static void setsoundvol(int soundvolume)
{
    //FCEUI_SetSoundVolume(soundvol);
    // draw on screen :D
    gp2x_sound_volume(soundvolume, soundvolume);
    int meterval=soundvolume/5;
    for (soundvolIndex=0; soundvolIndex < 20; soundvolIndex++)
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



void FCEUD_UpdateInput(void)
{
	static int volpushed_frames = 0;
	static int turbo_rate_cnt_a = 0, turbo_rate_cnt_b = 0;
	long lastpad2 = lastpad;
	unsigned long keys = gp2x_joystick_read(0);
	uint32 JS = 0; // RLDU SEBA
	int i;

	#define down(b) (keys & GP2X_##b)
	if ((down(VOL_DOWN) && down(VOL_UP)) || (keys & (GP2X_L|GP2X_L|GP2X_START)) == (GP2X_L|GP2X_L|GP2X_START))
	{
		Exit = 1;
		JSreturn = 0;
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


	for (i = 0; i < 32; i++)
	{
		if (keys & (1 << i)) {
			int acts = Settings.KeyBinds[i];
			if (!acts) continue;
			JS |= acts & 0xff;
			if (acts & 0x100) {		// A turbo
				turbo_rate_cnt_a += Settings.turbo_rate_add;
				JS |= (turbo_rate_cnt_a >> 24) & 1;
			}
			if (acts & 0x200) {		// B turbo
				turbo_rate_cnt_b += Settings.turbo_rate_add;
				JS |= (turbo_rate_cnt_b >> 23) & 2;
			}
		}
	}


	JSreturn = JS;
	lastpad=keys;

	//JSreturn=(JS&0xFF000000)|(JS&0xFF)|((JS&0xFF0000)>>8)|((JS&0xFF00)<<8);

#define pad keys

  //  JSreturn=(JSreturn&0xFF000000)|(JSreturn&0xFF)|((JSreturn&0xFF0000)>>8)|((JSreturn&0xFF00)<<8);
  // TODO: make these bindable, use new interface
  if(gametype==GIT_FDS)
  {
    NoWaiting&=~1;
  	if ((pad & GP2X_PUSH) && (!(pad & GP2X_SELECT)) && (!(pad & GP2X_L)) && (!(pad & GP2X_R)) && (!(lastpad2 & GP2X_PUSH)))
  	{
      DriverInterface(DES_FDSSELECT,0);
  	}
  	else if ((pad & GP2X_L) && (!(pad & GP2X_SELECT)) && (!(pad & GP2X_PUSH)) && (!(pad & GP2X_R))&& (!(lastpad2 & GP2X_L)))
  	{
      DriverInterface(DES_FDSINSERT,0);
  	}
  	else if ((pad & GP2X_R) && (!(pad & GP2X_SELECT)) && (!(pad & GP2X_L)) && (!(pad & GP2X_PUSH)) && (!(lastpad2 & GP2X_R)))
  	{
      DriverInterface(DES_FDSEJECT,0);
  	}
  }
  return;
}


static void InitOtherInput(void)
{

   void *InputDPtr;

   int t;
   int x;
   int attrib;

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

   if(t && !(inited&16))
   {
    InitMouse();
    inited|=16;
   }
}
