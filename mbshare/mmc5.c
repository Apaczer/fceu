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

/* None of this code should use any of the iNES bank switching wrappers. */

#include "mapinc.h"

void MMC5Sound(int Count);
void Do5SQ(int P);

static INLINE void MMC5SPRVROM_BANK1(uint32 A,uint32 V) 
{
 if(CHRptr[0])
 {
  V&=CHRmask1[0];
  MMC5SPRVPage[(A)>>10]=&CHRptr[0][(V)<<10]-(A);
 }
}

static INLINE void MMC5BGVROM_BANK1(uint32 A,uint32 V) {if(CHRptr[0]){V&=CHRmask1[0];MMC5BGVPage[(A)>>10]=&CHRptr[0][(V)<<10]-(A);}}

static INLINE void MMC5SPRVROM_BANK2(uint32 A,uint32 V) {if(CHRptr[0]){V&=CHRmask2[0];MMC5SPRVPage[(A)>>10]=MMC5SPRVPage[((A)>>10)+1]=&CHRptr[0][(V)<<11]-(A);}}
static INLINE void MMC5BGVROM_BANK2(uint32 A,uint32 V) {if(CHRptr[0]){V&=CHRmask2[0];MMC5BGVPage[(A)>>10]=MMC5BGVPage[((A)>>10)+1]=&CHRptr[0][(V)<<11]-(A);}}

static INLINE void MMC5SPRVROM_BANK4(uint32 A,uint32 V) {if(CHRptr[0]){V&=CHRmask4[0];MMC5SPRVPage[(A)>>10]=MMC5SPRVPage[((A)>>10)+1]= MMC5SPRVPage[((A)>>10)+2]=MMC5SPRVPage[((A)>>10)+3]=&CHRptr[0][(V)<<12]-(A);}}
static INLINE void MMC5BGVROM_BANK4(uint32 A,uint32 V) {if(CHRptr[0]){V&=CHRmask4[0];MMC5BGVPage[(A)>>10]=MMC5BGVPage[((A)>>10)+1]=MMC5BGVPage[((A)>>10)+2]=MMC5BGVPage[((A)>>10)+3]=&CHRptr[0][(V)<<12]-(A);}}

static INLINE void MMC5SPRVROM_BANK8(uint32 V) {if(CHRptr[0]){V&=CHRmask8[0];MMC5SPRVPage[0]=MMC5SPRVPage[1]=MMC5SPRVPage[2]=MMC5SPRVPage[3]=MMC5SPRVPage[4]=MMC5SPRVPage[5]=MMC5SPRVPage[6]=MMC5SPRVPage[7]=&CHRptr[0][(V)<<13];}}
static INLINE void MMC5BGVROM_BANK8(uint32 V) {if(CHRptr[0]){V&=CHRmask8[0];MMC5BGVPage[0]=MMC5BGVPage[1]=MMC5BGVPage[2]=MMC5BGVPage[3]=MMC5BGVPage[4]=MMC5BGVPage[5]=MMC5BGVPage[6]=MMC5BGVPage[7]=&CHRptr[0][(V)<<13];}}

static int32 inc;
uint8 MMC5fill[0x400];

#define MMC5IRQR        mapbyte3[4]
#define MMC5LineCounter mapbyte3[5]
#define mmc5psize mapbyte1[0]
#define mmc5vsize mapbyte1[1]

uint8 MMC5WRAMsize;
uint8 MMC5WRAMIndex[8];

uint8 MMC5ROMWrProtect[4];
uint8 MMC5MemIn[5];

static void MMC5CHRA(void);
static void MMC5CHRB(void);

typedef struct __cartdata {
        unsigned long crc32;
        unsigned char size;
} cartdata;


// ETROM seems to have 16KB of WRAM, ELROM seems to have 8KB
// EWROM seems to have 32KB of WRAM

