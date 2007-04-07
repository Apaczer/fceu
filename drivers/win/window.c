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

static void ConfigMisc(void);
static void ConfigPalette(void);
static void ConfigDirectories(void);

static HMENU fceumenu=0;
static HMENU recentmenu;

static int tog=0;

void ShowCursorAbs(int w)
{
 static int stat=0;
 if(w)
 {
  if(stat==-1) {stat++; ShowCursor(1);}
 }
 else
 {
  if(stat==0) {stat--; ShowCursor(0);}
 }
}


RECT *CalcWindowSize(void)
{
 static RECT al;
 al.left=0;
 al.right=VNSWID*winsizemul;
 al.top=0;
 al.bottom=totallines*winsizemul;

 AdjustWindowRectEx(&al,GetWindowLong(hAppWnd,GWL_STYLE),GetMenu(hAppWnd)!=NULL,GetWindowLong(hAppWnd,GWL_EXSTYLE));

 al.right-=al.left;
 al.left=0;
 al.bottom-=al.top;
 al.top=0;

 return(&al);
}

void UpdateMenu(void)
{
 static int *polo[2]={&genie,&palyo};
 static int polo2[2]={310,311};
 int x;

 for(x=0;x<2;x++)
  CheckMenuItem(fceumenu,polo2[x],*polo[x]?MF_CHECKED:MF_UNCHECKED);
 if(eoptions&EO_BGRUN)
  CheckMenuItem(fceumenu,301,MF_CHECKED);
 else
  CheckMenuItem(fceumenu,301,MF_UNCHECKED);
}

char *rfiles[10]={0,0,0,0,0,0,0,0,0,0};

void UpdateRMenu(void)
{
 MENUITEMINFO moo;
 int x;

 moo.cbSize=sizeof(moo);
 moo.fMask=MIIM_SUBMENU|MIIM_STATE;

 GetMenuItemInfo(GetSubMenu(fceumenu,0),102,FALSE,&moo);
 moo.hSubMenu=recentmenu;
 moo.fState=rfiles[0]?MFS_ENABLED:MFS_GRAYED;

 SetMenuItemInfo(GetSubMenu(fceumenu,0),102,FALSE,&moo);

 for(x=0;x<10;x++)
  RemoveMenu(recentmenu,600+x,MF_BYCOMMAND);
 for(x=9;x>=0;x--)
 {  
  char tmp[128+5];
  if(!rfiles[x]) continue;

  moo.cbSize=sizeof(moo);
  moo.fMask=MIIM_DATA|MIIM_ID|MIIM_TYPE;

  if(strlen(rfiles[x])<128)
  {
   sprintf(tmp,"&%d. %s",(x+1)%10,rfiles[x]);
  }
  else
   sprintf(tmp,"&%d. %s",(x+1)%10,rfiles[x]+strlen(rfiles[x])-127);

  moo.cch=strlen(tmp);
  moo.fType=0;
  moo.wID=600+x;
  moo.dwTypeData=tmp;
  InsertMenuItem(recentmenu,0,1,&moo);
 }
 DrawMenuBar(hAppWnd);
}

void AddRecent(char *fn)
{
 int x;

 for(x=0;x<10;x++)
  if(rfiles[x])
   if(!strcmp(rfiles[x],fn))    // Item is already in list.
   {
    int y;
    char *tmp;

    tmp=rfiles[x];              // Save pointer.
    for(y=x;y;y--)
     rfiles[y]=rfiles[y-1];     // Move items down.

    rfiles[0]=tmp;              // Put item on top.
    UpdateRMenu();
    return;
   }

 if(rfiles[9]) free(rfiles[9]);
 for(x=9;x;x--) rfiles[x]=rfiles[x-1];
 rfiles[0]=malloc(strlen(fn)+1);
 strcpy(rfiles[0],fn);
 UpdateRMenu();
}

