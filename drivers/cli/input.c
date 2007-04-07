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

static void UpdateFKB(void);

/* UsrInputType[] is user-specified.  InputType[] is current
	(game loading can override user settings) 
*/
static int UsrInputType[2]={SI_GAMEPAD,SI_GAMEPAD};
static int InputType[2];

static int UsrInputTypeFC={SI_NONE};
static int InputTypeFC;

static uint32 JSreturn;
int NoWaiting=0;

static void DoCheatSeq(void)
{
 #if defined(DOS) || defined(SDL)
 if(inited&1)
  SilenceSound(1);
 #endif
 KillKeyboard();
 KillVideo();

 DoConsoleCheatConfig();
 InitVideo();
 InitKeyboard();
 #if defined(DOS) || defined(SDL)
 if(inited&1)
  SilenceSound(0);
 #endif
}

#include "keyscan.h"
static char *keys;
static int DIPS=0;
static uint8 keyonce[MK_COUNT];
#define KEY(__a) keys[MK(__a)]
#define keyonly(__a,__z) {if(KEY(__a)){if(!keyonce[MK(__a)]) {keyonce[MK(__a)]=1;__z}}else{keyonce[MK(__a)]=0;}}
static int JoySwap=0;
static int cidisabled=0;
static int KeyboardUpdate(void)
{

 if(!UpdateKeyboard())
   if(keys)
    return 0;

  keys=GetKeyboard(); 

  if(InputTypeFC==SIFC_FKB)
  {
   keyonly(SCROLLLOCK,cidisabled^=1;
    FCEUI_DispMessage("Family Keyboard %sabled.",cidisabled?"en":"dis");)
   #ifdef SDL
   SDL_WM_GrabInput(cidisabled?SDL_GRAB_ON:SDL_GRAB_OFF);
   #endif
   if(cidisabled) return(1);
  }
  #ifdef SVGALIB
  keyonly(F3,LockConsole();)  
  keyonly(F4,UnlockConsole();)
  #elif SDL
  keyonly(F4,ToggleFS();)
  #endif
  NoWaiting&=~1;
  if(KEY(GRAVE))
   NoWaiting|=1;

  if(gametype==GIT_FDS)
  {
   keyonly(S,DriverInterface(DES_FDSSELECT,0);)
   keyonly(I,DriverInterface(DES_FDSINSERT,0);)
   keyonly(E,DriverInterface(DES_FDSEJECT,0);)
  }

 keyonly(F9,FCEUI_SaveSnapshot();)
 if(gametype!=GIT_NSF)
 {
  keyonly(F2,DoCheatSeq();)
  keyonly(F5,FCEUI_SaveState();)
  keyonly(F7,FCEUI_LoadState();)
 }
 else
 {
  keyonly(CURSORLEFT,DriverInterface(DES_NSFDEC,0);)
  keyonly(CURSORRIGHT,DriverInterface(DES_NSFINC,0);)
  if( KEY(ENTER)) DriverInterface(DES_NSFRES,0);
  if( KEY(CURSORUP)) DriverInterface(DES_NSFINC,0);
  if( KEY(CURSORDOWN)) DriverInterface(DES_NSFDEC,0);
 }

 keyonly(F10,DriverInterface(DES_RESET,0);)
 keyonly(F11,DriverInterface(DES_POWER,0);)
 if(KEY(F12) || KEY(ESCAPE)) FCEUI_CloseGame();

 if(gametype==GIT_VSUNI)
 {
   keyonly(C,DriverInterface(DES_VSUNICOIN,0);)
   keyonly(V,DIPS^=1;DriverInterface(DES_VSUNITOGGLEDIPVIEW,0);)
   if(!(DIPS&1)) goto DIPSless;
   keyonly(1,DriverInterface(DES_VSUNIDIPSET,(void *)1);)
   keyonly(2,DriverInterface(DES_VSUNIDIPSET,(void *)2);)
   keyonly(3,DriverInterface(DES_VSUNIDIPSET,(void *)3);)
   keyonly(4,DriverInterface(DES_VSUNIDIPSET,(void *)4);)
   keyonly(5,DriverInterface(DES_VSUNIDIPSET,(void *)5);)
   keyonly(6,DriverInterface(DES_VSUNIDIPSET,(void *)6);)
   keyonly(7,DriverInterface(DES_VSUNIDIPSET,(void *)7);)
   keyonly(8,DriverInterface(DES_VSUNIDIPSET,(void *)8);)
 }
 else
 {
  keyonly(H,DriverInterface(DES_NTSCSELHUE,0);)
  keyonly(T,DriverInterface(DES_NTSCSELTINT,0);)
  if(KEY(KP_MINUS) || KEY(MINUS)) DriverInterface(DES_NTSCDEC,0);
  if(KEY(KP_PLUS) || KEY(EQUAL)) DriverInterface(DES_NTSCINC,0);

  DIPSless:
  keyonly(0,FCEUI_SelectState(0);)
  keyonly(1,FCEUI_SelectState(1);)
  keyonly(2,FCEUI_SelectState(2);)
  keyonly(3,FCEUI_SelectState(3);)
  keyonly(4,FCEUI_SelectState(4);)
  keyonly(5,FCEUI_SelectState(5);)
  keyonly(6,FCEUI_SelectState(6);)
  keyonly(7,FCEUI_SelectState(7);)
  keyonly(8,FCEUI_SelectState(8);)
  keyonly(9,FCEUI_SelectState(9);)
 }
 return 1;
}