#define MMC5_NOCARTS 14
cartdata MMC5CartList[MMC5_NOCARTS]=
{
 {0x9c18762b,2},         /* L'Empereur */
 {0x26533405,2},
 {0x6396b988,2},

 {0xaca15643,2},        /* Uncharted Waters */
 {0xfe3488d1,2},        /* Dai Koukai Jidai */

 {0x15fe6d0f,2},        /* BKAC             */
 {0x39f2ce4b,2},        /* Suikoden              */

 {0x8ce478db,2},        /* Nobunaga's Ambition 2 */
 {0xeee9a682,2},

 {0x1ced086f,2},	/* Ishin no Arashi */

 {0xf540677b,4},        /* Nobunaga...Bushou Fuuun Roku */

 {0x6f4e4312,4},        /* Aoki Ookami..Genchou */

 {0xf011e490,4},        /* Romance of the 3 Kingdoms 2 */
 {0x184c2124,4},        /* Sangokushi 2 */
};


// Called by iNESLoad()
void DetectMMC5WRAMSize(void)
{
 int x;

 MMC5WRAMsize=1;

 for(x=0;x<MMC5_NOCARTS;x++)
  if(iNESGameCRC32==MMC5CartList[x].crc32)
   {
   MMC5WRAMsize=MMC5CartList[x].size;
   break;
   }
}

static void BuildWRAMSizeTable(void)
{
 int x;

 for(x=0;x<8;x++)
 {
  switch(MMC5WRAMsize)
  {
    default:
    case 1:MMC5WRAMIndex[x]=(x>3)?255:0;break;
    case 2:MMC5WRAMIndex[x]=(x&4)>>2;break;
    case 4:MMC5WRAMIndex[x]=(x>3)?255:(x&3);break;
  }
 }
}

static void MMC5CHRA(void)
{
	int x;
	switch(mapbyte1[1]&3)
	{
	 case 0:MMC5SPRVROM_BANK8(mapbyte2[7]);
	        setchr8(mapbyte2[7]);
        	break;
	 case 1:MMC5SPRVROM_BANK4(0x0000,mapbyte2[3]);
        	MMC5SPRVROM_BANK4(0x1000,mapbyte2[7]);
	        setchr4(0x0000,mapbyte2[3]);
	        setchr4(0x1000,mapbyte2[7]);
        	break;
 	 case 2:MMC5SPRVROM_BANK2(0x0000,mapbyte2[1]);
	        MMC5SPRVROM_BANK2(0x0800,mapbyte2[3]);
	        MMC5SPRVROM_BANK2(0x1000,mapbyte2[5]);
	        MMC5SPRVROM_BANK2(0x1800,mapbyte2[7]);
	        setchr2(0x0000,mapbyte2[1]);
        	setchr2(0x0800,mapbyte2[3]);
	        setchr2(0x1000,mapbyte2[5]);
	        setchr2(0x1800,mapbyte2[7]);
        	break;
	 case 3:
	 	for(x=0;x<8;x++)
	        {
	         setchr1(x<<10,mapbyte2[x]);
	         MMC5SPRVROM_BANK1(x<<10,mapbyte2[x]);
		}
	        break;
 }
}

static void MMC5CHRB(void)
{
int x;
switch(mapbyte1[1]&3)
 {
 case 0:MMC5BGVROM_BANK8(mapbyte3[3]);
        setchr8(mapbyte3[3]);
        break;
 case 1:
        MMC5BGVROM_BANK4(0x0000,mapbyte3[3]);
        MMC5BGVROM_BANK4(0x1000,mapbyte3[3]);
        setchr4(0x0000,mapbyte3[3]);
        setchr4(0x1000,mapbyte3[3]);
        break;
 case 2:MMC5BGVROM_BANK2(0x0000,mapbyte3[1]);
        MMC5BGVROM_BANK2(0x0800,mapbyte3[3]);
        MMC5BGVROM_BANK2(0x1000,mapbyte3[1]);
        MMC5BGVROM_BANK2(0x1800,mapbyte3[3]);
        setchr2(0x0000,mapbyte3[1]);
        setchr2(0x0800,mapbyte3[3]);
        setchr2(0x1000,mapbyte3[1]);
        setchr2(0x1800,mapbyte3[3]);
        break;
 case 3:
        for(x=0;x<8;x++)
	{
	 setchr1(x<<10,mapbyte3[x&3]);
         MMC5BGVROM_BANK1(x<<10,mapbyte3[x&3]);
	}
        break;
 }
}