void HideMenu(int h)
{
  if(h)
  {
   SetMenu(hAppWnd,0);   
  }
  else
  {
   SetMenu(hAppWnd,fceumenu);
  }
}

static LONG WindowXC=1<<30,WindowYC;
void HideFWindow(int h)
{
 LONG desa;

 if(h)
 {
   RECT bo;
   GetWindowRect(hAppWnd,&bo);
   WindowXC=bo.left;
   WindowYC=bo.top;

   SetMenu(hAppWnd,0);
   desa=WS_POPUP|WS_CLIPSIBLINGS;
 }
 else
 {
   desa=WS_OVERLAPPEDWINDOW|WS_CLIPSIBLINGS;
   HideMenu(tog);
 }
 
 SetWindowLong(hAppWnd,GWL_STYLE,desa|(GetWindowLong(hAppWnd,GWL_STYLE)&WS_VISIBLE));
 SetWindowPos(hAppWnd,0,0,0,0,0,SWP_FRAMECHANGED|SWP_NOACTIVATE|SWP_NOCOPYBITS|SWP_NOMOVE|SWP_NOREPOSITION|SWP_NOSIZE|SWP_NOZORDER);
}

void ToggleHideMenu(void)
{ 
 if(!fullscreen)
 {
  tog^=1;
  HideMenu(tog);
  SetMainWindowStuff();
 }
}

static void ALoad(char *nameo)
{
  if((GI=FCEUI_LoadGame(nameo)))
  {
   FixFL();
   FixGIGO();
   SetMainWindowStuff();
   AddRecent(nameo);
   RefreshThrottleFPS();
   if(eoptions&EO_FSAFTERLOAD)
    SetFSVideoMode();
  }
  else
   StopSound();
}

void LoadNewGamey(HWND hParent)
{
 const char filter[]="All usable files(*.nes,*.nsf,*.fds,*.unf,*.zip,*.gz)\0*.nes;*.nsf;*.fds;*.unf;*.zip;*.gz\0All non-compressed usable files(*.nes,*.nsf,*.fds,*.unf)\0*.nes;*.nsf;*.fds;*.unf\0All files (*.*)\0*.*\0";
 char nameo[2048];
 OPENFILENAME ofn;
 memset(&ofn,0,sizeof(ofn));
 ofn.lStructSize=sizeof(ofn);
 ofn.hInstance=fceu_hInstance;
 ofn.lpstrTitle="FCE Ultra Open File...";
 ofn.lpstrFilter=filter;
 nameo[0]=0;
 ofn.hwndOwner=hParent;
 ofn.lpstrFile=nameo;
 ofn.nMaxFile=256;
 ofn.Flags=OFN_EXPLORER|OFN_FILEMUSTEXIST|OFN_HIDEREADONLY; //OFN_EXPLORER|OFN_ENABLETEMPLATE|OFN_ENABLEHOOK;
 ofn.lpstrInitialDir=gfsdir;
 if(GetOpenFileName(&ofn))
 {
  if(gfsdir) free(gfsdir);
  if((gfsdir=malloc(ofn.nFileOffset+1)))
  {
   strncpy(gfsdir,ofn.lpstrFile,ofn.nFileOffset);
   gfsdir[ofn.nFileOffset]=0;
  }
  ALoad(nameo);
 }
}

static uint32 mousex,mousey,mouseb;
void GetMouseData(uint32 *x, uint32 *y, uint32 *b)
{
 *x=mousex;
 *y=mousey;
 if(!fullscreen)
 {
  if(eoptions&EO_USERFORCE)
  {
   RECT t;
   GetClientRect(hAppWnd,&t);

   *x=*x*VNSWID/(t.right?t.right:1);
   *y=*y*totallines/(t.bottom?t.bottom:1);
  }
  else
  {
   *x/=winsizemul;
   *y/=winsizemul;
  }
  *x+=VNSCLIP;
 }

 *y+=srendline;
 *b=((mouseb==MK_LBUTTON)?1:0)|((mouseb==MK_RBUTTON)?2:0);
}

