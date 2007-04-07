/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 1998 BERO 
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

/*                      SVGA High Level Routines
                          FCE / FCE Ultra
*/
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include <stdarg.h>


#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#include "types.h"
#include "svga.h" 
#include "fce.h"
#include "general.h"
#include "video.h"
#include "sound.h"
#include "version.h"
#include "nsf.h"
#include "palette.h"
#include "fds.h"
#include "netplay.h"
#include "state.h"
#include "cart.h"
#include "input.h"

FCEUS FSettings;

static int howlong;
static char errmsg[65];

void FCEU_PrintError(char *format, ...)
{
 char temp[2048];

 va_list ap;

 va_start(ap,format);
 vsprintf(temp,format,ap);
 FCEUD_PrintError(temp);

 va_end(ap);
}

void FCEU_DispMessage(char *format, ...)
{
 va_list ap;

 va_start(ap,format);
 vsprintf(errmsg,format,ap);
 va_end(ap);

 howlong=180;
}

void FCEUI_SetRenderedLines(int ntscf, int ntscl, int palf, int pall)
{
 FSettings.UsrFirstSLine[0]=ntscf;
 FSettings.UsrLastSLine[0]=ntscl;
 FSettings.UsrFirstSLine[1]=palf;
 FSettings.UsrLastSLine[1]=pall;
 if(PAL)
 {
  FSettings.FirstSLine=FSettings.UsrFirstSLine[1];
  FSettings.LastSLine=FSettings.UsrLastSLine[1];
 }
 else
 {
  FSettings.FirstSLine=FSettings.UsrFirstSLine[0];
  FSettings.LastSLine=FSettings.UsrLastSLine[0];
 }

}

void FCEUI_SetVidSystem(int a)
{
 FSettings.PAL=a?1:0;
 FCEU_ResetVidSys();
 FCEU_ResetPalette();
}

int FCEUI_GetCurrentVidSystem(int *slstart, int *slend)
{
 if(slstart)
  *slstart=FSettings.FirstSLine;
 if(slend)
  *slend=FSettings.LastSLine;
 return(PAL);
}

#ifdef NETWORK
void FCEUI_SetNetworkPlay(int type)
{
 FSettings.NetworkPlay=type;
}
#endif

void FCEUI_SetGameGenie(int a)
{
 FSettings.GameGenie=a?1:0;
}

static void CalculatePalette(void);
static void ChoosePalette(void);
static void WritePalette(void);

#ifndef NETWORK
#define netplay 0
#endif

static uint8 StateShow=0;

uint8 Exit=0;

uint8 DIPS=0;
uint8 vsdip=0;
int coinon=0;

uint8 pale=0;
uint8 CommandQueue=0;

static int controlselect=0;
static int ntsccol=0;
static int ntsctint=46+10;
static int ntschue=72;
static int controllength=0;

pal *palo;
static pal *palpoint[8]=
     {
     palette,
     palettevscv,
     palettevssmb,
     palettevsmar,
     palettevsgoon,
     palettevsslalom,
     palettevseb,
     rp2c04001
     };

void FCEUI_SetSnapName(int a)
{
 FSettings.SnapName=a;
}

void FCEUI_SaveExtraDataUnderBase(int a)
{
 FSettings.SUnderBase=a;
}

void FCEUI_SetPaletteArray(uint8 *pal)
{
 if(!pal)
  palpoint[0]=palette;
 else
 {
  int x;
  palpoint[0]=palettec;
  for(x=0;x<64;x++)
  {
   palpoint[0][x].r=*((uint8 *)pal+x+x+x);
   palpoint[0][x].g=*((uint8 *)pal+x+x+x+1);
   palpoint[0][x].b=*((uint8 *)pal+x+x+x+2);
  }
 }
 FCEU_ResetPalette();
}

void FCEUI_SelectState(int w)
{
 if(netplay!=2 && FCEUGameInfo.type!=GIT_NSF) 
  CommandQueue=42+w;
}

