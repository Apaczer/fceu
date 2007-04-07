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

#include "cheat.h"

static int selcheat;
static int scheatmethod=0;
static uint8 cheatval1=0;
static uint8 cheatval2=0;

static void ConfigAddCheat(HWND wnd);


static uint16 StrToU16(char *s)
{
 unsigned int ret=0;
 sscanf(s,"%4x",&ret);
 return ret;
}

static uint8 StrToU8(char *s)
{
 unsigned int ret=0;
 sscanf(s,"%d",&ret);
 return ret;
}


/* Need to be careful where these functions are used. */
static char *U16ToStr(uint16 a)
{
 static char TempArray[16];
 sprintf(TempArray,"%04X",a);
 return TempArray;
}

static char *U8ToStr(uint8 a)
{
 static char TempArray[16];
 sprintf(TempArray,"%03d",a);
 return TempArray;
}


static HWND RedoCheatsWND;
static int RedoCheatsCallB(char *name, uint32 a, uint8 v, int s)
{
 SendDlgItemMessage(RedoCheatsWND,101,LB_ADDSTRING,0,(LPARAM)(LPSTR)name);
 return(1);
}

static void RedoCheatsLB(HWND hwndDlg)
{
 SendDlgItemMessage(hwndDlg,101,LB_RESETCONTENT,0,0);
 RedoCheatsWND=hwndDlg;
 FCEUI_ListCheats(RedoCheatsCallB);
}


BOOL CALLBACK CheatsConCallB(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch(uMsg)
  {
   case WM_INITDIALOG:                  
                RedoCheatsLB(hwndDlg);
                break;
   case WM_CLOSE:
   case WM_QUIT: goto gornk;
   case WM_COMMAND:
                switch(HIWORD(wParam))
                {
                 case BN_CLICKED:
                        if(selcheat>=0)
                        {
                         if(LOWORD(wParam)==107)
                          FCEUI_SetCheat(selcheat,0,-1,-1,1);
                         else if(LOWORD(wParam)==108)
                          FCEUI_SetCheat(selcheat,0,-1,-1,0);
                        }
                        break;
                 case EN_KILLFOCUS:
                        if(selcheat>=0)
                        {
                         char TempArray[256];
                         int32 t;

                         GetDlgItemText(hwndDlg,LOWORD(wParam),TempArray,256);
                         switch(LOWORD(wParam))
                         {
                          case 102:FCEUI_SetCheat(selcheat,TempArray,-1,-1,-1);
                                   SendDlgItemMessage(hwndDlg,101,LB_INSERTSTRING,selcheat,(LPARAM)(LPCTSTR)TempArray);
                                   SendDlgItemMessage(hwndDlg,101,LB_DELETESTRING,selcheat+1,0);
                                   SendDlgItemMessage(hwndDlg,101,LB_SETCURSEL,selcheat,0);
                                   break;
                          case 103:t=StrToU16(TempArray);
                                   FCEUI_SetCheat(selcheat,0,t,-1,-1);
                                   break;
                          case 104:t=StrToU8(TempArray);
                                   FCEUI_SetCheat(selcheat,0,-1,t,-1);
                                   break;
                         }
                        }
                        break;
                }

                switch(LOWORD(wParam))
                {
                 case 101:
                        if(HIWORD(wParam)==LBN_SELCHANGE)
                        {
                         char *s;
                         uint32 a;
                         uint8 b;
			 int status;

                         selcheat=SendDlgItemMessage(hwndDlg,101,LB_GETCURSEL,0,(LPARAM)(LPSTR)0);
                         if(selcheat<0) break;

                         FCEUI_GetCheat(selcheat,&s,&a,&b,&status);
                         SetDlgItemText(hwndDlg,102,(LPTSTR)s);
                         SetDlgItemText(hwndDlg,103,(LPTSTR)U16ToStr(a));
                         SetDlgItemText(hwndDlg,104,(LPTSTR)U8ToStr(b));

                         CheckRadioButton(hwndDlg,107,108,status?107:108);
                        }
                        break;
                }

                if(!(wParam>>16))
                switch(wParam&0xFFFF)
                {
                 case 106:
                          if(selcheat>=0)
                          {
                           FCEUI_DelCheat(selcheat);
                           SendDlgItemMessage(hwndDlg,101,LB_DELETESTRING,selcheat,0);
                           selcheat=-1;
                           SetDlgItemText(hwndDlg,102,(LPTSTR)"");
                           SetDlgItemText(hwndDlg,103,(LPTSTR)"");
                           SetDlgItemText(hwndDlg,104,(LPTSTR)"");
                           CheckRadioButton(hwndDlg,107,108,0); // Is this correct?
                          }
                          break;
                 case 105:
                          ConfigAddCheat(hwndDlg);
                          RedoCheatsLB(hwndDlg);
                          break;
                 case 1:
                        gornk:
                        EndDialog(hwndDlg,0);
                        break;
                }
              }
  return 0;
}