static int sizchange=0;
static int vchanged=0;

LRESULT FAR PASCAL AppWndProc(HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam)
{
  switch(msg) {   
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
                  mouseb=wParam;
                  goto proco;
    case WM_MOUSEMOVE:
                  {
                   mousex=LOWORD(lParam);
                   mousey=HIWORD(lParam);
                  }
                  goto proco;
    case WM_SIZING:
                 sizchange=1;
                 goto proco;
    case WM_DISPLAYCHANGE:
                if(!fullscreen && !changerecursive)
                 vchanged=1;
                goto proco;
    case WM_DROPFILES:
                {
                 UINT len;
                 char *ftmp;

                 len=DragQueryFile((HANDLE)wParam,0,0,0)+1;
                 if((ftmp=malloc(len)))
                 {
                  DragQueryFile((HANDLE)wParam,0,ftmp,len);
                  ALoad(ftmp);
                  free(ftmp);
                 }                 
                }
                break;
    case WM_COMMAND:
                if(!(wParam>>16))
                {
                 wParam&=0xFFFF;
                 if(wParam>=600 && wParam<=609)
                 {
                  if(rfiles[wParam-600]) ALoad(rfiles[wParam-600]);
                 }
                 switch(wParam)
                 {
                  case 300:ToggleHideMenu();break;
                  case 301:eoptions^=EO_BGRUN;UpdateMenu();break;

                  case 310:genie^=1;FCEUI_SetGameGenie(genie);UpdateMenu();break;
                  case 311:palyo^=1;
                           FCEUI_SetVidSystem(palyo);
                           RefreshThrottleFPS();
                           UpdateMenu();
                           FixFL();
                           SetMainWindowStuff();
                           break;

                  case 320:StopSound();ConfigDirectories();break;
                  case 321:StopSound();ConfigInput(hWnd);break;
                  case 322:ConfigMisc();break;
                  case 323:StopSound();ConfigNetplay();break;
                  case 324:StopSound();ConfigPalette();break;
                  case 325:StopSound();ConfigSound();break;
                  case 326:ConfigVideo();break;

                  case 200:DriverInterface(DES_RESET,0);break;
                  case 201:DriverInterface(DES_POWER,0);break;
                  case 202:ConfigCheats(hWnd);break;

                  case 100:StopSound();
                           LoadNewGamey(hWnd);
                           break;
                  case 101:if(GI)
                           {
                            FCEUI_CloseGame();                            
                            GI=0;
                           }
                           break;
                  case 110:FCEUI_SaveState();break;
                  case 111:FCEUI_LoadState();break;

                  case 120:
                           {
                            MENUITEMINFO mi;
                            char *str;
             
                            StopSound();
                            if(CreateSoundSave())
                             str="Stop Sound Logging";
                            else
                             str="Log Sound As...";
                            memset(&mi,0,sizeof(mi));
                            mi.fMask=MIIM_DATA|MIIM_TYPE;
                            mi.cbSize=sizeof(mi);
                            GetMenuItemInfo(fceumenu,120,0,&mi);                           
                            mi.fMask=MIIM_DATA|MIIM_TYPE;
                            mi.cbSize=sizeof(mi);
                            mi.dwTypeData=str;
                            mi.cch=strlen(str);
                            SetMenuItemInfo(fceumenu,120,0,&mi);
                           }
                           break;
                  case 130:DoFCEUExit();break;

                  case 400:StopSound();ShowAboutBox();break;
                 }              
                }
                break;


    case WM_SYSCOMMAND:
               if(wParam==SC_KEYMENU)
                if(GI && InputTypeFC==SIFC_FKB && cidisabled)
                 break;
               goto proco;
    case WM_SYSKEYDOWN:
               if(GI && InputTypeFC==SIFC_FKB && cidisabled)
                break; /* Hopefully this won't break DInput... */

               if(fullscreen || tog)
               {
                if(wParam==VK_MENU)
                 break;
               }
               if(wParam==VK_F10)
               {
                if(!(lParam&0x40000000))
                 DriverInterface(DES_RESET,0);
                break;
               }
               goto proco;

    case WM_KEYDOWN:
              if(GI)
	      {
	       /* Only disable command keys if a game is loaded(and the other
		  conditions are right, of course). */
               if(InputTypeFC==SIFC_FKB)
	       {
		if(wParam==VK_SCROLL)
		{
 		 cidisabled^=1;
		 FCEUI_DispMessage("Family Keyboard %sabled.",cidisabled?"en":"dis");
		}
		if(cidisabled)
                 break; /* Hopefully this won't break DInput... */
	       }
               if(GI->type==GIT_NSF)
                switch(wParam)
                {
                 case VK_UP:DriverInterface(DES_NSFINC,0);break;
                 case VK_DOWN:DriverInterface(DES_NSFDEC,0);break;
                 case VK_RETURN:DriverInterface(DES_NSFRES,0);break;

                 case VK_LEFT:if(!(lParam&0x40000000))
                               DriverInterface(DES_NSFDEC,0); break;
                 case VK_RIGHT:if(!(lParam&0x40000000))
                                DriverInterface(DES_NSFINC,0); break;
                }

	       }
               if(!(lParam&0x40000000))
                switch( wParam )
                {
                  case VK_F11:DriverInterface(DES_POWER,0);break;
                  case VK_F12:DoFCEUExit();break;
                  case VK_F2:userpause^=1;break;
                  case VK_F3:ToggleHideMenu();break;
                  case VK_F4:       UpdateMenu();
                                    changerecursive=1;
                                    if(!SetVideoMode(fullscreen^1))
                                     SetVideoMode(fullscreen);
                                    changerecursive=0;
                                    break;
                }
                goto proco;


    case WM_NCRBUTTONDOWN:
    case WM_NCMBUTTONDOWN:StopSound();goto proco;
    case WM_NCLBUTTONDOWN:StopSound();goto proco;

    case WM_ENTERMENULOOP:StopSound();goto proco;
    case WM_CLOSE:
    case WM_DESTROY:
    case WM_QUIT:DoFCEUExit();break;
    case WM_ACTIVATEAPP:       
       if((BOOL)wParam)
       {
        nofocus=0;
       }
       else
       {
        nofocus=1;
       }
    default:
      proco:
      return DefWindowProc(hWnd,msg,wParam,lParam);
   }
  return 0;
}

