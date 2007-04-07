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
#include "joystick.h"


HRESULT  ddrval;

static GUID joyGUID[64];

static int joycounter;

static LPDIRECTINPUTDEVICE7 lpJoy[4]={0,0,0,0};

int joyOptions[4]={0,0,0,0};
int joyA[4]={1,1,1,1};
int joyB[4]={0,0,0,0};
int joySelect[4]={2,2,2,2};
int joyStart[4]={3,3,3,3};
int joyUp[4]={4,4,4,4};
int joyDown[4]={5,5,5,5};
int joyLeft[4]={6,6,6,6};
int joyRight[4]={7,7,7,7};

int joy[4]={0,0,0,0};

static int JoyXMax[4];
static int JoyXMin[4];

static int JoyYMax[4];
static int JoyYMin[4];

static DIJOYSTATE2 JoyStatus;

static void ShowDIJErr(int w, char *s)
{
 char tempo[128];
 sprintf(tempo,"DirectInput: Joystick %d: %s",w+1,s);
 FCEUD_PrintError(tempo);
}

static void JoyAutoRestore(HRESULT ddrval,LPDIRECTINPUTDEVICE7 lpJJoy)
{
   switch(ddrval)
    {
     case DIERR_INPUTLOST:
     case DIERR_NOTACQUIRED:
                           IDirectInputDevice7_Acquire(lpJJoy);
                           break;
    }
}

static int GetJoystickButton(int x)
{
 int errc=0;
 int z;

 if(lpJoy[x])
 {
   doagaino:
   if(errc>8) return(-1);

   ddrval=IDirectInputDevice7_Poll(lpJoy[x]);
   if(ddrval!=DI_OK && ddrval!=DI_NOEFFECT) {JoyAutoRestore(ddrval,lpJoy[x]);errc++;goto doagaino;}

   ddrval=IDirectInputDevice7_GetDeviceState(lpJoy[x],sizeof(JoyStatus),&JoyStatus);
   if(ddrval!=DI_OK) {JoyAutoRestore(ddrval,lpJoy[x]);errc++;goto doagaino;}

   for(z=0;z<128;z++)
    if(JoyStatus.rgbButtons[z]&0x80)
     return z;
 }
 return(-1);
}

uint32 GetJSOr(void)
{
        unsigned long ret;
        int x;
        ret=0;

        for(x=0;x<4;x++)
        {
         if(lpJoy[x])
         {

          ddrval=IDirectInputDevice7_Poll(lpJoy[x]);
          if(ddrval!=DI_OK && ddrval!=DI_NOEFFECT) JoyAutoRestore(ddrval,lpJoy[x]);

          ddrval=IDirectInputDevice7_GetDeviceState(lpJoy[x],sizeof(JoyStatus),&JoyStatus);
          if(ddrval!=DI_OK) JoyAutoRestore(ddrval,lpJoy[x]);

          if(joyOptions[x]&1)
          {
           if(JoyStatus.rgbButtons[joyUp[x]&127]&0x80) ret|=JOY_UP<<(x<<3);
           if(JoyStatus.rgbButtons[joyDown[x]&127]&0x80) ret|=JOY_DOWN<<(x<<3);
           if(JoyStatus.rgbButtons[joyLeft[x]&127]&0x80) ret|=JOY_LEFT<<(x<<3);
           if(JoyStatus.rgbButtons[joyRight[x]&127]&0x80) ret|=JOY_RIGHT<<(x<<3);
          }
          else
          {
           if(JoyStatus.lX>=JoyXMax[x])
            ret|=JOY_RIGHT<<(x<<3);
           else if(JoyStatus.lX<=JoyXMin[x])
            ret|=JOY_LEFT<<(x<<3);

           if(JoyStatus.lY>=JoyYMax[x])
            ret|=JOY_DOWN<<(x<<3);
           else if(JoyStatus.lY<=JoyYMin[x])
            ret|=JOY_UP<<(x<<3);
          }
          if(JoyStatus.rgbButtons[joyA[x]&127]&0x80) ret|=1<<(x<<3);
          if(JoyStatus.rgbButtons[joyB[x]&127]&0x80) ret|=2<<(x<<3);
          if(JoyStatus.rgbButtons[joySelect[x]&127]&0x80) ret|=4<<(x<<3);
          if(JoyStatus.rgbButtons[joyStart[x]&127]&0x80) ret|=8<<(x<<3);
         }
        }

        return ret;
}

static void KillJoystick(int w)
{
 if(lpJoy[w])
 {
  IDirectInputDevice7_Unacquire(lpJoy[w]);
  IDirectInputDevice7_Release(lpJoy[w]);
  lpJoy[w]=0;
 }
}

void KillJoysticks(void)
{
 int x;
 for(x=0;x<4;x++)
  KillJoystick(x);
}

