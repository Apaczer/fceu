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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "types.h"
#include "x6502.h"
#include "fce.h"
#include "svga.h"
#include "video.h"
#include "sound.h"
#include "ines.h"
#include "nsf.h"
#include "nsfbgnew.h"
#include "general.h"
#include "memory.h"
#include "file.h"
#include "fds.h"
#include "cart.h"
#include "input.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

uint8 SongReload;
uint8 CurrentSong;

static int sinetable[32];


static uint8 NSFROM[0x30+6]=
{
/* 0x00 */

0x08,0x48,0x8A,0x48,0x98,0x48,          /* Store regs           */
0xA9,0xFF,
0x8D,0xF2,0x5F,				/* NMI has occured	*/
0x68,0xA8,0x68,0xAA,0x68,0x28,
0x40,     /* Restore regs         */

/* 0x12 */

0xAD,0xF2,0x5F,				/* See if an NMI occured */
0xF0,0xFB,				/* If it hasn't, loop    */

0xA9,0x00,
0x8D,0xF2,0x5F,				/* Clear play pending reg*/


0xAD,0xF0,0x5F,                         /* See if we need to init. */
0xF0,0x09,                              /* If 0, go to JMP         */

0xAD,0xF1,0x5F,                         /* Confirm and load A      */
0xAE,0xF3,0x5F,                         /* Load X with PAL/NTSC byte */

0x20,0x00,0x00,                         /* JSR to init routine     */

0x20,0x00,0x00,                         /* JSR to play routine  */

0x4C,0x12,0x38,                         /* Loop                 */

0xA2,0xFF,0x9A,				/* Initialize the stack pointer. */
0x4C,0x12,0x38
};

static DECLFR(NSFROMRead)
{
 return (NSFROM-0x3800)[A];
}



static uint8 *NSFDATA=0;
static int NSFMaxBank;

static int NSFSize;
static uint8 BSon;
static uint16 PlayAddr;
static uint16 InitAddr;
static uint16 LoadAddr;

NSF_HEADER NSFHeader;

void NSFGI(int h)
{
 switch(h)
 {
 case GI_CLOSE:
  if(NSFDATA) {free(NSFDATA);NSFDATA=0;}
  break;
 case GI_POWER: NSF_init();break;
 }
}

// First 32KB is reserved for sound chip emulation in the iNES mapper code.

#define WRAM (GameMemBlock+32768)
#define FDSMEM (GameMemBlock+32768)

static INLINE void BANKSET(uint32 A, uint32 bank)
{
 bank&=NSFMaxBank;
 if(NSFHeader.SoundChip&4)
  memcpy(FDSMEM+(A-0x6000),NSFDATA+(bank<<12),4096);
 else 
  setprg4(A,bank);
}