void ConfigCheats(HWND hParent)
{
 if(!GI)
 {
  FCEUD_PrintError("You must have a game loaded before you can manipulate cheats.");
  return;
 }

 if(GI->type==GIT_NSF)
 {
  FCEUD_PrintError("Sorry, you can't cheat with NSFs.");
  return;
 }

 selcheat=-1;
 DialogBox(fceu_hInstance,"CHEATS",hParent,CheatsConCallB);
}






HWND cfcallbw;

int cfcallb(uint32 a, uint8 last, uint8 current)
{
 char temp[16];

 sprintf(temp,"%04X:%03d:%03d",(unsigned int)a,last,current);
 SendDlgItemMessage(cfcallbw,108,LB_ADDSTRING,0,(LPARAM)(LPSTR)temp);
 return(1);
}

static int scrollindex;
static int scrollnum;
static int scrollmax;

int cfcallbinsert(uint32 a, uint8 last, uint8 current)
{
 char temp[16];

 sprintf(temp,"%04X:%03d:%03d",(unsigned int)a,last,current);
 SendDlgItemMessage(cfcallbw,108,LB_INSERTSTRING,13,(LPARAM)(LPSTR)temp);
 return(1);
}

int cfcallbinsertt(uint32 a, uint8 last, uint8 current)
{
 char temp[16];

 sprintf(temp,"%04X:%03d:%03d",(unsigned int)a,last,current);
 SendDlgItemMessage(cfcallbw,108,LB_INSERTSTRING,0,(LPARAM)(LPSTR)temp);
 return(1);
}


void AddTheThing(HWND hwndDlg, char *s, int a, int v)
{
 if(FCEUI_AddCheat(s,a,v))
  MessageBox(hwndDlg,"Cheat Added","Cheat Added",MB_OK);
}


static void DoGet(void)
{
 int n=FCEUI_CheatSearchGetCount();
 int t;
 scrollnum=n;
 scrollindex=-32768;

 SendDlgItemMessage(cfcallbw,108,LB_RESETCONTENT,0,0);
 FCEUI_CheatSearchGetRange(0,13,cfcallb);

 t=-32768+n-1-13;
 if(t<-32768)
  t=-32768;
 scrollmax=t;
 SendDlgItemMessage(cfcallbw,120,SBM_SETRANGE,-32768,t);
 SendDlgItemMessage(cfcallbw,120,SBM_SETPOS,-32768,1);
}


