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
#include "joystick.h"


int UsrInputType[2]={SI_GAMEPAD,SI_GAMEPAD};
int UsrInputTypeFC=SIFC_NONE;
int InputType[2];
int InputTypeFC;

int NoFourScore=0;

static uint32 powerpadbuf[2];

LPDIRECTINPUT7 lpDI;

void FixGIGO(void)
{
 InputType[0]=UsrInputType[0];
 InputType[1]=UsrInputType[1];
 InputTypeFC=UsrInputTypeFC;

 if(GI)
 {
  if(GI->input[0]>=0)
   InputType[0]=GI->input[0];
  if(GI->input[1]>=0)
   InputType[1]=GI->input[1];
  if(GI->inputfc>=0)
   InputTypeFC=GI->inputfc;
  CreateInputStuff();
 }
}

static uint32 JSreturn;
static uint32 mousedata[3];


static void ConfigGamePad(HWND hParent, int port);

int InitDInput(void)
{
 HRESULT ddrval;

 ddrval=DirectInputCreateEx(fceu_hInstance,DIRECTINPUT_VERSION,&IID_IDirectInput7,(LPVOID *)&lpDI,0);
 if(ddrval!=DI_OK)
 {
  FCEUD_PrintError("DirectInput: Error creating DirectInput object.");
  return 0;
 }
 return 1;
}

static int screenmode=0;
void InputScreenChanged(int fs)
{
 int x;
 for(x=0;x<2;x++)
  if(InputType[x]==SI_ZAPPER)
   FCEUI_SetInput(x,SI_ZAPPER,mousedata,fs);
 if(InputTypeFC==SIFC_SHADOW)
  FCEUI_SetInputFC(SIFC_SHADOW,mousedata,fs);
 screenmode=fs;
}

void InitInputStuff(void)
{
   KeyboardInitialize();
   InitJoysticks(hAppWnd);
}

void CreateInputStuff(void)
{ 
   void *InputDPtr=0;
   int x;
   int TAttrib;

   for(x=0;x<2;x++)
   {
    TAttrib=0;
    switch(InputType[x])
    {
     case SI_GAMEPAD:InputDPtr=((uint8 *)&JSreturn)+(x<<1);
				 break;
     case SI_POWERPAD:InputDPtr=&powerpadbuf[x];break;
     case SI_ARKANOID:InputDPtr=mousedata;break;
     case SI_ZAPPER:InputDPtr=mousedata;
                                TAttrib=screenmode;
                                break;
    }   
    FCEUI_SetInput(x,InputType[x],InputDPtr,TAttrib);
   }

   TAttrib=0;
   switch(InputTypeFC)
   {
    case SIFC_SHADOW:InputDPtr=mousedata;TAttrib=screenmode;break;
    case SIFC_ARKANOID:InputDPtr=mousedata;break;    
    case SIFC_FKB:InputDPtr=fkbkeys;memset(fkbkeys,0,sizeof(fkbkeys));break;
   }
   FCEUI_SetInputFC(InputTypeFC,InputDPtr,TAttrib);
   FCEUI_DisableFourScore(NoFourScore);
}

void DestroyInput(void)
{
   KillJoysticks();
   KeyboardClose();
}

void FCEUD_UpdateInput(void)
{
  int x;
  uint32 JS;
  int t=0;

  KeyboardUpdate();

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
   case SIFC_FKB: if(cidisabled) UpdateFKB();break;
  }
  if(t&1)
  {
   JS=KeyboardDodo();
   if(joy[0]|joy[1]|joy[2]|joy[3])
    JS|=(uint32)GetJSOr();
   JSreturn=(JS&0xFF000000)|(JS&0xFF)|((JS&0xFF0000)>>8)|((JS&0xFF00)<<8);   
  }
  if(t&2)  
   GetMouseData(&mousedata[0], &mousedata[1], &mousedata[2]);
}