int NSFLoad(int fp)
{
  int x;

  FCEU_fseek(fp,0,SEEK_SET);
  FCEU_fread(&NSFHeader,1,0x80,fp);
  if (memcmp(NSFHeader.ID,"NESM\x1a",5))
                  return 0;
  NSFHeader.SongName[31]=NSFHeader.Artist[31]=NSFHeader.Copyright[31]=0;

  LoadAddr=NSFHeader.LoadAddressLow;
  LoadAddr|=NSFHeader.LoadAddressHigh<<8;

  InitAddr=NSFHeader.InitAddressLow;
  InitAddr|=NSFHeader.InitAddressHigh<<8;

  PlayAddr=NSFHeader.PlayAddressLow;
  PlayAddr|=NSFHeader.PlayAddressHigh<<8;

  NSFSize=FCEU_fgetsize(fp)-0x80;

  NSFMaxBank=((NSFSize+(LoadAddr&0xfff)+4095)/4096);
  NSFMaxBank=uppow2(NSFMaxBank);

  if(!(NSFDATA=(uint8 *)FCEU_malloc(NSFMaxBank*4096)))
   return 0;

  FCEU_fseek(fp,0x80,SEEK_SET);
  memset(NSFDATA,0x00,NSFMaxBank*4096);
  FCEU_fread(NSFDATA+(LoadAddr&0xfff),1,NSFSize,fp);
 
  NSFMaxBank--;

  BSon=0;
  for(x=0;x<8;x++)
   BSon|=NSFHeader.BankSwitch[x];

 FCEUGameInfo.type=GIT_NSF;
 FCEUGameInfo.input[0]=FCEUGameInfo.input[1]=SI_NONE;

 for(x=0;;x++)
 {
  if(NSFROM[x]==0x20)
  {
   NSFROM[x+1]=InitAddr&0xFF;
   NSFROM[x+2]=InitAddr>>8;
   NSFROM[x+4]=PlayAddr&0xFF;
   NSFROM[x+5]=PlayAddr>>8;
   break;
  }
 }

 if(NSFHeader.VideoSystem==0)
  FCEUGameInfo.vidsys=GIV_NTSC;
 else if(NSFHeader.VideoSystem==1)
  FCEUGameInfo.vidsys=GIV_PAL;

 {
  double fruit=0;
  for(x=0;x<32;x++)
  {
   double ta,no;
 
   ta=sin(fruit)*7;
   ta+=modf(ta,&no);
   sinetable[x]=ta;
   fruit+=(double)M_PI*2/32;
  }
 }
 GameInterface=NSFGI;

 puts("NSF Loaded.  File information:\n");
 printf(" Name:       %s\n Artist:     %s\n Copyright:  %s\n\n",NSFHeader.SongName,NSFHeader.Artist,NSFHeader.Copyright);
 if(NSFHeader.SoundChip)
 {
  static char *tab[6]={"Konami VRCVI","Konami VRCVII","Nintendo FDS","Nintendo MMC5","Namco 106","Sunsoft FME-07"};
  for(x=0;x<6;x++)
   if(NSFHeader.SoundChip&(1<<x))
   {
    printf(" Expansion hardware:  %s\n",tab[x]);
    break;
   }
 }
 if(BSon)
  puts(" Bank-switched.");
 printf(" Load address:  $%04x\n Init address:  $%04x\n Play address:  $%04x\n",LoadAddr,InitAddr,PlayAddr);
 printf(" %s\n",(NSFHeader.VideoSystem&1)?"PAL":"NTSC");
 printf(" Starting song:  %d / %d\n\n",NSFHeader.StartingSong,NSFHeader.TotalSongs);
 return 1;
}

static DECLFW(BWRAM)
{
                (WRAM-0x6000)[A]=V;
}

static DECLFW(NSFFDSWrite)
{
	(FDSMEM-0x6000)[A]=V;
}

static DECLFR(NSFFDSRead)
{
	return (FDSMEM-0x6000)[A];
}

static DECLFR(AWRAM)
{
	return WRAM[A-0x6000];
}

void NSF_init(void)
{
  if(NSFHeader.SoundChip&4)
  {
   memset(FDSMEM,0x00,32768+8192);
   SetWriteHandler(0x6000,0xDFFF,NSFFDSWrite);
   SetReadHandler(0x6000,0xFFFF,NSFFDSRead);
  }
  else
  {
   memset(WRAM,0x00,8192);
   SetReadHandler(0x6000,0x7FFF,AWRAM);
   SetWriteHandler(0x6000,0x7FFF,BWRAM);
   ResetCartMapping();
   SetupCartPRGMapping(0,NSFDATA,((NSFMaxBank+1)*4096),0);
   SetReadHandler(0x8000,0xFFFF,CartBR);
  }

  if(BSon)
  {
   int32 x;
   for(x=0;x<8;x++)
   {
    if(NSFHeader.SoundChip&4 && x>=6)
     BANKSET(0x6000+(x-6)*4096,NSFHeader.BankSwitch[x]);
    BANKSET(0x8000+x*4096,NSFHeader.BankSwitch[x]);
   }
  }
  else
  {
   int32 x;
    for(x=(LoadAddr&0x7000);x<0x8000;x+=0x1000)
     BANKSET(0x8000+x,((x-(LoadAddr&0x7000))>>12));
  }

  SetWriteHandler(0x2000,0x3fff,0);
  SetReadHandler(0x2000,0x37ff,0);
  SetReadHandler(0x3836,0x3FFF,0);
  SetReadHandler(0x3800,0x3835,NSFROMRead);

  SetWriteHandler(0x4020,0x5fff,NSF_write);
  SetReadHandler(0x4020,0x5fff,NSF_read);


  if(NSFHeader.SoundChip&1) { 
   VRC6_ESI(0);
  } else if (NSFHeader.SoundChip&2) {
   VRC7_ESI();
  } else if (NSFHeader.SoundChip&4) {
   FDSSoundReset();
  } else if (NSFHeader.SoundChip&8) {
   Mapper5_ESI();
  } else if (NSFHeader.SoundChip&0x10) {
   Mapper19_ESI();
  } else if (NSFHeader.SoundChip&0x20) {
   Mapper69_ESI();
  }
  CurrentSong=NSFHeader.StartingSong;
  SongReload=1;
}