BOOL CALLBACK AddCheatCallB(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  static int lbfocus;
  static HWND hwndLB;
  cfcallbw=hwndDlg;


  switch(uMsg)
  {                                                                               
   case WM_VSCROLL:
                if(scrollnum>13)
                {
                 switch((int)LOWORD(wParam))
                 {
                  case SB_TOP:
                        scrollindex=-32768;
                        SendDlgItemMessage(hwndDlg,120,SBM_SETPOS,scrollindex,1);
                        SendDlgItemMessage(hwndDlg,108,LB_RESETCONTENT,13,0);
                        FCEUI_CheatSearchGetRange(scrollindex+32768,scrollindex+32768+13,cfcallb);
                        break;
                  case SB_BOTTOM:
                        scrollindex=scrollmax;
                        SendDlgItemMessage(hwndDlg,120,SBM_SETPOS,scrollindex,1);
                        SendDlgItemMessage(hwndDlg,108,LB_RESETCONTENT,13,0);
                        FCEUI_CheatSearchGetRange(scrollindex+32768,scrollindex+32768+13,cfcallb);
                        break;
                  case SB_LINEUP:
                        if(scrollindex>-32768)
                        {
                         scrollindex--;
                         SendDlgItemMessage(hwndDlg,120,SBM_SETPOS,scrollindex,1);
                         SendDlgItemMessage(hwndDlg,108,LB_DELETESTRING,13,0);
                         FCEUI_CheatSearchGetRange(scrollindex+32768,scrollindex+32768,cfcallbinsertt);
                        }
                        break;

                  case SB_PAGEUP:
                        scrollindex-=14;
                        if(scrollindex<-32768) scrollindex=-32768;
                        SendDlgItemMessage(hwndDlg,120,SBM_SETPOS,scrollindex,1);
                        SendDlgItemMessage(hwndDlg,108,LB_RESETCONTENT,13,0);
                        FCEUI_CheatSearchGetRange(scrollindex+32768,scrollindex+32768+13,cfcallb);
                        break;

                  case SB_LINEDOWN:
                        if(scrollindex<scrollmax)
                        {
                         scrollindex++;
                         SendDlgItemMessage(hwndDlg,120,SBM_SETPOS,scrollindex,1);
                         SendDlgItemMessage(hwndDlg,108,LB_DELETESTRING,0,0);
                         FCEUI_CheatSearchGetRange(scrollindex+32768+13,scrollindex+32768+13,cfcallbinsert);
                        }
                        break;

                  case SB_PAGEDOWN:
                        scrollindex+=14;
                        if(scrollindex>scrollmax)
                         scrollindex=scrollmax;
                        SendDlgItemMessage(hwndDlg,120,SBM_SETPOS,scrollindex,1);
                        SendDlgItemMessage(hwndDlg,108,LB_RESETCONTENT,0,0);
                        FCEUI_CheatSearchGetRange(scrollindex+32768,scrollindex+32768+13,cfcallb);
                        break;

                  case SB_THUMBPOSITION:
                  case SB_THUMBTRACK:
                        scrollindex=(short int)HIWORD(wParam);
                        SendDlgItemMessage(hwndDlg,120,SBM_SETPOS,scrollindex,1);
                        SendDlgItemMessage(hwndDlg,108,LB_RESETCONTENT,0,0);
                        FCEUI_CheatSearchGetRange(32768+scrollindex,32768+scrollindex+13,cfcallb);
                        break;
                 }

                }                
                break;

   case WM_INITDIALOG:
                SetDlgItemText(hwndDlg,110,(LPTSTR)U8ToStr(cheatval1));
                SetDlgItemText(hwndDlg,111,(LPTSTR)U8ToStr(cheatval2));
                DoGet();
                CheckRadioButton(hwndDlg,115,118,scheatmethod+115);
                lbfocus=0;
                hwndLB=0;
                break;

   case WM_VKEYTOITEM:
                if(lbfocus)
                {
                 int real;

                 real=SendDlgItemMessage(hwndDlg,108,LB_GETCURSEL,0,(LPARAM)(LPSTR)0);
                 switch((int)LOWORD(wParam))
                 {
                  case VK_UP: 
                              /* mmmm....recursive goodness */
                              if(!real)
                               SendMessage(hwndDlg,WM_VSCROLL,SB_LINEUP,0);
                              return(-1);
                              break;
                  case VK_DOWN:
                              if(real==13)
                               SendMessage(hwndDlg,WM_VSCROLL,SB_LINEDOWN,0);
                              return(-1);
                              break;
                  case VK_PRIOR:
                              SendMessage(hwndDlg,WM_VSCROLL,SB_PAGEUP,0);
                              break;
                  case VK_NEXT:
                              SendMessage(hwndDlg,WM_VSCROLL,SB_PAGEDOWN,0);
                              break;
                  case VK_HOME:
                              SendMessage(hwndDlg,WM_VSCROLL,SB_TOP,0);
                              break;
                  case VK_END:
                              SendMessage(hwndDlg,WM_VSCROLL,SB_BOTTOM,0);
                              break;
                 }
                 return(-2);
                }
                break;

   case WM_CLOSE:
   case WM_QUIT: goto gornk;
   case WM_COMMAND:
                switch(LOWORD(wParam))
                {
                 case 108:
                        switch(HIWORD(wParam))
                        {
                         case LBN_SELCHANGE:
                                 {
                                  char TempArray[32];
                                  SendDlgItemMessage(hwndDlg,108,LB_GETTEXT,SendDlgItemMessage(hwndDlg,108,LB_GETCURSEL,0,(LPARAM)(LPSTR)0),(LPARAM)(LPCTSTR)TempArray);
                                  TempArray[4]=0;
                                  SetDlgItemText(hwndDlg,201,(LPTSTR)TempArray);                                 
                                 }
                                 break;
                         case LBN_SETFOCUS:
                                 lbfocus=1;
                                 break;
                         case LBN_KILLFOCUS:
                                 lbfocus=0;
                                 break;
                        }
                        break;
                }

                switch(HIWORD(wParam))
                {
                 case BN_CLICKED:
                        if(LOWORD(wParam)>=115 && LOWORD(wParam)<=118)
                         scheatmethod=LOWORD(wParam)-115;
                        break;
                 case EN_CHANGE:
                        {
                         char TempArray[256];
                         GetDlgItemText(hwndDlg,LOWORD(wParam),TempArray,256);
                         switch(LOWORD(wParam))
                         {
                          case 110:cheatval1=StrToU8(TempArray);break;
                          case 111:cheatval2=StrToU8(TempArray);break;
                         }
                        }
                        break;
                }


                if(!(wParam>>16))
                switch(wParam&0xFFFF)
                {
                 case 112:
                          FCEUI_CheatSearchBegin();
                          DoGet();
                          break;
                 case 113:
                          FCEUI_CheatSearchEnd(scheatmethod,cheatval1,cheatval2);
                          DoGet();
                          break;
                 case 114:
                          FCEUI_CheatSearchSetCurrentAsOriginal();
                          DoGet();
                          break;
                 case 107:
                          FCEUI_CheatSearchShowExcluded();
                          DoGet();
                          break;
                 case 105:
                          {
                           int a,v;
                           char temp[256];

                           GetDlgItemText(hwndDlg,201,temp,4+1);
                           a=StrToU16(temp);
                           GetDlgItemText(hwndDlg,202,temp,3+1);
                           v=StrToU8(temp);

                           GetDlgItemText(hwndDlg,200,temp,256);
                           AddTheThing(hwndDlg,temp,a,v);
                          }
                          break;
                 case 106:
                        gornk:
                        EndDialog(hwndDlg,0);
                        break;
                }
              }
  return 0;
}

static void ConfigAddCheat(HWND wnd)
{
 DialogBox(fceu_hInstance,"ADDCHEAT",wnd,AddCheatCallB);
}
