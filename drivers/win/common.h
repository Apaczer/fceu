#include <stdio.h>
#include <windows.h>
#include <windowsx.h>

#ifndef WIN32
#define WIN32
#endif
#undef  WINNT
#define NONAMELESSUNION

#define DIRECTSOUND_VERSION  0x0700
#define DIRECTDRAW_VERSION 0x0700
#define DIRECTINPUT_VERSION     0x700
#include "../../driver.h"

extern HWND hAppWnd;
extern HINSTANCE fceu_hInstance;

extern int NoWaiting;
extern FCEUGI *GI;