void UpdateFCEUWindow(void)
{
  int w,h;
  RECT wrect;

  if(vchanged && !fullscreen && !changerecursive && !nofocus)
  {
   SetVideoMode(0);
   vchanged=0;
  }

  if(sizchange && !fullscreen && !(eoptions&EO_USERFORCE))
  {      
   GetWindowRect(hAppWnd,&wrect);
   h=wrect.bottom-wrect.top;
   w=wrect.right-wrect.left;
   if(w!=winwidth)
    winsizemul=(w-(winwidth-VNSWID*winsizemul)+(VNSWID>>1))>>8;
   else
   if(h!=winheight)
    winsizemul=(h-(winheight-totallines*winsizemul)+(totallines>>1))>>8;

   if(winsizemul<1)
    winsizemul=1;
   SetMainWindowStuff();
  }
  sizchange=0;

  BlockingCheck();
}

void ByebyeWindow(void)
{
 SetMenu(hAppWnd,0);
 DestroyMenu(fceumenu);
 DestroyWindow(hAppWnd);
}

int CreateMainWindow(void)
{
  WNDCLASSEX winclass;
  RECT tmp;

  memset(&winclass,0,sizeof(winclass));
  winclass.cbSize=sizeof(WNDCLASSEX);
  winclass.style=CS_OWNDC|CS_HREDRAW|CS_VREDRAW|CS_SAVEBITS;
  winclass.lpfnWndProc=AppWndProc;
  winclass.cbClsExtra=0;
  winclass.cbWndExtra=0;
  winclass.hInstance=fceu_hInstance;
  winclass.hIcon=LoadIcon(fceu_hInstance, "ICON_1");
  winclass.hIconSm=LoadIcon(fceu_hInstance, "ICON_1");
  winclass.hCursor=LoadCursor(NULL, IDC_ARROW);
  winclass.hbrBackground=GetStockObject(BLACK_BRUSH);
  //winclass.lpszMenuName="FCEUMENU";
  winclass.lpszClassName="FCEULTRA";

  if(!RegisterClassEx(&winclass))
    return FALSE;

  AdjustWindowRectEx(&tmp,WS_OVERLAPPEDWINDOW,1,0);

  fceumenu=LoadMenu(fceu_hInstance,"FCEUMENU");
  recentmenu=CreateMenu();
  UpdateRMenu();

  hAppWnd = CreateWindowEx(0,"FCEULTRA","FCE Ultra",
                        WS_OVERLAPPEDWINDOW|WS_CLIPSIBLINGS,  /* Style */
                        CW_USEDEFAULT,CW_USEDEFAULT,256,240,  /* X,Y ; Width, Height */
                        NULL,fceumenu,fceu_hInstance,NULL );  
  DragAcceptFiles(hAppWnd, 1);
  SetMainWindowStuff();
  return 1;
}