void FCEUI_SaveState(void)
{
 if(netplay!=2 && FCEUGameInfo.type!=GIT_NSF)
  CommandQueue=40;
}

void FCEUI_LoadState(void)
{
 if(netplay!=2 && FCEUGameInfo.type!=GIT_NSF) 
  CommandQueue=41;
}

int32 FCEUI_GetDesiredFPS(void)
{
  if(PAL)
   return(838977920); // ~50.007
  else
   return(1008307711);	// ~60.1
}

static int dosnapsave=0;
void FCEUI_SaveSnapshot(void)
{
 dosnapsave=1;
}

/* I like the sounds of breaking necks. */
static void ReallySnap(void)
{
 int x=SaveSnapshot();
 if(!x)
  FCEU_DispMessage("Error saving screen snapshot.");
 else
  FCEU_DispMessage("Screen snapshot %d saved.",x-1);
}

void DriverInterface(int w, void *d)
{
 switch(w)
 {
  case DES_NTSCCOL:ntsccol=*(int *)d;FCEU_ResetPalette();break;
  case DES_RESET:if(netplay!=2) CommandQueue=30;break;
  case DES_POWER:if(netplay!=2) CommandQueue=31;break;
  case DES_GETNTSCTINT:*(int*)d=ntsctint;break;
  case DES_GETNTSCHUE:*(int*)d=ntschue;break;
  case DES_SETNTSCTINT:ntsctint=*(int*)d;if(ntsccol)FCEU_ResetPalette();break;
  case DES_SETNTSCHUE:ntschue=*(int*)d;if(ntsccol)FCEU_ResetPalette();break;

  case DES_FDSINSERT:if(netplay!=2) CommandQueue=2;break;
  case DES_FDSEJECT:if(netplay!=2) CommandQueue=3;break;
  case DES_FDSSELECT:if(netplay!=2) CommandQueue=1;break;

  case DES_NSFINC:NSFControl(1);break;
  case DES_NSFDEC:NSFControl(2);break;
  case DES_NSFRES:NSFControl(0);break;  

  case DES_VSUNIDIPSET:CommandQueue=10+(int)d;break;
  case DES_VSUNITOGGLEDIPVIEW:CommandQueue=10;break;
  case DES_VSUNICOIN:CommandQueue=19;break;
  case DES_NTSCSELHUE:if(ntsccol && FCEUGameInfo.type!=GIT_VSUNI && !PAL && FCEUGameInfo.type!=GIT_NSF){controlselect=1;controllength=360;}break;
  case DES_NTSCSELTINT:if(ntsccol && FCEUGameInfo.type!=GIT_VSUNI && !PAL && FCEUGameInfo.type!=GIT_NSF){controlselect=2;controllength=360;}break;

  case DES_NTSCDEC:
		  if(ntsccol && FCEUGameInfo.type!=GIT_VSUNI && !PAL && FCEUGameInfo.type!=GIT_NSF)
		  {
		   char which;
		   if(controlselect)
		   {
		    if(controllength)
		    {
		     which=controlselect==1?ntschue:ntsctint;
		     which--;
		     if(which<0) which=0;
			 if(controlselect==1) 
			  ntschue=which;		     
			 else ntsctint=which;		     
		     CalculatePalette();
		    }
		   controllength=360;
		    }
		   }
		  break;
  case DES_NTSCINC:	
		   if(ntsccol && FCEUGameInfo.type!=GIT_VSUNI && !PAL && FCEUGameInfo.type!=GIT_NSF)
		     if(controlselect)
		     {
		      if(controllength)
		      {
		       switch(controlselect)
		       {
		        case 1:ntschue++;
		               if(ntschue>128) ntschue=128;
		               CalculatePalette();
		               break;
		        case 2:ntsctint++;
		               if(ntsctint>128) ntsctint=128;
		               CalculatePalette();
		               break;
		       }
		      }
		      controllength=360;
		     }
          	    break;
  }
}