static void FASTAPASS(2) MMC5WRAM(uint32 A, uint32 V)
{
   V=MMC5WRAMIndex[V&7];
   if(V!=255)
   {
    setprg8r(0x10,A,V);
    MMC5MemIn[(A-0x6000)>>13]=1;
   }
   else
    MMC5MemIn[(A-0x6000)>>13]=0;
}

static void MMC5PRG(void)
{
 int x;

 switch(mapbyte1[0]&3)
  {
  case 0:
           MMC5ROMWrProtect[0]=MMC5ROMWrProtect[1]=
           MMC5ROMWrProtect[2]=MMC5ROMWrProtect[3]=1;
           setprg32(0x8000,((mapbyte1[5]&0x7F)>>2));
	   for(x=0;x<4;x++)
 	    MMC5MemIn[1+x]=1;
           break;
  case 1:
         if(mapbyte1[5]&0x80)
          {
           MMC5ROMWrProtect[0]=MMC5ROMWrProtect[1]=1;
           setprg16(0x8000,(mapbyte1[5]>>1));
	   MMC5MemIn[1]=MMC5MemIn[2]=1;
          }
         else
          {
           MMC5ROMWrProtect[0]=MMC5ROMWrProtect[1]=0;
           MMC5WRAM(0x8000,mapbyte1[5]&7&0xFE);
           MMC5WRAM(0xA000,(mapbyte1[5]&7&0xFE)+1);
          }
	  MMC5MemIn[3]=MMC5MemIn[4]=1;
          MMC5ROMWrProtect[2]=MMC5ROMWrProtect[3]=1;
          setprg16(0xC000,(mapbyte1[7]&0x7F)>>1);
         break;
  case 2:
         if(mapbyte1[5]&0x80)
          {
	   MMC5MemIn[1]=MMC5MemIn[2]=1;
           MMC5ROMWrProtect[0]=MMC5ROMWrProtect[1]=1;
           setprg16(0x8000,(mapbyte1[5]&0x7F)>>1);
          }
         else
          {
           MMC5ROMWrProtect[0]=MMC5ROMWrProtect[1]=0;
           MMC5WRAM(0x8000,mapbyte1[5]&7&0xFE);
           MMC5WRAM(0xA000,(mapbyte1[5]&7&0xFE)+1);
          }
         if(mapbyte1[6]&0x80)
          {MMC5ROMWrProtect[2]=1;MMC5MemIn[3]=1;setprg8(0xC000,mapbyte1[6]&0x7F);}
         else
          {MMC5ROMWrProtect[2]=0;MMC5WRAM(0xC000,mapbyte1[6]&7);}
	 MMC5MemIn[4]=1;
         MMC5ROMWrProtect[3]=1;
         setprg8(0xE000,mapbyte1[7]&0x7F);
         break;
  case 3:
	 for(x=0;x<3;x++)
	  if(mapbyte1[4+x]&0x80)
	  {
	   MMC5ROMWrProtect[0]=1;
	   setprg8(0x8000+(x<<13),mapbyte1[4+x]&0x7F);
	   MMC5MemIn[1+x]=1;
	  }
	  else
	  {
	   MMC5ROMWrProtect[0]=0;
	   MMC5WRAM(0x8000+(x<<13),mapbyte1[4+x]&7);
	  }
	 MMC5MemIn[4]=1;
         MMC5ROMWrProtect[3]=1;
         setprg8(0xE000,mapbyte1[7]&0x7F);
         break;
  }
}

