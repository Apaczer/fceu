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

#include        <string.h>
#include	<stdio.h>
#include	<stdlib.h>

#include	"types.h"
#include	"x6502.h"
#include	"fce.h"
#include	"sound.h"
#include        "svga.h"
#include	"netplay.h"
#include	"general.h"
#include	"endian.h"
#include	"version.h"
#include        "memory.h"

#include	"cart.h"
#include	"nsf.h"
#include	"fds.h"
#include	"ines.h"
#include	"unif.h"
#include        "cheat.h"

#include	"state.h"
#include        "video.h"
#include	"input.h"
#include	"file.h"
#include	"crc32.h"

#define Pal     (PALRAM)

static void FetchSpriteData(void);
static void FASTAPASS(1) RefreshLine(uint8 *target);
static void PRefreshLine(void);
static void FASTAPASS(1) RefreshSprite(uint8 *target);
static void ResetPPU(void);
static void PowerPPU(void);

uint64 timestampbase=0;

int MMC5Hack;
uint32 MMC5HackVROMMask;
uint8 *MMC5HackExNTARAMPtr;
uint8 *MMC5HackVROMPTR;
uint8 MMC5HackCHRMode=0; 
uint8 MMC5HackSPMode;
uint8 MMC5HackSPScroll;
uint8 MMC5HackSPPage;

uint8 *MMC5SPRVPage[8];
uint8 *MMC5BGVPage[8];


uint8 VRAMBuffer,PPUGenLatch;

uint8 *vnapage[4];
uint8 PPUNTARAM;
uint8 PPUCHRRAM;

/* Color deemphasis emulation.  Joy... */
static uint8 deemp=0;
static int deempcnt[8];

static int tosprite=256;

FCEUGI FCEUGameInfo;
void (*GameInterface)(int h);

void FP_FASTAPASS(1) (*PPU_hook)(uint32 A);

void (*GameStateRestore)(int version);
void (*GameHBIRQHook)(void);

readfunc ARead[0x10000];
writefunc BWrite[0x10000];
static readfunc *AReadG;
static writefunc *BWriteG;
static int RWWrap=0;

DECLFW(BNull)
{

}

DECLFR(ANull)
{
 return(X.DB);
}

int AllocGenieRW(void)
{
 if(!(AReadG=FCEU_malloc(0x8000*sizeof(readfunc))))
  return 0;
 if(!(BWriteG=FCEU_malloc(0x8000*sizeof(writefunc))))
  return 0;
 RWWrap=1;
 return 1;
}

void FlushGenieRW(void)
{
 int32 x;

 if(RWWrap)
 {
  for(x=0;x<0x8000;x++)
  {
   ARead[x+0x8000]=AReadG[x];
   BWrite[x+0x8000]=BWriteG[x];
  }
  free(AReadG);
  free(BWriteG);
  AReadG=0;
  BWriteG=0;
  RWWrap=0;
 }
}

readfunc FASTAPASS(1) GetReadHandler(int32 a)
{
  if(a>=0x8000 && RWWrap)
   return AReadG[a-0x8000];
  else
   return ARead[a];
}

void FASTAPASS(3) SetReadHandler(int32 start, int32 end, readfunc func)
{
  int32 x;

  if(!func)
   func=ANull;

  if(RWWrap)
   for(x=end;x>=start;x--)
   {
    if(x>=0x8000)
     AReadG[x-0x8000]=func;
    else
     ARead[x]=func;
   }
  else

   for(x=end;x>=start;x--)
    ARead[x]=func;
}

writefunc FASTAPASS(1) GetWriteHandler(int32 a)
{
  if(RWWrap && a>=0x8000)
   return BWriteG[a-0x8000];
  else
   return BWrite[a];
}

void FASTAPASS(3) SetWriteHandler(int32 start, int32 end, writefunc func)
{
  int32 x;

  if(!func)
   func=BNull;

  if(RWWrap)
   for(x=end;x>=start;x--)
   {
    if(x>=0x8000)
     BWriteG[x-0x8000]=func;
    else
     BWrite[x]=func;
   }
  else
   for(x=end;x>=start;x--)
    BWrite[x]=func;
}

uint8 vtoggle=0;
uint8 XOffset=0;

uint32 TempAddr,RefreshAddr;

static int maxsprites=8;

/* scanline is equal to the current visible scanline we're on. */

int scanline;
static uint32 scanlines_per_frame;

uint8 PPU[4];
uint8 PPUSPL;

uint8 GameMemBlock[131072];
uint8 NTARAM[0x800],PALRAM[0x20],SPRAM[0x100],SPRBUF[0x100];
uint8 RAM[0x800];

uint8 PAL=0;

#define MMC5SPRVRAMADR(V)      &MMC5SPRVPage[(V)>>10][(V)]
#define MMC5BGVRAMADR(V)      &MMC5BGVPage[(V)>>10][(V)]
#define	VRAMADR(V)	&VPage[(V)>>10][(V)]
 
static DECLFW(BRAML)
{  
        RAM[A]=V;
}

static DECLFW(BRAMH)
{
        RAM[A&0x7FF]=V;
}

static DECLFR(ARAML)
{
        return RAM[A];
}

static DECLFR(ARAMH)
{
        return RAM[A&0x7FF];
}

           
static DECLFR(A2002)
{
                        uint8 ret;
                        ret = PPU_status;
                        vtoggle=0;
                        PPU_status&=0x7F;
                        return ret|(PPUGenLatch&0x1F);
}

static DECLFR(A200x)
{
                        return PPUGenLatch;
}

