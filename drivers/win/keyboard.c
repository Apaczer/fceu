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

#include "common.h"
#include <dinput.h>


#include "input.h"
#include "keyboard.h"

#include "keyscan.h"


HRESULT  ddrval;

static LPDIRECTINPUTDEVICE7 lpdid=0;
static int porttemp;


int keyBMap[4][8]={
                   {SCAN_LEFTALT,SCAN_LEFTCONTROL,SCAN_TAB,SCAN_ENTER,SCAN_BL_CURSORUP,SCAN_BL_CURSORDOWN,SCAN_BL_CURSORLEFT,SCAN_BL_CURSORRIGHT},
                   {SCAN_LEFTALT,SCAN_LEFTCONTROL,SCAN_TAB,SCAN_ENTER,SCAN_BL_CURSORUP,SCAN_BL_CURSORDOWN,SCAN_BL_CURSORLEFT,SCAN_BL_CURSORRIGHT},
                   {SCAN_LEFTALT,SCAN_LEFTCONTROL,SCAN_TAB,SCAN_ENTER,SCAN_BL_CURSORUP,SCAN_BL_CURSORDOWN,SCAN_BL_CURSORLEFT,SCAN_BL_CURSORRIGHT},
                   {SCAN_LEFTALT,SCAN_LEFTCONTROL,SCAN_TAB,SCAN_ENTER,SCAN_BL_CURSORUP,SCAN_BL_CURSORDOWN,SCAN_BL_CURSORLEFT,SCAN_BL_CURSORRIGHT}
                  };
int keybEnable=1;

int powerpadside=0;
int powerpadsc[2][12]={
                              {
                               SCAN_O,SCAN_P,SCAN_BRACKET_LEFT,
                               SCAN_BRACKET_RIGHT,SCAN_K,SCAN_L,SCAN_SEMICOLON,SCAN_APOSTROPHE,
                               SCAN_M,SCAN_COMMA,SCAN_PERIOD,SCAN_SLASH
                              },
                              {
                               SCAN_O,SCAN_P,SCAN_BRACKET_LEFT,
                               SCAN_BRACKET_RIGHT,SCAN_K,SCAN_L,SCAN_SEMICOLON,SCAN_APOSTROPHE,
                               SCAN_M,SCAN_COMMA,SCAN_PERIOD,SCAN_SLASH
                              }
                             };




void KeyboardClose(void)
{
 if(lpdid) IDirectInputDevice7_Unacquire(lpdid);
 lpdid=0;
}

static char keys[256];
static void KeyboardUpdateState(void)
{
 ddrval=IDirectInputDevice7_GetDeviceState(lpdid,256,keys);
 switch(ddrval)
  {
   case DIERR_INPUTLOST:
   case DIERR_NOTACQUIRED:
                         IDirectInputDevice7_Acquire(lpdid);
                         break;
  }
}

int KeyboardInitialize(void)
{

 if(lpdid)
  return(1);

 ddrval=IDirectInput7_CreateDeviceEx(lpDI, &GUID_SysKeyboard,&IID_IDirectInputDevice7, (LPVOID *)&lpdid,0);
 if(ddrval != DI_OK)
 {
  FCEUD_PrintError("DirectInput: Error creating keyboard device.");
  return 0;
 }

 ddrval=IDirectInputDevice7_SetCooperativeLevel(lpdid, hAppWnd,DISCL_FOREGROUND|DISCL_NONEXCLUSIVE);
 if(ddrval != DI_OK)
 {
  FCEUD_PrintError("DirectInput: Error setting keyboard cooperative level.");
  return 0;
 }

 ddrval=IDirectInputDevice7_SetDataFormat(lpdid,&c_dfDIKeyboard);
 if(ddrval != DI_OK)
 {
  FCEUD_PrintError("DirectInput: Error setting keyboard data format.");
  return 0;
 }

 ddrval=IDirectInputDevice7_Acquire(lpdid);
 if(ddrval != DI_OK)
 {
  FCEUD_PrintError("DirectInput: Error acquiring keyboard.");
  return 0;
 }
 return 1;
}

static int DIPS=0;
static uint8 keyonce[256];
#define KEY(__a) keys[SCAN_##__a]
#define keyonly(__a,__z) {if(KEY(__a)){if(!keyonce[SCAN_##__a]) {keyonce[SCAN_##__a]=1;__z}} else{keyonce[SCAN_##__a]=0;}}
int cidisabled=0;

void KeyboardUpdate(void)
{ 
 KeyboardUpdateState();

 if(InputTypeFC==SIFC_FKB && cidisabled)
  return;

 NoWaiting&=~1;
 if(KEY(GRAVE))
  NoWaiting|=1;

 if(GI)
 {
  if(GI->type==GIT_FDS)
  {
   keyonly(S,DriverInterface(DES_FDSSELECT,0);)
   keyonly(I,DriverInterface(DES_FDSINSERT,0);)
   keyonly(E,DriverInterface(DES_FDSEJECT,0);)
  }

  if(GI->type!=GIT_NSF)
  {
   keyonly(F5,FCEUI_SaveState();)
   keyonly(F7,FCEUI_LoadState();)
  }
  keyonly(F9,FCEUI_SaveSnapshot();)

  if(GI->type==GIT_VSUNI)
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
 }
}