#define mul1 mapbyte3[6]
#define mul2 mapbyte3[7]

static DECLFW(Mapper5_write)
{
 switch(A)
  {
   default:break;
   case 0x5105:
                {
                int x;
                for(x=0;x<4;x++)
                {
                 switch((V>>(x<<1))&3)
                 {
                  case 0:PPUNTARAM|=1<<x;vnapage[x]=NTARAM;break;
                  case 1:PPUNTARAM|=1<<x;vnapage[x]=NTARAM+0x400;break;
                  case 2:PPUNTARAM|=1<<x;vnapage[x]=MapperExRAM+0x6000;break;
                  case 3:PPUNTARAM&=~(1<<x);vnapage[x]=MMC5fill;break;
                 }
                }
               }
               mapbyte4[3]=V;
               break;

   case 0x5113:mapbyte4[6]=V;MMC5WRAM(0x6000,V&7);break;
   case 0x5100:mapbyte1[0]=V;MMC5PRG();break;
   case 0x5101:mapbyte1[1]=V;
               if(!mapbyte4[7])
                {MMC5CHRB();MMC5CHRA();}
               else
                {MMC5CHRA();MMC5CHRB();}
               break;

   case 0x5114:
   case 0x5115:
   case 0x5116:
   case 0x5117:
               mapbyte1[A&7]=V;MMC5PRG();break;

   case 0x5120:
   case 0x5121:
   case 0x5122:
   case 0x5123:
   case 0x5124:
   case 0x5125:
   case 0x5126:
   case 0x5127:mapbyte4[7]=0;
               mapbyte2[A&7]=V;MMC5CHRA();break;
   case 0x5128:
   case 0x5129:
   case 0x512a:
   case 0x512b:mapbyte4[7]=1;
               mapbyte3[A&3]=V;MMC5CHRB();break;
   case 0x5102:mapbyte4[0]=V;break;
   case 0x5103:mapbyte4[1]=V;break;
   case 0x5104:mapbyte4[2]=V;MMC5HackCHRMode=V&3;break;
   case 0x5106:if(V!=mapbyte4[4])
               {
		uint32 t;
		t=V|(V<<8)|(V<<16)|(V<<24);
                FCEU_dwmemset(MMC5fill,t,0x3c0);
               }
               mapbyte4[4]=V;
               break;
   case 0x5107:if(V!=mapbyte4[5])
               {
                unsigned char moop;
                uint32 t;
                moop=V|(V<<2)|(V<<4)|(V<<6);
		t=moop|(moop<<8)|(moop<<16)|(moop<<24);
                FCEU_dwmemset(MMC5fill+0x3c0,t,0x40);
               }
               mapbyte4[5]=V;
               break;
   case 0x5200:MMC5HackSPMode=V;break;
   case 0x5201:MMC5HackSPScroll=(V>>3)&0x1F;break;
   case 0x5202:MMC5HackSPPage=V&0x3F;break;
   case 0x5203:X6502_IRQEnd(FCEU_IQEXT);IRQCount=V;break;
   case 0x5204:X6502_IRQEnd(FCEU_IQEXT);IRQa=V&0x80;break;
   case 0x5205:mul1=V;break;
   case 0x5206:mul2=V;break;
  }
}

DECLFR(MMC5_ReadROMRAM)
{
	if(MMC5MemIn[(A-0x6000)>>13])
         return Page[A>>11][A];
	else
	 return X.DB;
}

DECLFW(MMC5_WriteROMRAM)
{
       if(A>=0x8000)
        if(MMC5ROMWrProtect[(A-0x8000)>>13])
         return;
       if(MMC5MemIn[(A-0x6000)>>13])
        if(((mapbyte4[0]&3)|((mapbyte4[1]&3)<<2)) == 6)
         Page[A>>11][A]=V;
}

static DECLFW(MMC5_ExRAMWr)
{
 if(MMC5HackCHRMode!=3)
  (MapperExRAM+0x6000)[A&0x3ff]=V;
}