static DECLFR(A2007)
{
                        uint8 ret;
			uint32 tmp=RefreshAddr&0x3FFF;

                        PPUGenLatch=ret=VRAMBuffer;
			if(PPU_hook) PPU_hook(tmp);
                        if(tmp<0x2000) 
			{
			 VRAMBuffer=VPage[tmp>>10][tmp];
			}
                        else 
			{
			 VRAMBuffer=vnapage[(tmp>>10)&0x3][tmp&0x3FF];
			}

                        if (INC32) RefreshAddr+=32;
                        else RefreshAddr++;
			if(PPU_hook) PPU_hook(RefreshAddr&0x3fff);
                        return ret;
}

static DECLFW(B2000)
{
                PPUGenLatch=V;
                PPU[0]=V;
		TempAddr&=0xF3FF;
		TempAddr|=(V&3)<<10;
}

static DECLFW(B2001)
{
                  PPUGenLatch=V;
 	          PPU[1]=V;
		  if(V&0xE0)
		   deemp=V>>5;
		  //printf("$%04x:$%02x, %d\n",X.PC,V,scanline);
}

static DECLFW(B2002)
{
                 PPUGenLatch=V;
}

static DECLFW(B2003)
{
                PPUGenLatch=V;
                PPU[3]=V;
		PPUSPL=V&0x7;
}

static DECLFW(B2004)
{
                PPUGenLatch=V;
                //SPRAM[PPU[3]++]=V;
		if(PPUSPL&8)
		{
		 if(PPU[3]>=8)
  		  SPRAM[PPU[3]]=V;
		}
		else
		{
		 //printf("$%02x:$%02x\n",PPUSPL,V);
		 SPRAM[PPUSPL]=V;
		 PPUSPL++;
		}
		PPU[3]++;
}

static DECLFW(B2005)
{
		uint32 tmp=TempAddr;

		PPUGenLatch=V;
		if (!vtoggle)
                {
		 tmp&=0xFFE0;
		 tmp|=V>>3;
                 XOffset=V&7;
                }
                else
                {
                 tmp&=0x8C1F;
                 tmp|=((V&~0x7)<<2);
		 tmp|=(V&7)<<12;
                }

		TempAddr=tmp;
                vtoggle^=1;
}

static DECLFW(B2006)
{
                       PPUGenLatch=V;
                       if(!vtoggle)
                       {
			TempAddr&=0x00FF;
                        TempAddr|=(V&0x3f)<<8;
                       }
                       else
                       {
			TempAddr&=0xFF00;
		        TempAddr|=V;
                        RefreshAddr=TempAddr;

			if(PPU_hook)
			 PPU_hook(RefreshAddr);
                       }
                      vtoggle^=1;
}

static DECLFW(B2007)
{  
			uint32 tmp=RefreshAddr&0x3FFF;

                        PPUGenLatch=V;
                        if(tmp>=0x3F00)
                        {
                        // hmmm....
                        if(!(tmp&0xf))
                         PALRAM[0x00]=PALRAM[0x04]=PALRAM[0x08]=PALRAM[0x0C]=
                         PALRAM[0x10]=PALRAM[0x14]=PALRAM[0x18]=PALRAM[0x1c]=V&0x3f;
                        else if(tmp&3) PALRAM[(tmp&0x1f)]=V&0x3f;
                        }
                        else if(tmp<0x2000)
                        {
                          if(PPUCHRRAM&(1<<(tmp>>10)))
                            VPage[tmp>>10][tmp]=V;
                        }
                        else
			{                         			
                         if(PPUNTARAM&(1<<((tmp&0xF00)>>10)))
                          vnapage[((tmp&0xF00)>>10)][tmp&0x3FF]=V;
                        }
                        if (INC32) RefreshAddr+=32;
                        else RefreshAddr++;
                        if(PPU_hook) PPU_hook(RefreshAddr&0x3fff);
}

static DECLFW(B4014)
{                        
	uint32 t=V<<8;
	int x;
	for(x=0;x<256;x++)
	 B2004(0x2004,X.DB=ARead[t+x](t+x));
	X6502_AddCycles(512);
}

static void FASTAPASS(1) BGRender(uint8 *target)
{
	uint32 tem;
        RefreshLine(target);
        if(!(PPU[1]&2))
        {
         tem=Pal[0]|(Pal[0]<<8)|(Pal[0]<<16)|(Pal[0]<<24);
         tem|=0x40404040;
         *(uint32 *)target=*(uint32 *)(target+4)=tem;
        }
}

#ifdef FRAMESKIP
static int FSkip=0;
void FCEUI_FrameSkip(int x)
{
 FSkip=x;
}
#endif