int SetMainWindowStuff(void)
{
  RECT *srect;
  RECT tmp;

  GetWindowRect(hAppWnd,&tmp);

  if(WindowXC!=(1<<30))
  {
   /* Subtracting and adding for if(eoptions&EO_USERFORCE) below. */
   tmp.bottom-=tmp.top;
   tmp.bottom+=WindowYC;

   tmp.right-=tmp.left;
   tmp.right+=WindowXC;
   

   tmp.left=WindowXC;
   tmp.top=WindowYC;
   WindowXC=1<<30;
  }

  if(eoptions&EO_USERFORCE)
  {
   SetWindowPos(hAppWnd,HWND_TOP,tmp.left,tmp.top,0,0,SWP_NOSIZE|SWP_SHOWWINDOW);
   winwidth=tmp.right-tmp.left;
   winheight=tmp.bottom-tmp.top;
  }
  else
  {
   srect=CalcWindowSize();
   SetWindowPos(hAppWnd,HWND_TOP,tmp.left,tmp.top,srect->right,srect->bottom,SWP_SHOWWINDOW);
   winwidth=srect->right;
   winheight=srect->bottom;
  }


  ShowWindow(hAppWnd, SW_SHOWNORMAL);
  return 1;
}

int GetClientAbsRect(LPRECT lpRect)
{
  POINT point;
  point.x=point.y=0;
  if(!ClientToScreen(hAppWnd,&point)) return 0;

  lpRect->top=point.y;
  lpRect->left=point.x;

  if(eoptions&EO_USERFORCE)
  {
   RECT al;

   GetClientRect(hAppWnd,&al);

   lpRect->right=point.x+al.right;
   lpRect->bottom=point.y+al.bottom;
  }
  else
  {
   lpRect->right=point.x+VNSWID*winsizemul;
   lpRect->bottom=point.y+totallines*winsizemul;
  }
  return 1;
}


void LoadPaletteFile(void)
{
 FILE *fp;
 const char filter[]="All usable files(*.pal)\0*.pal\0All files (*.*)\0*.*\0";
 char nameo[2048];
 OPENFILENAME ofn;
 memset(&ofn,0,sizeof(ofn));
 ofn.lStructSize=sizeof(ofn);
 ofn.hInstance=fceu_hInstance;
 ofn.lpstrTitle="FCE Ultra Open Palette File...";
 ofn.lpstrFilter=filter;
 nameo[0]=0;
 ofn.lpstrFile=nameo;
 ofn.nMaxFile=256;
 ofn.Flags=OFN_EXPLORER|OFN_FILEMUSTEXIST|OFN_HIDEREADONLY;
 ofn.lpstrInitialDir=0;
 if(GetOpenFileName(&ofn))
 {
  if((fp=fopen(nameo,"rb")))
  {
   fread(cpalette,1,192,fp);
   fclose(fp);
   FCEUI_SetPaletteArray(cpalette);
   eoptions|=EO_CPALETTE;
  }
  else
   FCEUD_PrintError("Error opening palette file!");
 }
}