static uint8 DoUpdateStuff=0;
DECLFW(NSF_write)
{
switch(A)
{
 case 0x5FF2:if((X.PC&0xF000)==0x3000) DoUpdateStuff=V;break;
 
 case 0x5FF6:
 case 0x5FF7:if(!(NSFHeader.SoundChip&4)) return;
 case 0x5FF8:
 case 0x5FF9:
 case 0x5FFA:
 case 0x5FFB:
 case 0x5FFC:
 case 0x5FFD:
 case 0x5FFE:
 case 0x5FFF:if(!BSon) return;
             A&=0xF;
             BANKSET((A*4096),V);
	     break;
}
}

DECLFR(NSF_read)
{
 int x;

 if((X.PC&0xF000)==0x3000)
 switch(A)
 {
 case 0x5ff0:x=SongReload;SongReload=0;return x;
 case 0x5ff1:
	     {
             memset(RAM,0x00,0x800);
             memset(WRAM,0x00,8192);
             BWrite[0x4015](0x4015,0xF);
             for(x=0;x<0x14;x++)
              {if(x!=0x11) BWrite[0x4015](0x4015,0);}
             BWrite[0x4015](0x4015,0x0);
             for(x=0;x<0x14;x++)
              {if(x!=0x11) BWrite[0x4015](0x4015,0);}
	     BWrite[0x4011](0x4011,0x40);
             BWrite[0x4015](0x4015,0xF);
	     BWrite[0x4017](0x4017,0x40);
	     if(NSFHeader.SoundChip&4) 
	      BWrite[0x4089](0x4089,0x80);
             if(BSon)
             {
              for(x=0;x<8;x++)
	       BANKSET(0x8000+x*4096,NSFHeader.BankSwitch[x]);
             }
             return (CurrentSong-1);
 	     }
 case 0x5FF2:return DoUpdateStuff;
 case 0x5FF3:return PAL;
 }
 return 0;
}
static int32 *Bufpl;
void DrawNSF(uint8 *XBuf)
{
 char snbuf[16];
 static int z=0;
 int x,y;
 uint8 *XBuf2,*tmpb;

 XBuf+=8;
 XBuf2=XBuf;

 tmpb=NSFBG+8;
 for(y=120;y;y--)
  {
   uint8 *offs;

   offs=tmpb+sinetable[((z+y)>>2)&31];
   memcpy(XBuf2,offs,256);
   memcpy(XBuf2+(120*272),offs,256);

   XBuf2+=272;
   tmpb+=272;
  }
 tmpb=NSFBG+8;
 z=(z+1)&127;

 DrawTextTrans(XBuf+10*272+4+(((31-strlen(NSFHeader.SongName))<<2)), 272, NSFHeader.SongName, 38);
 DrawTextTrans(XBuf+30*272+4+(((31-strlen(NSFHeader.Artist))<<2)), 272, NSFHeader.Artist, 38);
 DrawTextTrans(XBuf+50*272+4+(((31-strlen(NSFHeader.Copyright))<<2)), 272, NSFHeader.Copyright, 38);

 DrawTextTrans(XBuf+90*272+4+(((31-strlen("Song:"))<<2)), 272, "Song:", 38);
 sprintf(snbuf,"<%d/%d>",CurrentSong,NSFHeader.TotalSongs);
 DrawTextTrans(XBuf+102*272+4+(((31-strlen(snbuf))<<2)), 272, snbuf, 38);

 GetSoundBuffer(&Bufpl);
  for(x=0;x<256;x++)
   XBuf[x+(224-((((Bufpl[x]>>(7)^128)&255)*3)>>3))*272]=38;
}

void NSFControl(int z)
{ 
 if(z==1)
 {
  if(CurrentSong<NSFHeader.TotalSongs) CurrentSong++;
 }
 else if(z==2)
 {
  if(CurrentSong>1) CurrentSong--;
 }
 SongReload=0xFF;
}