/*	This is called at the beginning of each visible scanline */
static void Loop6502(void)
{
	uint32 tem;
	int x;
        uint8 *target=XBuf+(scanline<<8)+(scanline<<4)+8;

        if(ScreenON || SpriteON)
        {
	 /* PRefreshLine() will not get called on skipped frames.  This
	    could cause a problem, but the solution would be rather complex,
	    due to the current sprite 0 hit code.
	 */
	 #ifdef FRAMESKIP
	 if(!FSkip)
	 {
	 #endif
	  if(ScreenON)
	  {
  	   if(scanline>=FSettings.FirstSLine && scanline<=FSettings.LastSLine)
	    BGRender(target);
	   else
	   {
	    if(PPU_hook)
 	     PRefreshLine();
   	   }
	  }
	  else
	  {
	   tem=Pal[0]|(Pal[0]<<8)|(Pal[0]<<16)|(Pal[0]<<24);
	   tem|=0x40404040;
	   FCEU_dwmemset(target,tem,264);
	  }
	 #ifdef FRAMESKIP
	 }
	 #endif
   	 if (SpriteON && scanline)
	  RefreshSprite(target);
	 #ifdef FRAMESKIP
	 if(!FSkip)
	 {
	 #endif
	  if(PPU[1]&0x01)
	  { 
	   for(x=63;x>=0;x--)
	    *(uint32 *)&target[x<<2]=(*(uint32*)&target[x<<2])&0xF0F0F0F0;
	  }
	   if((PPU[1]>>5)==0x7)
	    for(x=63;x>=0;x--)
	     *(uint32 *)&target[x<<2]=((*(uint32*)&target[x<<2])&0x3f3f3f3f)|0x40404040;
	   else	if(PPU[1]&0xE0)
	    for(x=63;x>=0;x--)
	     *(uint32 *)&target[x<<2]=(*(uint32*)&target[x<<2])|0xC0C0C0C0;
	   else
            for(x=63;x>=0;x--)
             *(uint32 *)&target[x<<2]=(*(uint32*)&target[x<<2])&0x3f3f3f3f;

	 #ifdef FRAMESKIP
	 }
	 #endif
	}
	else
	{
	 tem=Pal[0]|(Pal[0]<<8)|(Pal[0]<<16)|(Pal[0]<<24);
	 FCEU_dwmemset(target,tem,256);
	}		
        if(InputScanlineHook)
         InputScanlineHook(target, scanline);
}

#define PAL(c)  ((c)+cc)


static void PRefreshLine(void)
{
        uint32 vofs;
        uint8 X1;

        vofs = 0;
        if (BGAdrHI) vofs = 0x1000;

        vofs+=(RefreshAddr>>12)&7;

        for(X1=33;X1;X1--)
        {
                register uint8 no;
                register uint8 zz2;
                zz2=(uint8)((RefreshAddr>>10)&3);
		PPU_hook(0x2000|(RefreshAddr&0xFFF));
                no  = vnapage[zz2][(RefreshAddr&0x3ff)];
                PPU_hook((no<<4)+vofs);
                if((RefreshAddr&0x1f)==0x1f)
                 RefreshAddr^=0x41F;
                else
                 RefreshAddr++;
        }
}