BOOL CALLBACK PaletteConCallB(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch(uMsg) {
   case WM_INITDIALOG:                
                if(ntsccol)
                 CheckDlgButton(hwndDlg,100,BST_CHECKED);
                SendDlgItemMessage(hwndDlg,500,TBM_SETRANGE,1,MAKELONG(0,128));
                SendDlgItemMessage(hwndDlg,501,TBM_SETRANGE,1,MAKELONG(0,128));
                DriverInterface(DES_GETNTSCTINT,&ntsctint);
                DriverInterface(DES_GETNTSCHUE,&ntschue);
                SendDlgItemMessage(hwndDlg,500,TBM_SETPOS,1,ntsctint);
                SendDlgItemMessage(hwndDlg,501,TBM_SETPOS,1,ntschue);
                break;
   case WM_CLOSE:
   case WM_QUIT: goto gornk;
   case WM_COMMAND:
                if(!(wParam>>16))
                switch(wParam&0xFFFF)
                {
                 case 100:ntsccol^=1;DriverInterface(DES_NTSCCOL,&ntsccol);break;
                 case 200:
                          LoadPaletteFile();
                          break;
                 case 201:FCEUI_SetPaletteArray(0);
                          eoptions&=~EO_CPALETTE;
                          break;
                 case 1:
                        gornk:
                        ntsctint=SendDlgItemMessage(hwndDlg,500,TBM_GETPOS,0,(LPARAM)(LPSTR)0);
                        ntschue=SendDlgItemMessage(hwndDlg,501,TBM_GETPOS,0,(LPARAM)(LPSTR)0);
                        EndDialog(hwndDlg,0);
                        break;
               }
              }
  return 0;
}

static void ConfigPalette(void)
{
  DialogBox(fceu_hInstance,"PALCONFIG",hAppWnd,PaletteConCallB);
  DriverInterface(DES_SETNTSCTINT,&ntsctint);
  DriverInterface(DES_SETNTSCHUE,&ntschue);
}


static BOOL CALLBACK MiscConCallB(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch(uMsg) {
   case WM_INITDIALOG:                
                if(eoptions&EO_NOSPRLIM)
                 CheckDlgButton(hwndDlg,100,BST_CHECKED);
                if(eoptions&EO_FOAFTERSTART)
                 CheckDlgButton(hwndDlg,102,BST_CHECKED);
                if(eoptions&EO_SNAPNAME)
                 CheckDlgButton(hwndDlg,103,BST_CHECKED);
                if(eoptions&EO_NOTHROTTLE)
                 CheckDlgButton(hwndDlg,101,BST_CHECKED);
                break;
   case WM_CLOSE:
   case WM_QUIT: goto gornk;
   case WM_COMMAND:
                if(!(wParam>>16))
                switch(wParam&0xFFFF)
                {
                 case 1:
                        gornk:
                        if(IsDlgButtonChecked(hwndDlg,100)==BST_CHECKED)
                         eoptions|=EO_NOSPRLIM;
                        else
                         eoptions&=~EO_NOSPRLIM;
                        if(IsDlgButtonChecked(hwndDlg,102)==BST_CHECKED)
                         eoptions|=EO_FOAFTERSTART;
                        else
                         eoptions&=~EO_FOAFTERSTART;
                        if(IsDlgButtonChecked(hwndDlg,103)==BST_CHECKED)
                         eoptions|=EO_SNAPNAME;
                        else
                         eoptions&=~EO_SNAPNAME;
                        if(IsDlgButtonChecked(hwndDlg,101)==BST_CHECKED)
                         eoptions|=EO_NOTHROTTLE;
                        else
                         eoptions&=~EO_NOTHROTTLE;
                        EndDialog(hwndDlg,0);
                        break;
               }
              }
  return 0;
}