static uint32 KeyboardDodo(void)
{
 uint32 JS=0;

 if(gametype!=GIT_NSF)
 {
  int x,y;
  x=y=0;
  keyonly(CAPSLOCK,
                   {
                    char tmp[64];
                    JoySwap=(JoySwap+8)%32;
                    sprintf(tmp,"Joystick %d selected.",(JoySwap>>3)+1);
                    FCEUI_DispMessage(tmp);
                   })

  if(KEY(LEFTALT) || KEY(X))        JS|=JOY_A<<JoySwap;
  if(KEY(LEFTCONTROL) || KEY(SPACE) || KEY(Z) ) JS |=JOY_B<<JoySwap;
  if(KEY(ENTER))       JS |= JOY_START<<JoySwap;
  if(KEY(TAB))         JS |= JOY_SELECT<<JoySwap;
  if(KEY(CURSORDOWN))  y|= JOY_DOWN;
  if(KEY(CURSORUP))    y|= JOY_UP;
  if(KEY(CURSORLEFT))  x|= JOY_LEFT;
  if(KEY(CURSORRIGHT)) x|= JOY_RIGHT;
  if(y!=(JOY_DOWN|JOY_UP)) JS|=y<<JoySwap;
  if(x!=(JOY_LEFT|JOY_RIGHT)) JS|=x<<JoySwap;
 }
 return JS;
}

static int powerpadsc[2][12]={
                              {
                               MK(O),MK(P),MK(BRACKET_LEFT),
                               MK(BRACKET_RIGHT),MK(K),MK(L),MK(SEMICOLON),
				MK(APOSTROPHE),
                               MK(M),MK(COMMA),MK(PERIOD),MK(SLASH)
                              },
                              {
                               MK(O),MK(P),MK(BRACKET_LEFT),
                               MK(BRACKET_RIGHT),MK(K),MK(L),MK(SEMICOLON),
                                MK(APOSTROPHE),
                               MK(M),MK(COMMA),MK(PERIOD),MK(SLASH)
                              }
                             };

static uint32 powerpadbuf[2];
static int powerpadside=0;

static uint32 UpdatePPadData(int w)
{
 static const char shifttableA[12]={8,9,0,1,11,7,4,2,10,6,5,3};
 static const char shifttableB[12]={1,0,9,8,2,4,7,11,3,5,6,10};
 uint32 r=0;
 int *ppadtsc=powerpadsc[w];
 int x;

 if(powerpadside&(1<<w))
 {
  for(x=0;x<12;x++)
   if(keys[ppadtsc[x]]) r|=1<<shifttableA[x];
 }
 else
 {
  for(x=0;x<12;x++)
   if(keys[ppadtsc[x]]) r|=1<<shifttableB[x];
 }
 return r;
}

static uint32 MouseData[3];
static uint8 fkbkeys[0x48];

void FCEUD_UpdateInput(void)
{
  int x;
  int t=0;
  static uint32 KeyBJS=0;
  uint32 JS;
  int b;

  b=KeyboardUpdate();

  for(x=0;x<2;x++)
   switch(InputType[x])
   {
    case SI_GAMEPAD:t|=1;break;
    case SI_ARKANOID:t|=2;break;
    case SI_ZAPPER:t|=2;break;
    case SI_POWERPAD:powerpadbuf[x]=UpdatePPadData(x);break;
   }

  switch(InputTypeFC)
  {
   case SIFC_ARKANOID:t|=2;break;
   case SIFC_SHADOW:t|=2;break;
   case SIFC_FKB:if(cidisabled) UpdateFKB();break;
  }

  if(t&1)
  {
   if(b)
    KeyBJS=KeyboardDodo();
   JS=KeyBJS;
   JS|=(uint32)GetJSOr();
   JSreturn=(JS&0xFF000000)|(JS&0xFF)|((JS&0xFF0000)>>8)|((JS&0xFF00)<<8);
  }
  if(t&2)
   GetMouseData(MouseData);
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
     case SI_POWERPAD:InputDPtr=&powerpadbuf[x];break;
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

int fkbmap[0x48]=
{
 MK(F1),MK(F2),MK(F3),MK(F4),MK(F5),MK(F6),MK(F7),MK(F8),
 MK(1),MK(2),MK(3),MK(4),MK(5),MK(6),MK(7),MK(8),MK(9),MK(0),
        MK(MINUS),MK(EQUAL),MK(BACKSLASH),MK(BACKSPACE),
 MK(ESCAPE),MK(Q),MK(W),MK(E),MK(R),MK(T),MK(Y),MK(U),MK(I),MK(O),
        MK(P),MK(GRAVE),MK(BRACKET_LEFT),MK(ENTER),
 MK(LEFTCONTROL),MK(A),MK(S),MK(D),MK(F),MK(G),MK(H),MK(J),MK(K),
        MK(L),MK(SEMICOLON),MK(APOSTROPHE),MK(BRACKET_RIGHT),MK(INSERT),
 MK(LEFTSHIFT),MK(Z),MK(X),MK(C),MK(V),MK(B),MK(N),MK(M),MK(COMMA),
        MK(PERIOD),MK(SLASH),MK(RIGHTALT),MK(RIGHTSHIFT),MK(LEFTALT),MK(SPACE),
 MK(DELETE),MK(END),MK(PAGEDOWN),
 MK(CURSORUP),MK(CURSORLEFT),MK(CURSORRIGHT),MK(CURSORDOWN)
};

static void UpdateFKB(void)
{
 int x;

 for(x=0;x<0x48;x++)
 {
  fkbkeys[x]=0;
  if(keys[fkbmap[x]])
   fkbkeys[x]=1;
 }
}
