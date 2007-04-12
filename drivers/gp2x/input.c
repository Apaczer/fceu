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
extern int swapbuttons;
extern int scaled_display;
extern int FSkip_setting;

extern void SetVideoScaling(int pixels,int width,int height);
static INLINE long UpdateGamepadGP2X(void);



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

void FCEUD_UpdateInput(void)
{
  int t=0;
  long lastpad2=lastpad;
  long pad = UpdateGamepadGP2X();
  t=1;
  //  JSreturn=(JSreturn&0xFF000000)|(JSreturn&0xFF)|((JSreturn&0xFF0000)>>8)|((JSreturn&0xFF00)<<8);
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

}


//#ifdef GP2X

extern void ResetNES(void);
extern void CleanSurface(void);

char soundvolmeter[21];
int soundvolIndex=0;
int L_count=0;
int R_count=0;
extern int CurrentState;
int TurboFireOff=1;
int TurboFireTop=0;  // 0 is none  // 1 is  A & X turbo  // 2 is Y & B turbo
int TurboFireBottom=0;
int turbo_toggle_A=0;
int turbo_toggle_B=0;

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
/**
 * GP2x joystick reader
 *
 */
static INLINE long UpdateGamepadGP2X(void)
{
  uint32 JS=0;

  unsigned long pad=gp2x_joystick_read();
#define down(b) (pad & GP2X_##b)
#define last_down(b) (lastpad & GP2X_##b)
#define L_down (pad & GP2X_L)
#define R_down (pad & GP2X_R)
#define last_L_down (lastpad & GP2X_L)
#define last_R_down (lastpad & GP2X_R)
#define shift      ((pad     & GP2X_PUSH) || ((pad     & (GP2X_VOL_UP|GP2X_VOL_DOWN)) == (GP2X_VOL_UP|GP2X_VOL_DOWN)))
#define last_shift ((lastpad & GP2X_PUSH) || ((lastpad & (GP2X_VOL_UP|GP2X_VOL_DOWN)) == (GP2X_VOL_UP|GP2X_VOL_DOWN)))

  if (L_down && R_down && !(pad & GP2X_PUSH) && !(last_R_down && last_L_down))
  {
     ResetNES();
     puts("Reset");
     goto no_pad;
  }

  if (down(VOL_UP) && !down(VOL_DOWN))
  {
    soundvol+=1;
    if (soundvol >= 100) soundvol=100;
    //FCEUI_SetSoundVolume(soundvol);
    setsoundvol(soundvol);
  }
  else if (down(VOL_DOWN) && !down(VOL_UP))
  {
    soundvol-=1;
    if (soundvol < 0) soundvol=0;
    //FCEUI_SetSoundVolume(soundvol);
    setsoundvol(soundvol);
  }

  if (shift)
  {
    // only if it's something else then last time, and not moving around the joystick
    if (!(pad & (GP2X_UP|GP2X_DOWN|GP2X_LEFT|GP2X_RIGHT)))
    {
      if (down(SELECT))
      {
        if (last_down(SELECT) && last_shift)
	{
          // still pressed down from stretching from last one
          goto no_pad;
	}
        scaled_display = !scaled_display;

        if (scaled_display)
	{
	  SetVideoScaling(320, 256, 240);
	}
        else
	{
          SetVideoScaling(320, 320, 240);
	}

        goto no_pad;
      }
      else if (L_down && R_down)
      {
        FCEUI_CloseGame();
        puts("Quit");
        goto no_pad;
      }
      else if (R_down && !(last_R_down && last_shift))
      {
       FCEUI_LoadState();
       goto no_pad;
      }
      else if (L_down && !(last_L_down && last_shift))
      {
       FCEUI_SaveState();
       goto no_pad;
      }
      else if (down(A) && !(last_down(A) && last_shift))
      {
       FSkip_setting--;
       if (FSkip_setting < 0) {
        FSkip_setting = -1;
        FCEUI_DispMessage("Auto frameskip");
       }
       else
        FCEUI_DispMessage("Frameskip: %i", FSkip_setting);
       goto no_pad;
      }
      else if (down(Y) && !(last_down(Y) && last_shift))
      {
       FSkip_setting++;
       if (FSkip_setting > 8) FSkip_setting = 8;
       FCEUI_DispMessage("Frameskip: %i", FSkip_setting);
       goto no_pad;
      }
    }
  }

  // r is toggle savestate
  if (R_down)
  {
  	if (last_R_down)
  	{
  	  R_count++;
      if ((R_count & 31)== 31)
      {
  		CurrentState=(CurrentState+1) % 10;
  		FCEUI_DispMessage("Now Using Save State %d", CurrentState);
  	  }
  	}
  }
  else
  {
    R_count=0;
  }

  // l is toggle turbo
  if (L_down)
  {
  	if (last_L_down)
  	{
  	  L_count++;
      if ((L_count & 31)== 31)
      {
       // 0 is none  // 1 is  Y & B turbo  // 2 is X & A turbo
        if ((!TurboFireTop) && (!TurboFireBottom))
        {
        	// was off
        	TurboFireTop=1;
        	TurboFireBottom=0;
        	if (swapbuttons)
        	{
 	     	  FCEUI_DispMessage("Turbo A and Y");
        	}
        	else
        	{
 	     	  FCEUI_DispMessage("Turbo Y and B");
        	}
        }
        else if (TurboFireTop)
        {
        	TurboFireTop=0;
        	TurboFireBottom=1;
        	if (swapbuttons)
        	{
       		  FCEUI_DispMessage("Turbo X and B");
        	}
        	else
        	{
       		  FCEUI_DispMessage("Turbo A and X");
        	}
        }
        else
        {
        	TurboFireTop=0;
        	TurboFireBottom=0;
        	FCEUI_DispMessage("Turbo Off");
        }

  	  }
  	}
  }
  else
  {
    L_count=0;
  }

  //unsigned long padTmp=0;
  // shift the bits in
  // up
  //padTmp=(pad & GP2X_UP) ;  // 1 is 2^0,
  JS |= ((pad & GP2X_UP) << (4-0));  // 0x10 is 2^4

  //padTmp=(pad & GP2X_DOWN);  // 0x10 is 2^4,
  JS |= ((pad & GP2X_DOWN) << (5-4));  // 0x20 is 2^5

  //padTmp=(pad & GP2X_LEFT);  // 0x4 is 2^2,
  JS |= ((pad & GP2X_LEFT) << (6-2));  // 0x40 is 2^6

  //padTmp=(pad & GP2X_RIGHT);  // 0x40 is 2^6,
  JS |= ((pad & GP2X_RIGHT) << (7-6));  // 0x80 is 2^7


#define  A_down (pad & GP2X_A)
#define  B_down (pad & GP2X_B)
#define  X_down (pad & GP2X_X)
#define  Y_down (pad & GP2X_Y)

  // should be 2 cycles held, 1 cycle release
  turbo_toggle_A=(turbo_toggle_A+1) % 3;
  turbo_toggle_B=(turbo_toggle_B+1) % 3;

   // 0 is none  // 1 is  Y & B turbo  // 2 is X & A turbo
  // B or X are both considered A
  //padTmp=B_down >> 13;  // 2^13,

   if (!(TurboFireTop && (!turbo_toggle_A)))
   {
    JS |= ((B_down >> 13) << 0);  // 0x1 is 2^0
   }
   // A or Y are both considered B
   //padTmp=A_down >> 12;  // 2^13,
   if (!(TurboFireBottom && (!turbo_toggle_B)))
   {
    JS |= ((A_down >> 12) << 1);  // 0x2 is 2^1
   }

  if (swapbuttons)
  {
   //padTmp=X_down >> 14;  // 2^13,
   if (!(TurboFireBottom && (!turbo_toggle_A)))
   {
//    JS |= ((X_down >> 14) << 0);  // 0x1 is 2^0
    JS |= ((Y_down >> 15) << 0);  // 0x1 is 2^0
   }

   //padTmp=Y_down >> 15;  // 2^13,
   if (!(TurboFireTop && (!turbo_toggle_B)))
   {
    JS |= ((X_down >> 14) << 1);  // 0x2 is 2^1
   }
  }
  else
  {
   //padTmp=X_down >> 14;  // 2^13,
   if (!(TurboFireBottom && (!turbo_toggle_A)))
   {
    JS |= ((X_down >> 14) << 0);  // 0x1 is 2^0
   }

   //padTmp=Y_down >> 15;  // 2^13,
   if (!(TurboFireTop && (!turbo_toggle_B)))
   {
    JS |= ((Y_down >> 15) << 1);  // 0x2 is 2^1
   }
  }

  // select
  //padTmp=(pad & GP2X_SELECT) >> 9;  // 0x40 is 2^9,
  JS |= (((pad & GP2X_SELECT) >> 9) << 2);  // 0x4 is 2^2

  // start
  //padTmp=(pad & GP2X_START) >> 8;  //   2^8,
  JS |= (((pad & GP2X_START) >> 8) << 3);  // 0x8 is 2^3


  JSreturn = JS;
  lastpad=pad;

  return pad;
  //JSreturn=(JS&0xFF000000)|(JS&0xFF)|((JS&0xFF0000)>>8)|((JS&0xFF00)<<8);
no_pad:
  JSreturn=0;
  lastpad=pad;
  return 0;
}
//#endif


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