void DoMiscConfigFix(void)
{
  FCEUI_DisableSpriteLimitation(eoptions&EO_NOSPRLIM);
  FCEUI_SetSnapName(eoptions&EO_SNAPNAME);
}

static void ConfigMisc(void)
{
  DialogBox(fceu_hInstance,"MISCCONFIG",hAppWnd,MiscConCallB);  
  DoMiscConfigFix();
}

static int BrowseForFolder(HWND hParent, char *htext, char *buf)
{
 BROWSEINFO bi;
 LPCITEMIDLIST pidl;
 int ret=1;

 buf[0]=0;

 memset(&bi,0,sizeof(bi));
                
 bi.hwndOwner=hParent;
 bi.lpszTitle=htext;
 bi.ulFlags=BIF_RETURNONLYFSDIRS; 

 if(FAILED(CoInitialize(0)))
  return(0);

 if(!(pidl=SHBrowseForFolder(&bi)))
 {
  ret=0;
  goto end1;
 }

 if(!SHGetPathFromIDList(pidl,buf))
 {
  ret=0;
  goto end2;
 }

 end2:
 /* This probably isn't the best way to free the memory... */
 CoTaskMemFree((PVOID)pidl);

 end1:
 CoUninitialize();
 return(ret);
}

static BOOL CALLBACK DirConCallB(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  int x;

  switch(uMsg){
   case WM_INITDIALOG:                
                for(x=0;x<6;x++)
                 SetDlgItemText(hwndDlg,100+x,DOvers[x]);

                if(eoptions&EO_BSAV)
                 CheckDlgButton(hwndDlg,300,BST_CHECKED);
                break;
   case WM_CLOSE:
   case WM_QUIT: goto gornk;
   case WM_COMMAND:
                if(!(wParam>>16))
                {
                 if((wParam&0xFFFF)>=200 && (wParam&0xFFFF)<=205)
                 {
                  static char *helpert[6]={"Cheats","Miscellaneous","Nonvolatile Game Data","Save States","Screen Snapshots","Base Directory"};
                  char name[MAX_PATH];

                  if(BrowseForFolder(hwndDlg,helpert[((wParam&0xFFFF)-200)],name))
                   SetDlgItemText(hwndDlg,100+((wParam&0xFFFF)-200),name);
                 }
                 else switch(wParam&0xFFFF)
                 {
                  case 1:
                        gornk:

                        RemoveDirs();   // Remove empty directories.

                        for(x=0;x<6;x++)
                        {
                         LONG len;
                         len=SendDlgItemMessage(hwndDlg,100+x,WM_GETTEXTLENGTH,0,0);
                         if(len<=0)
                         {
                          if(DOvers[x]) free(DOvers[x]);
                          DOvers[x]=0;
                          continue;
                         }
                         len++; // Add 1 for null character.
                         if(!(DOvers[x]=malloc(len)))
                          continue;
                         if(!GetDlgItemText(hwndDlg,100+x,DOvers[x],len))
                         {
                          free(DOvers[x]);
                          DOvers[x]=0;
                          continue;
                         }

                        }
                        if(IsDlgButtonChecked(hwndDlg,300)==BST_CHECKED)
                         eoptions|=EO_BSAV;
                        else
                         eoptions&=~EO_BSAV;

                        CreateDirs();   // Create needed directories.
                        SetDirs();      // Set the directories in the core.
                        EndDialog(hwndDlg,0);
                        break;
                 }
                }
              }
  return 0;
}



static void ConfigDirectories(void)
{
  DialogBox(fceu_hInstance,"DIRCONFIG",hAppWnd,DirConCallB);
}