/*              Total of 33 tiles(32 + 1 extra) */
static void FASTAPASS(1) RefreshLine(uint8 *target)
{
	uint32 vofs;
        int X1;
        register uint8 *P=target; 

	vofs=0;
	
        Pal[0]|=64;
        Pal[4]|=64;
        Pal[8]|=64;
        Pal[0xC]|=64;

	vofs=((PPU[0]&0x10)<<8) | ((RefreshAddr>>12)&7);
        P-=XOffset;

	/* This high-level graphics MMC5 emulation code was written
	   for MMC5 carts in "CL" mode.  It's probably not totally
	   correct for carts in "SL" mode.
	*/
        if(MMC5Hack && geniestage!=1)
        {
	 if(MMC5HackCHRMode==0 && (MMC5HackSPMode&0x80))
	 {
	  int8 tochange;

          tochange=MMC5HackSPMode&0x1F;

          for(X1=33;X1;X1--,P+=8)
          {
                uint8 *C;
                register uint8 cc,zz,zz2;
                uint32 vadr;

                if((tochange<=0 && MMC5HackSPMode&0x40) || 
		   (tochange>0 && !(MMC5HackSPMode&0x40)))
                {
                 uint8 xs,ys;

                 xs=33-X1; 
                 ys=((scanline>>3)+MMC5HackSPScroll)&0x1F;
                 if(ys>=0x1E) ys-=0x1E;
                 vadr=(MMC5HackExNTARAMPtr[xs|(ys<<5)]<<4)+(vofs&7);

                 C = MMC5HackVROMPTR+vadr;
                 C += ((MMC5HackSPPage & 0x3f & MMC5HackVROMMask) << 12);

                 cc=MMC5HackExNTARAMPtr[0x3c0+(xs>>2)+((ys&0x1C)<<1)];
                 cc=((cc >> ((xs&2) + ((ys&0x2)<<1))) &3) <<2;
                }
                else
                {
                 zz=RefreshAddr&0x1F;
                 zz2=(RefreshAddr>>10)&3;
                 vadr=(vnapage[zz2][RefreshAddr&0x3ff]<<4)+vofs;
                 C = MMC5BGVRAMADR(vadr);
                 cc=vnapage[zz2][0x3c0+(zz>>2)+((RefreshAddr&0x380)>>4)];
                 cc=((cc >> ((zz&2) + ((RefreshAddr&0x40)>>4))) &3) <<2;
                }
                #include "fceline.h"

                if((RefreshAddr&0x1f)==0x1f)
                 RefreshAddr^=0x41F;
                else
                 RefreshAddr++;
                tochange--;
          }
	 }
	 else if(MMC5HackCHRMode==1 && (MMC5HackSPMode&0x80))
	 {
          int8 tochange;

          tochange=MMC5HackSPMode&0x1F;

          for(X1=33;X1;X1--,P+=8)
          {
                uint8 *C;
                register uint8 cc;
                register uint8 zz2;
                uint32 vadr;

                if((tochange<=0 && MMC5HackSPMode&0x40) ||
                   (tochange>0 && !(MMC5HackSPMode&0x40)))
                {
                 uint8 xs,ys;

                 xs=33-X1; 
                 ys=((scanline>>3)+MMC5HackSPScroll)&0x1F;
                 if(ys>=0x1E) ys-=0x1E;
                 vadr=(MMC5HackExNTARAMPtr[xs|(ys<<5)]<<4)+(vofs&7);

                 C = MMC5HackVROMPTR+vadr;
                 C += ((MMC5HackSPPage & 0x3f & MMC5HackVROMMask) << 12);

                 cc=MMC5HackExNTARAMPtr[0x3c0+(xs>>2)+((ys&0x1C)<<1)];
                 cc=((cc >> ((xs&2) + ((ys&0x2)<<1))) &3) <<2;
                }
                else
                {
                 C=MMC5HackVROMPTR;
                 zz2=(RefreshAddr>>10)&3;
                 vadr = (vnapage[zz2][RefreshAddr & 0x3ff] << 4) + vofs;
                 C += (((MMC5HackExNTARAMPtr[RefreshAddr & 0x3ff]) & 0x3f &
                         MMC5HackVROMMask) << 12) + (vadr & 0xfff);
                 vadr = (MMC5HackExNTARAMPtr[RefreshAddr & 0x3ff] & 0xC0)>> 4;
                 cc = vadr;
		}
                #include "fceline.h"
                if((RefreshAddr&0x1f)==0x1f)
                 RefreshAddr^=0x41F;
                else
                 RefreshAddr++;
		tochange--;
          }
	 }

         else if(MMC5HackCHRMode==1)
         {
          for(X1=33;X1;X1--,P+=8)
          {
                uint8 *C;                                   
                register uint8 cc;
                register uint8 zz2;
                uint32 vadr;  

                C=MMC5HackVROMPTR;
                zz2=(RefreshAddr>>10)&3;
                vadr = (vnapage[zz2][RefreshAddr & 0x3ff] << 4) + vofs;
                C += (((MMC5HackExNTARAMPtr[RefreshAddr & 0x3ff]) & 0x3f & 
			MMC5HackVROMMask) << 12) + (vadr & 0xfff);
                vadr = (MMC5HackExNTARAMPtr[RefreshAddr & 0x3ff] & 0xC0)>> 4;
                cc = vadr;

                #include "fceline.h"
                if((RefreshAddr&0x1f)==0x1f)
                 RefreshAddr^=0x41F;
                else
                 RefreshAddr++;
          }
         }
         else
         {
          for(X1=33;X1;X1--,P+=8)
          {
                uint8 *C;
                register uint8 cc,zz,zz2;
                uint32 vadr;

                zz=RefreshAddr&0x1F;
                zz2=(RefreshAddr>>10)&3;
                vadr=(vnapage[zz2][RefreshAddr&0x3ff]<<4)+vofs;
                C = MMC5BGVRAMADR(vadr);
                cc=vnapage[zz2][0x3c0+(zz>>2)+((RefreshAddr&0x380)>>4)];
                cc=((cc >> ((zz&2) + ((RefreshAddr&0x40)>>4))) &3) <<2;

		#include "fceline.h"
                
		if((RefreshAddr&0x1f)==0x1f)
                 RefreshAddr^=0x41F;
                else
                 RefreshAddr++;
          }          
         }
        }       // End if(MMC5Hack) 

        else if(PPU_hook)
        {
         for(X1=33;X1;X1--,P+=8)
         {
                uint8 *C;                                   
                register uint8 cc,zz,zz2;
                uint32 vadr;  

                zz=RefreshAddr&0x1F;
                zz2=(RefreshAddr>>10)&3;
                PPU_hook(0x2000|(RefreshAddr&0xFFF));
                cc=vnapage[zz2][0x3c0+(zz>>2)+((RefreshAddr&0x380)>>4)];
                cc=((cc >> ((zz&2) + ((RefreshAddr&0x40)>>4))) &3) <<2;
                vadr=(vnapage[zz2][RefreshAddr&0x3ff]<<4)+vofs;
                C = VRAMADR(vadr);

	        #include "fceline.h"

	        PPU_hook(vadr);

                if((RefreshAddr&0x1f)==0x1f)
                 RefreshAddr^=0x41F;
                else
                 RefreshAddr++;
         }
        }
        else
        {      
         for(X1=33;X1;X1--,P+=8)
         {
                uint8 *C;
                register uint8 cc,zz,zz2;
                uint32 vadr;

                zz=RefreshAddr&0x1F;
		zz2=(RefreshAddr>>10)&3;
                vadr=(vnapage[zz2][RefreshAddr&0x3ff]<<4)+vofs;
                C = VRAMADR(vadr);
		cc=vnapage[zz2][0x3c0+(zz>>2)+((RefreshAddr&0x380)>>4)];
	        cc=((cc >> ((zz&2) + ((RefreshAddr&0x40)>>4))) &3) <<2;
		#include "fceline.h"
                
                if((RefreshAddr&0x1f)==0x1f)
                 RefreshAddr^=0x41F;
                else
                 RefreshAddr++;
         }
        }

        #undef vofs

        Pal[0]&=63;
        Pal[4]&=63;
        Pal[8]&=63;
        Pal[0xC]&=63;
}

static INLINE void Fixit2(void)
{
   if(ScreenON || SpriteON)
   {
    uint32 rad=RefreshAddr;
    rad&=0xFBE0;
    rad|=TempAddr&0x041f;
    RefreshAddr=rad;
    //PPU_hook(RefreshAddr,-1);
   }
}

static INLINE void Fixit1(void)
{
   if(ScreenON || SpriteON)
   {
    uint32 rad=RefreshAddr;

    if((rad&0x7000)==0x7000)
    {
     rad^=0x7000;
     if((rad&0x3E0)==0x3A0)
     {
      rad^=0x3A0;
      rad^=0x800;
     }
     else
     {
      if((rad&0x3E0)==0x3e0)
       rad^=0x3e0;
      else rad+=0x20;
     }
    }
    else
     rad+=0x1000;
    RefreshAddr=rad;
    //PPU_hook(RefreshAddr,-1);
   }
}