static uint8 lastd=0;
void SetNESDeemph(uint8 d, int force)
{
 static uint16 rtmul[7]={32768*1.239,32768*.794,32768*1.019,32768*.905,32768*1.023,32768*.741,32768*.75};
 static uint16 gtmul[7]={32768*.915,32768*1.086,32768*.98,32768*1.026,32768*.908,32768*.987,32768*.75};
 static uint16 btmul[7]={32768*.743,32768*.882,32768*.653,32768*1.277,32768*.979,32768*.101,32768*.75};
 uint32 r,g,b;
 int x;

 /* If it's not forced(only forced when the palette changes), 
    don't waste cpu time if the same deemphasis bits are set as the last call. 
 */
 if(!force) 
 {
  if(d==lastd) 
   return;
 }
 else	/* Only set this when palette has changed. */
 {
  r=rtmul[6];
  g=rtmul[6];
  b=rtmul[6];

  for(x=0;x<0x40;x++)
  {
   uint32 m,n,o;
   m=palo[x].r;
   n=palo[x].g;
   o=palo[x].b;
   m=(m*r)>>15;
   n=(n*g)>>15;
   o=(o*b)>>15;
   if(m>0xff) m=0xff;
   if(n>0xff) n=0xff;
   if(o>0xff) o=0xff;
   FCEUD_SetPalette(x|0x40,m,n,o);
  }
 }
 if(!d) return;	/* No deemphasis, so return. */

  r=rtmul[d-1];
  g=gtmul[d-1];
  b=btmul[d-1];

    for(x=0;x<0x40;x++)
    {
     uint32 m,n,o;

     m=palo[x].r;
     n=palo[x].g;
     o=palo[x].b;
     m=(m*r)>>15;
     n=(n*g)>>15;
     o=(o*b)>>15;
     if(m>0xff) m=0xff;
     if(n>0xff) n=0xff;
     if(o>0xff) o=0xff;

     FCEUD_SetPalette(x|0xC0,m,n,o);
    }
 
 lastd=d;
}

#define HUEVAL  ((double)((double)ntschue/(double)2)+(double)300)
#define TINTVAL ((double)((double)ntsctint/(double)128))

static void CalculatePalette(void)
{
 int x,z;
 int r,g,b;
 double s,y,theta;
 static uint8 cols[16]={0,24,21,18,15,12,9,6,3,0,33,30,27,0,0,0};
 static uint8 br1[4]={6,9,12,12};
 static double br2[4]={.29,.45,.73,.9};
 static double br3[4]={0,.24,.47,.77};
  
 for(x=0;x<=3;x++)
  for(z=0;z<16;z++)
  {
   s=(double)TINTVAL;
   y=(double)br2[x];
   if(z==0)  {s=0;y=((double)br1[x])/12;}
   
   if(z>=13)
   {
    s=y=0;
    if(z==13)
     y=(double)br3[x];     
   }

   theta=(double)M_PI*(double)(((double)cols[z]*10+HUEVAL)/(double)180);
   r=(int)(((double)y+(double)s*(double)sin(theta))*(double)256);
   g=(int)(((double)y-(double)((double)27/(double)53)*s*(double)sin(theta)+(double)((double)10/(double)53)*s*cos(theta))*(double)256);
   b=(int)(((double)y-(double)s*(double)cos(theta))*(double)256);  

   // TODO:  Fix RGB to compensate for phosphor changes(add to red??).

   if(r>255) r=255;
   if(g>255) g=255;
   if(b>255) b=255;
   if(r<0) r=0;
   if(g<0) g=0;
   if(b<0) b=0;
    
   paletten[(x<<4)+z].r=r;
   paletten[(x<<4)+z].g=g;
   paletten[(x<<4)+z].b=b;
  }
 WritePalette();
}

#include "drawing.h"
#ifdef FRAMESKIP
void FCEU_PutImageDummy(void)
{
 if(FCEUGameInfo.type!=GIT_NSF)
 {
  if(controllength) controllength--;
 }
 if(StateShow) StateShow--; /* DrawState() */
 if(howlong) howlong--;	/* DrawMessage() */
 #ifdef FPS
 {
  extern uint64 frcount;
  frcount++;
 }
 #endif

}
#endif