uint32 KeyboardDodo(void)
{
 uint32 JS=0;


 if(GI)
 if(GI->type!=GIT_NSF)
 {
  int x,y,u,b;

  for(u=0;u<4;u++)
  {
   if(keybEnable&(1<<u))
   {
    int *tmpo=keyBMap[u];
    x=y=0;

    for(b=3;b>=0;b--)
     if(keys[tmpo[b]]) JS|=(1<<b)<<(u<<3);

    if(keys[tmpo[4]])    y|= JOY_UP;
    if(keys[tmpo[5]])  y|= JOY_DOWN;
    if(keys[tmpo[6]])  x|= JOY_LEFT;
    if(keys[tmpo[7]]) x|= JOY_RIGHT;

    if(y!=(JOY_DOWN|JOY_UP)) JS|=y<<(u<<3);
    if(x!=(JOY_LEFT|JOY_RIGHT)) JS|=x<<(u<<3);
   }
  }
 }
 return JS;
}

uint32 UpdatePPadData(int w)
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

int fkbmap[0x48]=
{
 SCAN_F1,SCAN_F2,SCAN_F3,SCAN_F4,SCAN_F5,SCAN_F6,SCAN_F7,SCAN_F8,
 SCAN_1,SCAN_2,SCAN_3,SCAN_4,SCAN_5,SCAN_6,SCAN_7,SCAN_8,SCAN_9,SCAN_0,
        SCAN_MINUS,SCAN_EQUAL,SCAN_BACKSLASH,SCAN_BACKSPACE,
 SCAN_ESCAPE,SCAN_Q,SCAN_W,SCAN_E,SCAN_R,SCAN_T,SCAN_Y,SCAN_U,SCAN_I,SCAN_O,
        SCAN_P,SCAN_GRAVE,SCAN_BRACKET_LEFT,SCAN_ENTER,
 SCAN_LEFTCONTROL,SCAN_A,SCAN_S,SCAN_D,SCAN_F,SCAN_G,SCAN_H,SCAN_J,SCAN_K,
        SCAN_L,SCAN_SEMICOLON,SCAN_APOSTROPHE,SCAN_BRACKET_RIGHT,SCAN_BL_INSERT,
 SCAN_LEFTSHIFT,SCAN_Z,SCAN_X,SCAN_C,SCAN_V,SCAN_B,SCAN_N,SCAN_M,SCAN_COMMA,
        SCAN_PERIOD,SCAN_SLASH,SCAN_RIGHTALT,SCAN_RIGHTSHIFT,SCAN_LEFTALT,SCAN_SPACE,
 SCAN_BL_DELETE,SCAN_BL_END,SCAN_BL_PAGEDOWN,
 SCAN_BL_CURSORUP,SCAN_BL_CURSORLEFT,SCAN_BL_CURSORRIGHT,SCAN_BL_CURSORDOWN
};

uint8 fkbkeys[0x48];
void UpdateFKB(void)
{
 int x;

 for(x=0;x<0x48;x++)
 {
  fkbkeys[x]=0;
  if(keys[fkbmap[x]])
   fkbkeys[x]=1;
 }
}



static int inkeyloop=0;

static BOOL CALLBACK KeyConCallB(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  int x,y;
  char tempo[64];

  switch(uMsg) {
   case WM_USER+666:
                   if(inkeyloop)
                   {
                    SetDlgItemInt(hwndDlg,inkeyloop,lParam,0);
                    inkeyloop=0;
                   }                   
                   break;
   case WM_INITDIALOG:
                sprintf(tempo,"Virtual Gamepad %d",porttemp+1);
                SetDlgItemText(hwndDlg,302,tempo);
                sprintf(tempo,"Virtual Gamepad %d",porttemp+3);
                SetDlgItemText(hwndDlg,311,tempo);

                for(x=0;x<2;x++)
                {
                 for(y=0;y<8;y++)
                  SetDlgItemInt(hwndDlg,600+y+x*10,keyBMap[porttemp+(x<<1)][y],0);
                 if(keybEnable&(1<<((x<<1)+porttemp)))
                  CheckDlgButton(hwndDlg,320+x,BST_CHECKED);
                }
                break;
   case WM_CLOSE:
   case WM_QUIT: goto gornk;
   case WM_COMMAND:
                  if(!(wParam>>16))
                  {
                   wParam&=0xFFFF;
                   if((wParam>=600 && wParam<=607) || (wParam>=610 && wParam<=617))
                    inkeyloop=wParam;
                   else switch(wParam)
                   {                    
                    case 1:
                           gornk:
                           for(x=0;x<2;x++)
                           {
                            for(y=0;y<8;y++)
                             keyBMap[porttemp+(x<<1)][y]=GetDlgItemInt(hwndDlg,600+y+x*10,0,0);

                            if(IsDlgButtonChecked(hwndDlg,320+x)==BST_CHECKED)
                             keybEnable|=(1<<((x<<1)+porttemp));
                            else
                             keybEnable&=~(1<<((x<<1)+porttemp));
                           }

                           EndDialog(hwndDlg,0);
                           break;
                   }  
                  }
              }
  return 0;
}