/*      This is called at the beginning of all h-blanks on visible lines. */
static void DoHBlank(void)
{
 if(ScreenON || SpriteON)
  FetchSpriteData();
 if(GameHBIRQHook && (ScreenON || SpriteON))
 {
  X6502_Run(12);
  GameHBIRQHook();
  X6502_Run(25-12);
  Fixit2();
  X6502_Run(85-25);
 }
 else
 {
  X6502_Run(25);	// Tried 65, caused problems with Slalom(maybe others)
  Fixit2();
  X6502_Run(85-25);
 }
 //PPU_hook(0,-1);
 //fprintf(stderr,"%3d: $%04x\n",scanline,RefreshAddr);
}

#define	V_FLIP	0x80
#define	H_FLIP	0x40
#define	SP_BACK	0x20

typedef struct {
        uint8 y,no,atr,x;
} SPR;

typedef struct {
	uint8 ca[2],atr,x;
} SPRB;

uint8 sprlinebuf[256+8];        

void FCEUI_DisableSpriteLimitation(int a)
{
 maxsprites=a?64:8;
}

static uint8 nosprites,SpriteBlurp;

static void FetchSpriteData(void)
{
	SPR *spr;
	uint8 H;
	int n,vofs;

	spr=(SPR *)SPRAM;
	H=8;

	nosprites=SpriteBlurp=0;

        vofs=(unsigned int)(PPU[0]&0x8&(((PPU[0]&0x20)^0x20)>>2))<<9;
	H+=(PPU[0]&0x20)>>2;

        if(!PPU_hook)
         for(n=63;n>=0;n--,spr++)
         {
                if((unsigned int)(scanline-spr->y)>=H) continue;

                if(nosprites<maxsprites)
                {
                 if(n==63) SpriteBlurp=1;

		 {
		  SPRB dst;
		  uint8 *C;
                  int t;
                  unsigned int vadr;

                  t = (int)scanline-(spr->y);

                  if (Sprite16)
                   vadr = ((spr->no&1)<<12) + ((spr->no&0xFE)<<4);
                  else
                   vadr = (spr->no<<4)+vofs;

                  if (spr->atr&V_FLIP)
                  {
                        vadr+=7;
                        vadr-=t;
                        vadr+=(PPU[0]&0x20)>>1;
                        vadr-=t&8;
                  }
                  else
                  {
                        vadr+=t;
                        vadr+=t&8;
                  }

		  /* Fix this geniestage hack */
      	          if(MMC5Hack && geniestage!=1) C = MMC5SPRVRAMADR(vadr);
                  else C = VRAMADR(vadr);

		  
		  dst.ca[0]=C[0];
		  dst.ca[1]=C[8];
		  dst.x=spr->x;
		  dst.atr=spr->atr;


		  *(uint32 *)&SPRBUF[nosprites<<2]=*(uint32 *)&dst;
		 }

                 nosprites++;
                }
                else
                {
                  PPU_status|=0x20;
                  break;
                }
         }
	else
         for(n=63;n>=0;n--,spr++)
         {
                if((unsigned int)(scanline-spr->y)>=H) continue;

                if(nosprites<maxsprites)
                {
                 if(n==63) SpriteBlurp=1;

                 {
                  SPRB dst;
                  uint8 *C;
                  int t;
                  unsigned int vadr;

                  t = (int)scanline-(spr->y);

                  if (Sprite16)
                   vadr = ((spr->no&1)<<12) + ((spr->no&0xFE)<<4);
                  else
                   vadr = (spr->no<<4)+vofs;

                  if (spr->atr&V_FLIP)
                  {
                        vadr+=7;
                        vadr-=t;
                        vadr+=(PPU[0]&0x20)>>1;
                        vadr-=t&8;
                  }
                  else
                  {
                        vadr+=t;
                        vadr+=t&8;
                  }

                  if(MMC5Hack) C = MMC5SPRVRAMADR(vadr);
                  else C = VRAMADR(vadr);
                  dst.ca[0]=C[0];
		  PPU_hook(vadr);
                  dst.ca[1]=C[8];
		  PPU_hook(vadr|8);
                  dst.x=spr->x;
                  dst.atr=spr->atr;


                  *(uint32 *)&SPRBUF[nosprites<<2]=*(uint32 *)&dst;
                 }

                 nosprites++;
                }
                else
                {
                  PPU_status|=0x20;
                  break;
                }
         }
}