static DECLFR(MMC5_ExRAMRd)
{
 /* Not sure if this is correct, so I'll comment it out for now. */
 //if(MMC5HackCHRMode>=2)
  return (MapperExRAM+0x6000)[A&0x3ff];
 //else
 // return(X.DB);
}

static DECLFR(MMC5_read)
{
 switch(A)
 {
  //default:printf("$%04x\n",A);break;
  case 0x5204:X6502_IRQEnd(FCEU_IQEXT);
              {
		uint8 x;
		x=MMC5IRQR;
		MMC5IRQR&=0x40;
		return x;
	      }
  case 0x5205:return (mul1*mul2);
  case 0x5206:return ((mul1*mul2)>>8);
 }
 return(X.DB);
}

uint8 dorko[0x400];

void MMC5Synco(void)
{
 int x;

 MMC5PRG();
 for(x=0;x<4;x++)
 {
  switch((mapbyte4[3]>>(x<<1))&3)
   {
    case 0:PPUNTARAM|=1<<x;vnapage[x]=NTARAM;break;
    case 1:PPUNTARAM|=1<<x;vnapage[x]=NTARAM+0x400;break;
    case 2:PPUNTARAM|=1<<x;vnapage[x]=MapperExRAM+0x6000;break;
    case 3:PPUNTARAM&=~(1<<x);vnapage[x]=MMC5fill;break;
   }
 }
 MMC5WRAM(0x6000,mapbyte4[6]&7);
 if(!mapbyte4[7])
 {
  MMC5CHRB();
  MMC5CHRA();
 }
 else
 {
   MMC5CHRA();
   MMC5CHRB();
 }
  {
    uint32 t;
    t=mapbyte4[4]|(mapbyte4[4]<<8)|(mapbyte4[4]<<16)|(mapbyte4[4]<<24);
    FCEU_dwmemset(MMC5fill,t,0x3c0);
  }
  {
   unsigned char moop;
   uint32 t;
   moop=mapbyte4[5]|(mapbyte4[5]<<2)|(mapbyte4[5]<<4)|(mapbyte4[5]<<6);
   t=moop|(moop<<8)|(moop<<16)|(moop<<24);
   FCEU_dwmemset(MMC5fill+0x3c0,t,0x40);
  }
  X6502_IRQEnd(FCEU_IQEXT);
  MMC5HackCHRMode=mapbyte4[2]&3;
}

void MMC5_hbo(void)
{
  if(scanline==0)
   MMC5LineCounter=0;

  if(MMC5LineCounter<245)
  {
   MMC5LineCounter++;
   if(MMC5LineCounter==IRQCount) MMC5IRQR|=0x80;
   if((MMC5LineCounter==IRQCount && IRQa&0x80))
    X6502_IRQBegin(FCEU_IQEXT);
  }
  if(MMC5LineCounter>=239)
    MMC5IRQR|=0x40;
}

void MMC5_hb(void)
{
  if(scanline==240)
  {
   MMC5LineCounter=0;
   MMC5IRQR=0x40;
   return;
  }

  if(MMC5LineCounter<240)
  {
   MMC5LineCounter++;
   if(MMC5LineCounter==IRQCount) 
   {
    MMC5IRQR|=0x80;
    if(IRQa&0x80)
     X6502_IRQBegin(FCEU_IQEXT);
   }
  }
//  printf("%d:%d\n",MMC5LineCounter,scanline);
 if(MMC5LineCounter==240)
   MMC5IRQR=0;
}

void Mapper5_StateRestore(int version)
{
 if(version<=70)
 {
  uint8 tmp[8192];

  FCEU_memmove(tmp,MapperExRAM,8192);
  FCEU_memmove(MapperExRAM,MapperExRAM+8192,16384+8192);
  FCEU_memmove(MapperExRAM+16384+8192,tmp,8192);
 }
 MMC5Synco();
}