static BOOL CALLBACK KeyPPConCallB(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  int x;
  char tempo[64];

  switch(uMsg) {
   case WM_USER+666:
                   if(inkeyloop)
                   {
                    SetDlgItemInt(hwndDlg,inkeyloop,lParam,0);
                    inkeyloop=0;
                   }                   
                   break;
   case WM_INITDIALOG:
                for(x=0;x<12;x++)
                 SetDlgItemInt(hwndDlg,500+x,powerpadsc[porttemp][x],0);
                CheckDlgButton(hwndDlg,300+((powerpadside>>porttemp)&1),BST_CHECKED);
                sprintf(tempo,"Virtual Power Pad %d",porttemp+1);
                SetDlgItemText(hwndDlg,302,tempo);
                break;
   case WM_CLOSE:
   case WM_QUIT: goto gornk;
   case WM_COMMAND:
                  if(!(wParam>>16))
                  {                   
                   wParam&=0xFFFF;
                   if(wParam>=500 && wParam<=511)
                    inkeyloop=wParam;
                   else switch(wParam)
                   {                    
                    case 1:
                           gornk:
                           for(x=0;x<12;x++)
                            powerpadsc[porttemp][x]=GetDlgItemInt(hwndDlg,500+x,0,0);
                           powerpadside&=~(1<<porttemp);
                           if(IsDlgButtonChecked(hwndDlg,301)==BST_CHECKED)
                            powerpadside|=1<<porttemp;
                           EndDialog(hwndDlg,0);
                           break;
                   }  
                  }
              }
  return 0;
}

static BOOL CALLBACK FKBConCallB(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  int x;

  switch(uMsg) {
   case WM_USER+666:
                   if(inkeyloop)
                   {
                    SetDlgItemInt(hwndDlg,inkeyloop,lParam,0);
                    fkbmap[inkeyloop-300]=lParam;
                    inkeyloop=0;
                   }                   
                   break;
   case WM_INITDIALOG:
                for(x=0;x<72;x++)
                 SetDlgItemInt(hwndDlg,300+x,fkbmap[x],0);
                break;
   case WM_CLOSE:
   case WM_QUIT: goto gornk;
   case WM_COMMAND:
                  if(!(wParam>>16))
                  {                   
                   wParam&=0xFFFF;
                   if(wParam>=300 && wParam<=371)
                    inkeyloop=wParam;
                   else switch(wParam)
                   {                    
                    case 1:
                           gornk:
                           EndDialog(hwndDlg,0);
                           break;
                   }  
                  }
              }
  return 0;
}

static HHOOK hHook;
static LRESULT CALLBACK FilterFunc(int nCode, WORD wParam, DWORD lParam)
{
 MSG FAR *ptrMsg;
 LPARAM tmpo;

 if(nCode>=0)
 {
  if(nCode==MSGF_DIALOGBOX)
  {
   ptrMsg=(MSG FAR *)lParam;
   if(ptrMsg->message==WM_KEYDOWN || ptrMsg->message==WM_SYSKEYDOWN)
   {
    tmpo=((ptrMsg->lParam>>16)&0x7F)|((ptrMsg->lParam>>17)&0x80);
    PostMessage(GetParent(ptrMsg->hwnd),WM_USER+666,0,tmpo);
    if(inkeyloop) return 1;
   }
  }
 }
 return CallNextHookEx(hHook,nCode,wParam,lParam);
}


void ConfigKeyboardie(HWND hParent, int port)
{
 porttemp=port;

 hHook=SetWindowsHookEx(WH_MSGFILTER,(HOOKPROC)FilterFunc,fceu_hInstance,GetCurrentThreadId());
 DialogBox(fceu_hInstance,"KEYCONFIG",hParent,KeyConCallB);
 UnhookWindowsHookEx(hHook);
}

void ConfigKeyboardiePowerpad(HWND hParent, int port)
{
 porttemp=port;

 hHook=SetWindowsHookEx(WH_MSGFILTER,(HOOKPROC)FilterFunc,fceu_hInstance,GetCurrentThreadId());
 DialogBox(fceu_hInstance,"KEYPPCONFIG",hParent,KeyPPConCallB);
 UnhookWindowsHookEx(hHook);
}

void ConfigFKB(HWND hParent)
{
 hHook=SetWindowsHookEx(WH_MSGFILTER,(HOOKPROC)FilterFunc,fceu_hInstance,GetCurrentThreadId());
 DialogBox(fceu_hInstance,"FKBCONFIG",hParent,FKBConCallB);
 UnhookWindowsHookEx(hHook);
}