static void FASTAPASS(1) RefreshSprite(uint8 *target)
{
	int n;
        SPRB *spr;
        uint8 *P=target;

        if(!nosprites) return;
	#ifdef FRAMESKIP
	if(FSkip)
	{
	 if(!SpriteBlurp)
	 {
	  nosprites=0;
	  return;
	 }
	 else
	  nosprites=1;
	}
	#endif

        FCEU_dwmemset(sprlinebuf,0x80808080,256);
        nosprites--;
        spr = (SPRB*)SPRBUF+nosprites;

       for(n=nosprites;n>=0;n--,spr--)
       {
        register uint8 J,atr,c1,c2;
	int x=spr->x;
        uint8 *C;
        uint8 *VB;
                
        P+=x;

        c1=((spr->ca[0]>>1)&0x55)|(spr->ca[1]&0xAA);
	c2=(spr->ca[0]&0x55)|((spr->ca[1]<<1)&0xAA);

        J=spr->ca[0]|spr->ca[1];
	atr=spr->atr;

                       if(J)
                       {        
                        if(n==0 && SpriteBlurp && !(PPU_status&0x40))
                        {  
			 int z,ze=x+8;
			 if(ze>256) {ze=256;}
			 if(ScreenON && (scanline<FSettings.FirstSLine || scanline>FSettings.LastSLine
			 #ifdef FRAMESKIP
			 || FSkip
			 #endif
			 ))
			  BGRender(target);

			 if(!(atr&H_FLIP))
			 {
			  for(z=x;z<ze;z++)
			  {
			   if(J&(0x80>>(z-x)))
			   {
			    if(!(target[z]&64))
			     tosprite=z;
			   }
			  }
			 }
			 else
			 {
                          for(z=x;z<ze;z++)
                          {
                           if(J&(1<<(z-x)))
                           {
                            if(!(target[z]&64))
                             tosprite=z;
                           }
                          }
			 }
			 //FCEU_DispMessage("%d, %d:%d",scanline,x,tosprite);
                        }

	 C = sprlinebuf+x;
         VB = (PALRAM+0x10)+((atr&3)<<2);

         if(atr&SP_BACK) 
         {
          if (atr&H_FLIP)
          {
           if (J&0x02)  C[1]=VB[c1&3]|0x40;
           if (J&0x01)  *C=VB[c2&3]|0x40;
           c1>>=2;c2>>=2;
           if (J&0x08)  C[3]=VB[c1&3]|0x40;;
           if (J&0x04)  C[2]=VB[c2&3]|0x40;;
           c1>>=2;c2>>=2;
           if (J&0x20)  C[5]=VB[c1&3]|0x40;;
           if (J&0x10)  C[4]=VB[c2&3]|0x40;;
           c1>>=2;c2>>=2;
           if (J&0x80)  C[7]=VB[c1]|0x40;;
           if (J&0x40)  C[6]=VB[c2]|0x40;;
	  } else  {
           if (J&0x02)  C[6]=VB[c1&3]|0x40;
           if (J&0x01)  C[7]=VB[c2&3]|0x40;
	   c1>>=2;c2>>=2;
           if (J&0x08)  C[4]=VB[c1&3]|0x40;
           if (J&0x04)  C[5]=VB[c2&3]|0x40;
           c1>>=2;c2>>=2;
           if (J&0x20)  C[2]=VB[c1&3]|0x40;
           if (J&0x10)  C[3]=VB[c2&3]|0x40;
           c1>>=2;c2>>=2;
           if (J&0x80)  *C=VB[c1]|0x40;
           if (J&0x40)  C[1]=VB[c2]|0x40;
	  }
         } else {
          if (atr&H_FLIP)
	  {
           if (J&0x02)  C[1]=VB[(c1&3)];
           if (J&0x01)  *C=VB[(c2&3)];
           c1>>=2;c2>>=2;
           if (J&0x08)  C[3]=VB[(c1&3)];
           if (J&0x04)  C[2]=VB[(c2&3)];
           c1>>=2;c2>>=2;
           if (J&0x20)  C[5]=VB[(c1&3)];
           if (J&0x10)  C[4]=VB[(c2&3)];
           c1>>=2;c2>>=2;
           if (J&0x80)  C[7]=VB[c1];
           if (J&0x40)  C[6]=VB[c2];
          }else{                 
           if (J&0x02)  C[6]=VB[(c1&3)];
           if (J&0x01)  C[7]=VB[(c2&3)];
           c1>>=2;c2>>=2;
           if (J&0x08)  C[4]=VB[(c1&3)];
           if (J&0x04)  C[5]=VB[(c2&3)];
           c1>>=2;c2>>=2;
           if (J&0x20)  C[2]=VB[(c1&3)];
           if (J&0x10)  C[3]=VB[(c2&3)];
           c1>>=2;c2>>=2;
           if (J&0x80)  *C=VB[c1];
           if (J&0x40)  C[1]=VB[c2];
          }
         }
        }
       P-=x;
      }

     nosprites=0;
     #ifdef FRAMESKIP
     if(FSkip) return;
     #endif

     {
      uint8 n=((PPU[1]&4)^4)<<1;
      loopskie:
      {
       uint32 t=*(uint32 *)(sprlinebuf+n);
       if(t!=0x80808080)
       {
	#ifdef LSB_FIRST
        if(!(t&0x80))
        {
         if(!(t&0x40))       // Normal sprite
          P[n]=sprlinebuf[n];
         else if(P[n]&64)        // behind bg sprite
          P[n]=sprlinebuf[n];
        }

        if(!(t&0x8000))
        {
         if(!(t&0x4000))       // Normal sprite
          P[n+1]=(sprlinebuf+1)[n];
         else if(P[n+1]&64)        // behind bg sprite
          P[n+1]=(sprlinebuf+1)[n];
        }

        if(!(t&0x800000))
        {
         if(!(t&0x400000))       // Normal sprite
          P[n+2]=(sprlinebuf+2)[n];
         else if(P[n+2]&64)        // behind bg sprite
          P[n+2]=(sprlinebuf+2)[n];
        }

        if(!(t&0x80000000))
        {
         if(!(t&0x40000000))       // Normal sprite
          P[n+3]=(sprlinebuf+3)[n];
         else if(P[n+3]&64)        // behind bg sprite
          P[n+3]=(sprlinebuf+3)[n];
        }
	#else
        if(!(t&0x80000000))
        {
         if(!(t&0x40))       // Normal sprite
          P[n]=sprlinebuf[n];
         else if(P[n]&64)        // behind bg sprite
          P[n]=sprlinebuf[n];
        }

        if(!(t&0x800000))
        {
         if(!(t&0x4000))       // Normal sprite
          P[n+1]=(sprlinebuf+1)[n];
         else if(P[n+1]&64)        // behind bg sprite
          P[n+1]=(sprlinebuf+1)[n];
        }

        if(!(t&0x8000))
        {
         if(!(t&0x400000))       // Normal sprite
          P[n+2]=(sprlinebuf+2)[n];
         else if(P[n+2]&64)        // behind bg sprite
          P[n+2]=(sprlinebuf+2)[n];
        }

        if(!(t&0x80))
        {
         if(!(t&0x40000000))       // Normal sprite
          P[n+3]=(sprlinebuf+3)[n];
         else if(P[n+3]&64)        // behind bg sprite
          P[n+3]=(sprlinebuf+3)[n];
        }
	#endif
       }
      }
      n+=4;
      if(n) goto loopskie;
     }
}