#define MMC5PSG         (MapperExRAM+0x640B+8)

static int C5BC[3]={0,0,0};

static void Do5PCM(void)
{
   int32 V;
   int32 start,end;

   start=C5BC[2];
   end=(timestamp<<16)/soundtsinc;
   if(end<=start) return;
   C5BC[2]=end;
   
   if(!(MMC5PSG[0x10]&0x40) && MMC5PSG[0x11])
    for(V=start;V<end;V++)
     Wave[V>>4]+=MMC5PSG[0x11]<<2;
}

DECLFW(Mapper5_SW)
{
 GameExpSound.Fill=MMC5Sound;
 A&=0x1F;

 switch(A)
 {
  case 0x10:
  case 0x11:Do5PCM();break;

  case 0x0:Do5SQ(0);break;
  case 0x2:Do5SQ(0);break;
  case 0x3:Do5SQ(0);break;
  case 0x4:Do5SQ(1);break;
  case 0x6:Do5SQ(1);break;
  case 0x7:Do5SQ(1);break;
  case 0x15:
	  {
	   int t=V^MMC5PSG[0x15];
           if(t&1)
            Do5SQ(0);
           if(t&2)
            Do5SQ(1);
	  }
          break;
 }
 MMC5PSG[A]=V;
}

static int32 vcount[2];
void Do5SQ(int P)
{
    int32 start,end;
    int V;
    int32 freq;

    start=C5BC[P];
    end=(timestamp<<16)/soundtsinc;
    if(end<=start) return;
    C5BC[P]=end;



    if(MMC5PSG[0x15]&(P+1))
    {
     unsigned long dcycs;
     unsigned char amplitude;
     long vcoo;

     freq=(((MMC5PSG[(P<<2)+0x2]|((MMC5PSG[(P<<2)+0x3]&7)<<8))));

     if(freq<8) goto mmc5enda;
     freq+=1;
     inc=(long double)((unsigned long)((FSettings.SndRate OVERSAMPLE)<<12))/((long double)PSG_base/freq);

     switch(MMC5PSG[P<<2]&0xC0)
     {
     default:
     case 0x00:dcycs=inc>>3;break;
     case 0x40:dcycs=inc>>2;break;
     case 0x80:dcycs=inc>>1;break;
     case 0xC0:dcycs=(inc+inc+inc)>>2;break;
     }

      amplitude=(MMC5PSG[P<<2]&15)<<4;

      vcoo=vcount[P];
             for(V=start;V<end;V++)
              {
               if(vcoo<dcycs)
                Wave[V>>4]+=amplitude;
               vcoo+=0x1000;
               if(vcoo>=inc) vcoo-=inc;
               }
      vcount[P]=vcoo;
    }
  mmc5enda:; // semi-colon must be here for MSVC
}

void MMC5Sound(int Count)
{
  int x;
  Do5SQ(0);
  Do5SQ(1);
  Do5PCM();
  for(x=0;x<3;x++)
   C5BC[x]=Count;
}

static void MMC5SoundC(void)
{
 if(FSettings.SndRate)
  Mapper5_ESI();
 else
  SetWriteHandler(0x5000,0x5015,0); 
}

void Mapper5_ESI(void)
{
 GameExpSound.RChange=MMC5SoundC;

 if(FSettings.SndRate)
 {
  SetWriteHandler(0x5000,0x5015,Mapper5_SW);
  SetWriteHandler(0x5205,0x5206,Mapper5_write);
  SetReadHandler(0x5205,0x5206,MMC5_read);
 }
 if(FCEUGameInfo.type==GIT_NSF)
 {
  SetWriteHandler(0x5c00,0x5fef,MMC5_ExRAMWr);
  SetReadHandler(0x5c00,0x5fef,MMC5_ExRAMRd);
  MMC5HackCHRMode=2;
 }
 else
  GameHBIRQHook=MMC5_hb;
}