void FCEU_PutImage(void)
{
        if(FCEUGameInfo.type==GIT_NSF)
	{
         DrawNSF(XBuf);
	 /* Save snapshot after NSF screen is drawn.  Why would we want to
	    do it before?
	 */
         if(dosnapsave)
         {
          ReallySnap();
          dosnapsave=0;
         }
	}
        else
        {	
	 /* Save snapshot before overlay stuff is written. */
         if(dosnapsave)
         {
          ReallySnap();
          dosnapsave=0;
         }
	 if(FCEUGameInfo.type==GIT_VSUNI && DIPS&2)         
	  DrawDips();
         if(StateShow) DrawState();
         if(controllength) {controllength--;DrawBars();}
        }
	DrawMessage();
	#ifdef FPS
	{
	extern uint64 frcount;
	frcount++;
	}
	#endif
	DrawInput(XBuf+8);
}

static int ipalette=0;

void LoadGamePalette(void)
{
  uint8 ptmp[192];
  FILE *fp;
  ipalette=0;
  if((fp=fopen(FCEU_MakeFName(FCEUMKF_PALETTE,0,0),"rb")))
  {
   int x;
   fread(ptmp,1,192,fp);
   fclose(fp);
   for(x=0;x<64;x++)
   {
    palettei[x].r=ptmp[x+x+x];
    palettei[x].g=ptmp[x+x+x+1];
    palettei[x].b=ptmp[x+x+x+2];
   }
   ipalette=1;
  }
}

void FCEU_ResetPalette(void)
{
   ChoosePalette();
   WritePalette();
}

static void ChoosePalette(void)
{
    if(FCEUGameInfo.type==GIT_NSF)
     palo=NSFPalette;   
    else if(ipalette)
     palo=palettei;
    else if(ntsccol && !PAL && FCEUGameInfo.type!=GIT_VSUNI)
     {
      palo=paletten;
      CalculatePalette();
     }
    else
     palo=palpoint[pale];  
}

void WritePalette(void)
{
    int x;

    for(x=0;x<6;x++)
     FCEUD_SetPalette(x+128,unvpalette[x].r,unvpalette[x].g,unvpalette[x].b);
    if(FCEUGameInfo.type==GIT_NSF)
    {
     for(x=0;x<39;x++)
      FCEUD_SetPalette(x,palo[x].r,palo[x].g,palo[x].b); 
    }
    else
    {
     for(x=0;x<64;x++)
      FCEUD_SetPalette(x,palo[x].r,palo[x].g,palo[x].b);
     SetNESDeemph(lastd,1);
    }
}

void FlushCommandQueue(void)
{
  if(!netplay && CommandQueue) {DoCommand(CommandQueue);CommandQueue=0;}
}

void DoCommand(uint8 c)
{
 switch(c)
 {
  case 1:FDSControl(FDS_SELECT);break;
  case 2:FDSControl(FDS_IDISK);break;
  case 3:FDSControl(FDS_EJECT);break;

  case 10:DIPS^=2;break;
  case 11:vsdip^=1;DIPS|=2;break;
  case 12:vsdip^=2;DIPS|=2;break;
  case 13:vsdip^=4;DIPS|=2;break;
  case 14:vsdip^=8;DIPS|=2;break;
  case 15:vsdip^=0x10;DIPS|=2;break;
  case 16:vsdip^=0x20;DIPS|=2;break;
  case 17:vsdip^=0x40;DIPS|=2;break;
  case 18:vsdip^=0x80;DIPS|=2;break;
  case 19:coinon=6;break;
  case 30:ResetNES();break;
  case 31:PowerNES();break;
  case 40:CheckStates();StateShow=0;SaveState();break;
  case 41:CheckStates();StateShow=0;LoadState();break;
  case 42: case 43: case 44: case 45: case 46: case 47: case 48: case 49:
  case 50: case 51:StateShow=180;CurrentState=c-42;CheckStates();break;
 }
}
