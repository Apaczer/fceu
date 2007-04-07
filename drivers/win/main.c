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

#include <winsock.h>
#include <mmsystem.h>
#include <ddraw.h>
#include <dsound.h>
#include <dinput.h>
#include <dir.h>
#include <commctrl.h>
#include <shlobj.h>     // For directories configuration dialog.

#include "input.h"
#include "joystick.h"
#include "keyboard.h"
#include "cheat.h"


#define EO_BGRUN          1

#define EO_CPALETTE       4
#define EO_NOSPRLIM       8
#define EO_BSAV          16
#define EO_FSAFTERLOAD   32
#define EO_FOAFTERSTART  64
#define EO_NOTHROTTLE   128
#define EO_CLIPSIDES    256
#define EO_SNAPNAME	512

/* EO_USERFORCE is something I've been playing with.
   The code for it isn't finished.
*/
#define EO_USERFORCE   1024


#define VNSCLIP  ((eoptions&EO_CLIPSIDES)?8:0)
#define VNSWID   ((eoptions&EO_CLIPSIDES)?240:256)

static int eoptions=EO_BGRUN;

void ResetVideo(void);
void ShowCursorAbs(int w);
void HideFWindow(int h);
int SetMainWindowStuff(void);
int GetClientAbsRect(LPRECT lpRect);
void UpdateFCEUWindow(void);


HWND hAppWnd=0;
HINSTANCE fceu_hInstance;

HRESULT  ddrval;

FCEUGI *GI=0;

// cheats, misc, nonvol, states, snaps, base
static char *DOvers[6]={0,0,0,0,0,0};
static char *defaultds[5]={"cheats","gameinfo","sav","fcs","snaps"};

static char TempArray[2048];
static char BaseDirectory[2048];

void SetDirs(void)
{
 int x;
 static int jlist[5]=
  {FCEUIOD_CHEATS,FCEUIOD_MISC,FCEUIOD_NV,FCEUIOD_STATE,FCEUIOD_SNAPS};

 for(x=0;x<5;x++)
  FCEUI_SetDirOverride(jlist[x], DOvers[x]);  
 if(DOvers[5])
  FCEUI_SetBaseDirectory(DOvers[5]);
 else
  FCEUI_SetBaseDirectory(BaseDirectory);
 FCEUI_SaveExtraDataUnderBase(eoptions&EO_BSAV);
}
/* Remove empty, unused directories. */
void RemoveDirs(void)
{
 int x;

 for(x=0;x<5;x++)
  if(!DOvers[x])
  {
   sprintf(TempArray,"%s\\%s",DOvers[5]?DOvers[5]:BaseDirectory,defaultds[x]);
   RemoveDirectory(TempArray);
  }
}

void CreateDirs(void)
{
 int x;

 for(x=0;x<5;x++)
  if(!DOvers[x])
  {
   sprintf(TempArray,"%s\\%s",DOvers[5]?DOvers[5]:BaseDirectory,defaultds[x]);
   CreateDirectory(TempArray,0);
  }
}

static char *gfsdir=0;
void GetBaseDirectory(void)
{
 int x;
 BaseDirectory[0]=0;
 GetModuleFileName(0,(LPTSTR)BaseDirectory,2047);

 for(x=strlen(BaseDirectory);x>=0;x--)
 {
  if(BaseDirectory[x]=='\\' || BaseDirectory[x]=='/')
   {BaseDirectory[x]=0;break;}
 }
}

static int exiting=0;
int BlockingCheck(void)
{
  MSG msg;

  while( PeekMessage( &msg, 0, 0, 0, PM_NOREMOVE ) ) {
     if( GetMessage( &msg, 0,  0, 0)>0 )
     {
     TranslateMessage(&msg);
     DispatchMessage(&msg);
     }
   }

 if(exiting) return(0);

 return(1);
}

int NoWaiting=0;
static int fullscreen=0;
static int soundflush=0;
static int soundsleep=0;
static int genie=0;
static int palyo=0;
static int windowedfailed;
static int winsizemul=1;
static int winwidth,winheight;

static volatile int nofocus=0;
static volatile int userpause=0;

#define SO_FORCE8BIT  1
#define SO_SECONDARY  2
#define SO_GFOCUS     4
#define SO_D16VOL     8