void ResetMapping(void)
{
	int x;

        SetReadHandler(0x0000,0xFFFF,ANull);
	SetWriteHandler(0x0000,0xFFFF,BNull);

        SetReadHandler(0,0x7FF,ARAML);
        SetWriteHandler(0,0x7FF,BRAML);

        SetReadHandler(0x800,0x1FFF,ARAMH);  /* Part of a little */
        SetWriteHandler(0x800,0x1FFF,BRAMH); /* hack for a small speed boost. */

        for(x=0x2000;x<0x4000;x+=8)
        {
         ARead[x]=A200x;
         BWrite[x]=B2000;
         ARead[x+1]=A200x;
         BWrite[x+1]=B2001;
         ARead[x+2]=A2002;
         BWrite[x+2]=B2002;
         ARead[x+3]=A200x;
         BWrite[x+3]=B2003;
         ARead[x+4]=A200x;
         BWrite[x+4]=B2004;
         ARead[x+5]=A200x;
         BWrite[x+5]=B2005;
         ARead[x+6]=A200x;
         BWrite[x+6]=B2006;
         ARead[x+7]=A2007;
         BWrite[x+7]=B2007;
        }

        BWrite[0x4014]=B4014;
        SetNESSoundMap();
	InitializeInput();
}

int GameLoaded=0;
void CloseGame(void)
{
 if(GameLoaded)
 {
  if(FCEUGameInfo.type!=GIT_NSF)
   FlushGameCheats();
  #ifdef NETWORK
  if(FSettings.NetworkPlay) KillNetplay();
  #endif       
  GameInterface(GI_CLOSE);
  CloseGenie();
  GameLoaded=0;
 }
}

void ResetGameLoaded(void)
{
        if(GameLoaded) CloseGame();
        GameStateRestore=0;
        PPU_hook=0;
        GameHBIRQHook=0;
	GameExpSound.Fill=0;
	GameExpSound.RChange=0;
        if(GameExpSound.Kill)
         GameExpSound.Kill();
        GameExpSound.Kill=0;
        MapIRQHook=0;
        MMC5Hack=0;
        PAL&=1;
	pale=0;

	FCEUGameInfo.name=0;
	FCEUGameInfo.type=GIT_CART;
	FCEUGameInfo.vidsys=GIV_USER;
	FCEUGameInfo.input[0]=FCEUGameInfo.input[1]=-1;
	FCEUGameInfo.inputfc=-1;
}

FCEUGI *FCEUI_LoadGame(char *name)
{
        int fp;

        Exit=1;
        ResetGameLoaded();

	fp=FCEU_fopen(name,"rb");
	if(!fp)
        {
 	 FCEU_PrintError("Error opening \"%s\"!",name);
	 return 0;
	}

        GetFileBase(name);
        if(iNESLoad(name,fp))
         goto endlseq;
        if(NSFLoad(fp))
         goto endlseq;
        if(FDSLoad(name,fp))
         goto endlseq;
        if(UNIFLoad(name,fp))
         goto endlseq;

        FCEU_PrintError("An error occurred while loading the file.");
        FCEU_fclose(fp);
        return 0;

        endlseq:
        FCEU_fclose(fp);
        GameLoaded=1;        

        FCEU_ResetVidSys();
        if(FCEUGameInfo.type!=GIT_NSF)
         if(FSettings.GameGenie)
	  OpenGenie();

        PowerNES();
	#ifdef NETWORK
        if(FSettings.NetworkPlay) InitNetplay();
	#endif
        SaveStateRefresh();
        if(FCEUGameInfo.type!=GIT_NSF)
        {
         LoadGamePalette();
         LoadGameCheats();
        }
        
	FCEU_ResetPalette();
        Exit=0;
        return(&FCEUGameInfo);
}


void FCEU_ResetVidSys(void)
{
 int w;

 if(FCEUGameInfo.vidsys==GIV_NTSC)
  w=0;
 else if(FCEUGameInfo.vidsys==GIV_PAL)
  w=1;
 else
  w=FSettings.PAL;

 if(w)
 {
  PAL=1;
  scanlines_per_frame=312;
  FSettings.FirstSLine=FSettings.UsrFirstSLine[1];
  FSettings.LastSLine=FSettings.UsrLastSLine[1];
 }
 else
 {
  PAL=0;
  scanlines_per_frame=262;
  FSettings.FirstSLine=FSettings.UsrFirstSLine[0];
  FSettings.LastSLine=FSettings.UsrLastSLine[0];
 }
 SetSoundVariables();
}