static BOOL CALLBACK JoystickFound(LPCDIDEVICEINSTANCE lpddi, LPVOID pvRef)
{
   if(joycounter<64)
   {
    joyGUID[joycounter]=lpddi->guidInstance;
    joycounter++;
    if(pvRef)
    {
     SendDlgItemMessage(pvRef,106,CB_ADDSTRING,0,(LPARAM)(LPSTR)lpddi->tszProductName);
     SendDlgItemMessage(pvRef,112,CB_ADDSTRING,0,(LPARAM)(LPSTR)lpddi->tszProductName);
    }
    return DIENUM_CONTINUE;
   }
   else
    return 0;   
}

void InitJoystick(int w, HWND wnd)
{
  if(joy[w])
  {
   if(joy[w]>joycounter)
   {
    ShowDIJErr(w,"Not found."); 
    joy[w]=0;
    return;
   }
   ddrval=IDirectInput7_CreateDeviceEx(lpDI,&joyGUID[joy[w]-1],&IID_IDirectInputDevice7,(LPVOID *)&lpJoy[w],0);
   if(ddrval != DI_OK)
   {   
    ShowDIJErr(w,"Error creating device.");
    joy[w]=0;
    return;
   }
   ddrval=IDirectInputDevice7_SetCooperativeLevel(lpJoy[w],wnd,DISCL_FOREGROUND|DISCL_NONEXCLUSIVE);
   if (ddrval != DI_OK)
   {
    ShowDIJErr(w,"Error setting cooperative level.");
    KillJoystick(w);
    joy[w]=0;
    return;
   }
   ddrval=IDirectInputDevice7_SetDataFormat(lpJoy[w],&c_dfDIJoystick2);
   if (ddrval != DI_OK)
   {
    ShowDIJErr(w,"Error setting data format.");
    KillJoystick(w);
    joy[w]=0;
    return;
   }

   {
    DIPROPRANGE diprg;
    int r;

    memset(&diprg,0,sizeof(DIPROPRANGE));
    diprg.diph.dwSize=sizeof(DIPROPRANGE);
    diprg.diph.dwHeaderSize=sizeof(DIPROPHEADER);
    diprg.diph.dwHow=DIPH_BYOFFSET;
    diprg.diph.dwObj=DIJOFS_X;
    ddrval=IDirectInputDevice7_GetProperty(lpJoy[w],DIPROP_RANGE,&diprg.diph);
    if(ddrval!=DI_OK)
    {
     ShowDIJErr(w,"Error getting X axis range.");
     joy[w]=0;
     KillJoystick(w);
     joy[w]=0;
     return;
    }
    r=diprg.lMax-diprg.lMin;
    JoyXMax[w]=diprg.lMax-(r>>2);
    JoyXMin[w]=diprg.lMin+(r>>2);

    memset(&diprg,0,sizeof(DIPROPRANGE));
    diprg.diph.dwSize=sizeof(DIPROPRANGE);
    diprg.diph.dwHeaderSize=sizeof(DIPROPHEADER);
    diprg.diph.dwHow=DIPH_BYOFFSET;
    diprg.diph.dwObj=DIJOFS_Y;
    ddrval=IDirectInputDevice7_GetProperty(lpJoy[w],DIPROP_RANGE,&diprg.diph);
    if(ddrval!=DI_OK)
    {
     ShowDIJErr(w,"Error getting X axis range.");
     KillJoystick(w);
     joy[w]=0;
     return;
    }
    r=diprg.lMax-diprg.lMin;
    JoyYMax[w]=diprg.lMax-(r>>2);
    JoyYMin[w]=diprg.lMin+(r>>2);
   }

  }
}

void InitJoysticks(HWND wnd)
{
 int x;

 joycounter=0;
 IDirectInput7_EnumDevices(lpDI, DIDEVTYPE_JOYSTICK,JoystickFound,0,DIEDFL_ATTACHEDONLY);
 
 for(x=0;x<4;x++)
  InitJoystick(x,wnd);
}


static int joyconport;