static int soundrate=44100;
static int soundbuftime=46;
static int soundbufsize;
static int soundoptions=0;
static int soundvolume=100;

static unsigned int srendline,erendline;
static unsigned int srendlinen=8;
static unsigned int erendlinen=239;
static unsigned int srendlinep=0;
static unsigned int erendlinep=239;


static unsigned int totallines;

static void FixFL(void)
{
 FCEUI_GetCurrentVidSystem(&srendline,&erendline);
 totallines=erendline-srendline+1;
}

static void UpdateRendBounds(void)
{ 
 FCEUI_SetRenderedLines(srendlinen,erendlinen,srendlinep,erendlinep);
 FixFL(); 
}

static uint8 cpalette[192];
static int vmod=1;
static int soundo=1;
static int ntsccol=0,ntsctint,ntschue;

void FCEUD_PrintError(char *s)
{
 if(fullscreen) ShowCursorAbs(1);
 MessageBox(0,s,"FCE Ultra Error",MB_ICONERROR|MB_OK|MB_SETFOREGROUND|MB_TOPMOST);
 if(fullscreen)ShowCursorAbs(0);
}

void ShowAboutBox(void)
{
 sprintf(TempArray,"FCE Ultra "VERSION_STRING"\n\nhttp://fceultra.sourceforge.net\n\n"__TIME__"\n"__DATE__"\n""gcc "__VERSION__);
 MessageBox(hAppWnd,TempArray,"About FCE Ultra",MB_OK);
}

void DoFCEUExit(void)
{
 exiting=1;
 if(GI)
 {
  FCEUI_CloseGame();
  GI=0;
 }
}

static int changerecursive=0;

#include "throttle.c"

#include "netplay.c"
#include "sound.c"
#include "video.c"
#include "window.c"
#include "config.c"


int DriverInitialize(void)
{
  if(!InitializeDDraw())
   return(0);

  if(soundo)
   soundo=InitSound();

  SetVideoMode(fullscreen);
  InitInputStuff();             /* Initialize DInput interfaces. */
  CreateInputStuff();           /* Create and set virtual NES/FC devices. */
  return 1;
}

static void DriverKill(void)
{ 
 sprintf(TempArray,"%s/fceu.cfg",BaseDirectory);
 SaveConfig(TempArray);
 DestroyInput();
 ResetVideo();
 if(soundo) TrashSound();
 CloseWave();
 ByebyeWindow();
}


int main(int argc,char *argv[])
{
  char *t;

  if(!FCEUI_Initialize())
   goto doexito;

  fceu_hInstance=GetModuleHandle(0);

  GetBaseDirectory();

  sprintf(TempArray,"%s\\fceu.cfg",BaseDirectory);
  LoadConfig(TempArray);
  FixGIGO();      /* Since a game doesn't have to be
                     loaded before the GUI can be used, make
                     sure the temporary input type variables
                     are set.
                  */


  CreateDirs();
  SetDirs();

  DoVideoConfigFix();
  DoMiscConfigFix();

  if(eoptions&EO_CPALETTE)
   FCEUI_SetPaletteArray(cpalette);

  t=0;
  if(argc>1)
   t=argv[1];
  if(!t) fullscreen=0;

  CreateMainWindow();

  if(!InitDInput())
   goto doexito;

  if(!DriverInitialize())
   goto doexito;
 
  InitSpeedThrottle();
  UpdateMenu();

  if(t)
   ALoad(t);
  else if(eoptions&EO_FOAFTERSTART)
   LoadNewGamey(hAppWnd);

  doloopy:
  UpdateFCEUWindow();
  if(GI)
  {
   FCEUI_Emulate();
   RedrawWindow(hAppWnd,0,0,RDW_ERASE|RDW_INVALIDATE);
   StopSound();
  }
  Sleep(50);
  if(!exiting)
   goto doloopy;

  doexito:
  DriverKill();
  return(0);
}

void FCEUD_Update(uint8 *XBuf, int32 *Buffer, int Count)
{
 FCEUD_BlitScreen(XBuf);
 if(Count)
  FCEUD_WriteSoundData(Buffer,Count);
 FCEUD_UpdateInput();
}