int FCEUI_Initialize(void)
{
        if(!InitVirtualVideo())
         return 0;
	memset(&FSettings,0,sizeof(FSettings));
	FSettings.UsrFirstSLine[0]=8;
	FSettings.UsrFirstSLine[1]=0;
        FSettings.UsrLastSLine[0]=FSettings.UsrLastSLine[1]=239;
	FSettings.SoundVolume=65536;	// 100%
        return 1;
}

#define harko 0xe //0x9
static INLINE void Thingo(void)
{
   Loop6502();

   if(tosprite>=256)
   { 
    X6502_Run(256-harko);
    Fixit1();
    X6502_Run(harko);
   }
   else
   {
    if(tosprite<=240)
    {
     X6502_Run(tosprite);
     PPU[2]|=0x40;
     X6502_Run(256-tosprite-harko);
     Fixit1();
     X6502_Run(harko);
    }
    else
    {
     X6502_Run(256-harko);
     Fixit1();
     X6502_Run(tosprite-(256-harko));
     PPU[2]|=0x40;
     X6502_Run(256-tosprite);
    }
    tosprite=256;
   }
   DoHBlank();
}
#undef harko

void EmLoop(void)
{
 for(;;)
 {
  ApplyPeriodicCheats();
  X6502_Run(256+85);

  PPU[2]|=0x80;
  PPU[3]=PPUSPL=0;	       /* Not sure if this is correct.  According
				  to Matt Conte and my own tests, it is.  Timing is probably
			 	  off, though.  NOTE:  Not having this here
				  breaks a Super Donkey Kong game. */

  X6502_Run(12);		/* I need to figure out the true nature and length
				   of this delay. 
			 	*/
  if(FCEUGameInfo.type==GIT_NSF)
   TriggerNMINSF();
  else if(VBlankON)
   TriggerNMI();

  X6502_Run((scanlines_per_frame-242)*(256+85)-12); 

  PPU_status&=0x1f;

  X6502_Run(256);
  {
   static int kook=0;
   if(ScreenON || SpriteON)
    if(GameHBIRQHook)
     GameHBIRQHook();

   X6502_Run(85-kook);
   kook=(kook+1)&1;
  }

  if(ScreenON || SpriteON)
  {
   RefreshAddr=TempAddr;
   if(PPU_hook) PPU_hook(RefreshAddr&0x3fff);
  }
  if(FCEUGameInfo.type==GIT_NSF)
   X6502_Run((256+85)*240);
  else
  {
   int x,max,maxref;

   deemp=PPU[1]>>5;
   for(scanline=0;scanline<240;scanline++)
   {
    deempcnt[deemp]++;
    Thingo();
   }
   for(x=1,max=0,maxref=0;x<7;x++)
   {
    if(deempcnt[x]>max)
    {
     max=deempcnt[x];
     maxref=x;
    }
    deempcnt[x]=0;
   }
   //FCEU_DispMessage("%2x:%2x:%2x:%2x:%2x:%2x:%2x:%2x %d",deempcnt[0],deempcnt[1],deempcnt[2],deempcnt[3],deempcnt[4],deempcnt[5],deempcnt[6],deempcnt[7],maxref);
   //memset(deempcnt,0,sizeof(deempcnt));
   SetNESDeemph(maxref,0);
  }

  {
   int ssize;

   ssize=FlushEmulateSound();

   #ifdef FRAMESKIP
   if(FSkip)
   {
    FCEU_PutImageDummy();
    FSkip--;
    FCEUD_Update(0,WaveFinal,ssize);
   }
   else
   #endif
   {
    FCEU_PutImage();
    FCEUD_Update(XBuf+8,WaveFinal,ssize);
   }
   UpdateInput();
  }

  if(Exit)
  {
   CloseGame();
   break;
  }

 }
}

#ifdef FPS
#include <sys/time.h>
uint64 frcount;
#endif
void FCEUI_Emulate(void)
{
	#ifdef FPS
        uint64 starttime,end;
        struct timeval tv;
	frcount=0;
        gettimeofday(&tv,0);
        starttime=((uint64)tv.tv_sec*1000000)+tv.tv_usec;
	#endif
	EmLoop();

        #ifdef FPS
        // Probably won't work well on Windows port; for
	// debugging/speed testing.
	{
	 uint64 w;
	 int i,frac;
         gettimeofday(&tv,0);
         end=((uint64)tv.tv_sec*1000000)+tv.tv_usec;
	 w=frcount*10000000000LL/(end-starttime);
	 i=w/10000;
	 frac=w-i*10000;
         printf("Average FPS: %d.%04d\n",i,frac);
	}
        #endif

}

void FCEUI_CloseGame(void)
{
        Exit=1;
}

static void ResetPPU(void)
{
        VRAMBuffer=PPU[0]=PPU[1]=PPU[2]=PPU[3]=0;
        PPUSPL=0;
	PPUGenLatch=0;
        RefreshAddr=TempAddr=0;
        vtoggle = 0;
}

static void PowerPPU(void)
{
        memset(NTARAM,0x00,0x800);
        memset(PALRAM,0x00,0x20);
        memset(SPRAM,0x00,0x100);
	ResetPPU();
}

void ResetNES(void)
{
        if(!GameLoaded || (FCEUGameInfo.type==GIT_NSF)) return;
        GameInterface(GI_RESETM2);
        ResetSound();
        ResetPPU();
        X6502_Reset();
}

void PowerNES(void) 
{
        if(!GameLoaded) return;

	FCEU_CheatResetRAM();
	FCEU_CheatAddRAM(2,0,RAM);

        GeniePower();

        memset(RAM,0x00,0x800);
        ResetMapping();
	GameInterface(GI_POWER);
        PowerSound();
	PowerPPU();
	timestampbase=0;
	X6502_Power();
}