static void GenMMC5Reset(void)
{
 mapbyte1[4]=mapbyte1[5]=mapbyte1[6]=mapbyte1[7]=~0;
 mapbyte1[0]=mapbyte1[1]=3;
 mapbyte4[2]=0;

 mapbyte4[3]=mapbyte4[4]=mapbyte4[5]=0xFF;

 MMC5Synco();

 SetWriteHandler(0x4020,0x5bff,Mapper5_write);
 SetReadHandler(0x4020,0x5bff,MMC5_read);

 SetWriteHandler(0x5c00,0x5fff,MMC5_ExRAMWr);
 SetReadHandler(0x5c00,0x5fff,MMC5_ExRAMRd);

 SetWriteHandler(0x6000,0xFFFF,MMC5_WriteROMRAM);
 SetReadHandler(0x6000,0xFFFF,MMC5_ReadROMRAM);

 Mapper5_ESI();

 GameHBIRQHook=MMC5_hb;
 FCEU_CheatAddRAM(8,0x6000,WRAM);
 FCEU_CheatAddRAM(1,0x5c00,MapperExRAM+0x6000);
}

void Mapper5_init(void)
{
 AddExState(&MMC5HackSPMode, 1, 0, "SPLM");
 AddExState(&MMC5HackSPScroll, 1, 0, "SPLS");
 AddExState(&MMC5HackSPPage, 1, 0, "SPLP");
 SetupCartPRGMapping(0x10,WRAM,32768,1);
 GenMMC5Reset();
 BuildWRAMSizeTable();
 GameStateRestore=Mapper5_StateRestore;
}

static int m5boo;
static void GenMMC5_Close(void)
{
 UNIFOpenWRAM(UOW_WR,0,1);
 if(m5boo<=16)
  UNIFWriteWRAM(WRAM,8192);
 else
  UNIFWriteWRAM(WRAM,32768);
 UNIFCloseWRAM();
}

static void GenMMC5_Init(int wsize, int battery)
{
 SetupCartPRGMapping(0x10,WRAM,32768,1);
 AddExState(WRAM, 8192, 0, "WRAM");
 AddExState(MapperExRAM, 32768, 0, "MEXR");
 AddExState(&IRQCount, 4, 1, "IRQC");
 AddExState(&IRQa, 1, 0, "IRQA");
 AddExState(mapbyte1, 32, 0, "MPBY");
 AddExState(&MMC5HackSPMode, 1, 0, "SPLM");
 AddExState(&MMC5HackSPScroll, 1, 0, "SPLS");
 AddExState(&MMC5HackSPPage, 1, 0, "SPLP");

 MMC5WRAMsize=wsize/8; 
 BuildWRAMSizeTable();
 GameStateRestore=Mapper5_StateRestore;
 BoardPower=GenMMC5Reset;

 if(battery)
 {
  UNIFOpenWRAM(UOW_RD,0,1);
  if(wsize<=16)
   UNIFReadWRAM(WRAM,8192);
  else
   UNIFReadWRAM(WRAM,32768);
  UNIFCloseWRAM();
  BoardClose=GenMMC5_Close;
  m5boo=wsize;
 }

 MMC5HackVROMMask=CHRmask4[0];
 MMC5HackExNTARAMPtr=MapperExRAM+0x6000;
 MMC5Hack=1;
 MMC5HackVROMPTR=CHRptr[0];
 MMC5HackCHRMode=0;
 MMC5HackSPMode=MMC5HackSPScroll=MMC5HackSPPage=0;

}

// ETROM seems to have 16KB of WRAM, ELROM seems to have 8KB
// EWROM seems to have 32KB of WRAM

// ETROM and EWROM are battery-backed, ELROM isn't.

void ETROM_Init(void)
{
 GenMMC5_Init(16,1);
}

void ELROM_Init(void)
{
 GenMMC5_Init(8,0);
}

void EWROM_Init(void)
{
 GenMMC5_Init(32,1);
}

void EKROM_Init(void)
{
 GenMMC5_Init(8,1);
}