static BOOL CALLBACK JoyConCallB(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  char tempo[64];
  int x;
  static int bid;

  switch(uMsg) {
   case WM_TIMER:
                 if(bid>=200 && bid<=215)
                 {
                  int z;

                  /* GetJoystickButton() makes sure there is a joystick,
                     so we don't need to here.
                  */                     
                  if(bid<=207)
                  {
                   if( (z=GetJoystickButton(joyconport))!=-1)
                     SetDlgItemInt(hwndDlg,bid,z,0);
                   }
                   else
                   {
                    if( (z=GetJoystickButton(2+joyconport))!=-1)
                     SetDlgItemInt(hwndDlg,bid,z,0);
                   }
                 }
                break;
   case WM_INITDIALOG:
                bid=0;
                SetTimer(hwndDlg,666,20,0);     /* Every 20ms(50x a second).*/

                InitJoysticks(hwndDlg);

                SendDlgItemMessage(hwndDlg,106,CB_ADDSTRING,0,(LPARAM)(LPSTR)"<none>");
                SendDlgItemMessage(hwndDlg,112,CB_ADDSTRING,0,(LPARAM)(LPSTR)"<none>");

                sprintf(tempo,"Virtual Gamepad %d",joyconport+1);
                SetDlgItemText(hwndDlg,102,tempo);
                sprintf(tempo,"Virtual Gamepad %d",joyconport+3);
                SetDlgItemText(hwndDlg,104,tempo);

                for(x=0;x<=2;x+=2)
                {
                 SetDlgItemInt(hwndDlg,200+(x<<2),joySelect[x+joyconport],0);
                 SetDlgItemInt(hwndDlg,201+(x<<2),joyStart[x+joyconport],0);
                 SetDlgItemInt(hwndDlg,202+(x<<2),joyB[x+joyconport],0);
                 SetDlgItemInt(hwndDlg,203+(x<<2),joyA[x+joyconport],0);

                 SetDlgItemInt(hwndDlg,204+(x<<2),joyUp[x+joyconport],0);
                 SetDlgItemInt(hwndDlg,205+(x<<2),joyDown[x+joyconport],0);
                 SetDlgItemInt(hwndDlg,206+(x<<2),joyLeft[x+joyconport],0);
                 SetDlgItemInt(hwndDlg,207+(x<<2),joyRight[x+joyconport],0);

                }
                joycounter=0;
                IDirectInput7_EnumDevices(lpDI, DIDEVTYPE_JOYSTICK,JoystickFound,hwndDlg,DIEDFL_ATTACHEDONLY);

                SendDlgItemMessage(hwndDlg,106,CB_SETCURSEL,joy[0+joyconport],(LPARAM)(LPSTR)0);
                SendDlgItemMessage(hwndDlg,112,CB_SETCURSEL,joy[2+joyconport],(LPARAM)(LPSTR)0);
                
                if(joyOptions[joyconport]&1)
                 CheckDlgButton(hwndDlg,300,BST_CHECKED);
                if(joyOptions[joyconport+2]&1)
                 CheckDlgButton(hwndDlg,301,BST_CHECKED);               
                break;
   case WM_CLOSE:
   case WM_QUIT: goto gornk;
   case WM_COMMAND:
                if(HIWORD(wParam)==EN_SETFOCUS)
                {
                 bid=LOWORD(wParam);
                }
                else if(HIWORD(wParam)==EN_KILLFOCUS)
                {
                 bid=0;
                }
                else if(HIWORD(wParam)==CBN_SELENDOK)
                {
                 switch(LOWORD(wParam))
                 {
                  case 106:
                   KillJoystick(joyconport);
                   joy[0+(joyconport)]=SendDlgItemMessage(hwndDlg,106,CB_GETCURSEL,0,(LPARAM)(LPSTR)0);
                   InitJoystick(joyconport,hwndDlg);
                   SendDlgItemMessage(hwndDlg,106,CB_SETCURSEL,joy[0+joyconport],(LPARAM)(LPSTR)0);
                   break;
                  case 112:
                   KillJoystick(2+joyconport);
                   joy[2+(joyconport)]=SendDlgItemMessage(hwndDlg,112,CB_GETCURSEL,0,(LPARAM)(LPSTR)0);
                   InitJoystick(2+joyconport,hwndDlg);                                      
                   SendDlgItemMessage(hwndDlg,112,CB_SETCURSEL,joy[2+joyconport],(LPARAM)(LPSTR)0);
                   break;
                 }
                }

                if(!(wParam>>16))
                switch(wParam&0xFFFF)
                {
                 case 1:
                        gornk:

                        KillTimer(hwndDlg,666);
                        KillJoysticks();

                        for(x=0;x<=2;x+=2)
                        {
                         joySelect[x+(joyconport)]=GetDlgItemInt(hwndDlg,200+(x<<2),0,0);
                         joyStart[x+(joyconport)]=GetDlgItemInt(hwndDlg,201+(x<<2),0,0);
                         joyB[x+(joyconport)]=GetDlgItemInt(hwndDlg,202+(x<<2),0,0);
                         joyA[x+(joyconport)]=GetDlgItemInt(hwndDlg,203+(x<<2),0,0);

                         joyUp[x+(joyconport)]=GetDlgItemInt(hwndDlg,204+(x<<2),0,0);
                         joyDown[x+(joyconport)]=GetDlgItemInt(hwndDlg,205+(x<<2),0,0);
                         joyLeft[x+(joyconport)]=GetDlgItemInt(hwndDlg,206+(x<<2),0,0);
                         joyRight[x+(joyconport)]=GetDlgItemInt(hwndDlg,207+(x<<2),0,0);
                        }
                        if(IsDlgButtonChecked(hwndDlg,300)==BST_CHECKED)
                         joyOptions[joyconport]|=1;
                        else
                         joyOptions[joyconport]&=~1;
                        if(IsDlgButtonChecked(hwndDlg,301)==BST_CHECKED)
                         joyOptions[joyconport+2]|=1;
                        else
                         joyOptions[joyconport+2]&=~1;                        
                        EndDialog(hwndDlg,0);
                        break;
               }
              }
  return 0;
}

void ConfigJoystickies(HWND hParent, int port)
{
 joyconport=port;

 KillJoysticks();
 DialogBox(fceu_hInstance,"JOYCONFIG",hParent,JoyConCallB);
 InitJoysticks(hAppWnd);
}