BOOL CALLBACK InputConCallB(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  static void (*boopar[5])(HWND hParent, int port)={0,ConfigGamePad,0,ConfigKeyboardiePowerpad,0};
  static void (*boopar2[5])(HWND hParent)={0,0,0,0,ConfigFKB};
  static char *strn[5]={"<none>","Gamepad","Zapper","Power Pad","Arkanoid Paddle"};
  static char *strf[5]={"<none>","Arkanoid Paddle","Hyper Shot gun","4-Player Adapter","Family Keyboard"};
  int x;
  
  switch(uMsg) {
   case WM_INITDIALOG:                
                for(x=0;x<2;x++)        
                {
                 int y;

                 for(y=0;y<5;y++)
                  SendDlgItemMessage(hwndDlg,104+x,CB_ADDSTRING,0,(LPARAM)(LPSTR)strn[y]);

                 SendDlgItemMessage(hwndDlg,104+x,CB_SETCURSEL,UsrInputType[x],(LPARAM)(LPSTR)0);
                 EnableWindow(GetDlgItem(hwndDlg,106+x),boopar[InputType[x]]?1:0);
                 SetDlgItemText(hwndDlg,200+x,(LPTSTR)strn[InputType[x]]);
                }


                {
                 int y;
                 for(y=0;y<5;y++)
                  SendDlgItemMessage(hwndDlg,110,CB_ADDSTRING,0,(LPARAM)(LPSTR)strf[y]);
                 SendDlgItemMessage(hwndDlg,110,CB_SETCURSEL,UsrInputTypeFC,(LPARAM)(LPSTR)0);                
                 EnableWindow(GetDlgItem(hwndDlg,111),boopar2[InputTypeFC]?1:0);
                 SetDlgItemText(hwndDlg,202,(LPTSTR)strf[InputTypeFC]);
                }
                
                break;
   case WM_CLOSE:
   case WM_QUIT: goto gornk;
   case WM_COMMAND:
                if(HIWORD(wParam)==CBN_SELENDOK)
                {
                 switch(LOWORD(wParam))
                 {
                  case 104:
                  case 105:UsrInputType[LOWORD(wParam)-104]=InputType[LOWORD(wParam)-104]=SendDlgItemMessage(hwndDlg,LOWORD(wParam),CB_GETCURSEL,0,(LPARAM)(LPSTR)0);
                           EnableWindow( GetDlgItem(hwndDlg,LOWORD(wParam)+2),boopar[InputType[LOWORD(wParam)-104]]?1:0);
                           SetDlgItemText(hwndDlg,200+LOWORD(wParam)-104,(LPTSTR)strn[InputType[LOWORD(wParam)-104]]);
                           break;
                  case 110:UsrInputTypeFC=InputTypeFC=SendDlgItemMessage(hwndDlg,110,CB_GETCURSEL,0,(LPARAM)(LPSTR)0);
                           EnableWindow(GetDlgItem(hwndDlg,111),boopar2[InputTypeFC]?1:0);
                           SetDlgItemText(hwndDlg,202,(LPTSTR)strf[InputTypeFC]);
                           break;
                           
                 }

                }
                if(!(wParam>>16))
                switch(wParam&0xFFFF)
                {
                 case 111:
                  if(boopar2[InputTypeFC])
                   boopar2[InputTypeFC](hwndDlg);
                  break;

                 case 107:
                 case 106:
                  {
                   int t=(wParam&0xFFFF)-106;
                   if(boopar[InputType[t]])
                    boopar[InputType[t]](hwndDlg,t);
                  }
                  break;
                 case 1:
                        gornk:
                        EndDialog(hwndDlg,0);
                        break;
               }
              }
  return 0;
}

void ConfigInput(HWND hParent)
{
  DialogBox(fceu_hInstance,"INPUTCONFIG",hParent,InputConCallB);
  CreateInputStuff();
}


static int porttemp;

BOOL CALLBACK GPConCallB(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch(uMsg) {
   case WM_INITDIALOG:                
                if(NoFourScore)
                 CheckDlgButton(hwndDlg,200,BST_CHECKED);
                break;
   case WM_CLOSE:
   case WM_QUIT: goto gornk;
   case WM_COMMAND:
                if(!(wParam>>16))
                switch(wParam&0xFFFF)
                {
                 case 107:ConfigJoystickies(hwndDlg, porttemp);break;
                 case 106:ConfigKeyboardie(hwndDlg, porttemp);break;
                 case 1:
                        gornk:
                        NoFourScore=0;
                        if(IsDlgButtonChecked(hwndDlg,200)==BST_CHECKED)
                         NoFourScore=1;
                        EndDialog(hwndDlg,0);
                        break;
               }
              }
  return 0;
}

static void ConfigGamePad(HWND hParent, int port)
{
  porttemp=port;
  DialogBox(fceu_hInstance,"GAMEPADCONFIG",hParent,GPConCallB);
}


