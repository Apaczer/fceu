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

/*  Code for emulating iNES mappers 4, 118,119 */

#include "mapinc.h"

#define resetmode mapbyte1[0]
#define MMC3_cmd mapbyte1[1]
#define A000B mapbyte1[2]
#define A001B mapbyte1[3]
#define DRegBuf mapbyte4

#define PPUCHRBus mapbyte2[0]
#define TKSMIR mapbyte3
#define PIRREGS	mapbyte2

static void (*pwrap)(uint32 A, uint8 V);
static void (*cwrap)(uint32 A, uint8 V);
static void (*mwrap)(uint8 V);

static int mmc3opts=0;

static INLINE void FixMMC3PRG(int V);
static INLINE void FixMMC3CHR(int V);

static int latched;

static DECLFW(MMC3_IRQWrite)
{
	//printf("$%04x:$%02x, %d\n",A,V,scanline);
        switch(A&0xE001)
        {
         case 0xc000:IRQLatch=V;
	             latched=1;
		     if(resetmode)
		     {
		      IRQCount=V;
		      latched=0;
		      //resetmode=0;
		     }
                     break;
         case 0xc001:IRQCount=IRQLatch;
                     break;
         case 0xE000:IRQa=0;
		     X6502_IRQEnd(FCEU_IQEXT);
		     resetmode=1;
                     break;
         case 0xE001:IRQa=1;
		     if(latched) 
		      IRQCount=IRQLatch;
                     break;
        }
}

static INLINE void FixMMC3PRG(int V)
{
          if(V&0x40)
          {
           pwrap(0xC000,DRegBuf[6]);
           pwrap(0x8000,~1);
          }
          else
          {
           pwrap(0x8000,DRegBuf[6]);
           pwrap(0xC000,~1);
          }
	  pwrap(0xA000,DRegBuf[7]);
	  pwrap(0xE000,~0);
}

static INLINE void FixMMC3CHR(int V)
{
           int cbase=(V&0x80)<<5;
           cwrap((cbase^0x000),DRegBuf[0]&(~1));
           cwrap((cbase^0x400),DRegBuf[0]|1);
           cwrap((cbase^0x800),DRegBuf[1]&(~1));
           cwrap((cbase^0xC00),DRegBuf[1]|1);

           cwrap(cbase^0x1000,DRegBuf[2]);
           cwrap(cbase^0x1400,DRegBuf[3]);
           cwrap(cbase^0x1800,DRegBuf[4]);
           cwrap(cbase^0x1c00,DRegBuf[5]);
}

static void MMC3RegReset(void)
{
 IRQCount=IRQLatch=IRQa=MMC3_cmd=0;

 DRegBuf[0]=0;
 DRegBuf[1]=2;
 DRegBuf[2]=4;
 DRegBuf[3]=5;
 DRegBuf[4]=6;
 DRegBuf[5]=7;
 DRegBuf[6]=0;
 DRegBuf[7]=1;

 FixMMC3PRG(0);
 FixMMC3CHR(0);
}

static DECLFW(Mapper4_write)
{
        switch(A&0xE001)
	{
         case 0x8000:
          if((V&0x40) != (MMC3_cmd&0x40))
	   FixMMC3PRG(V);
          if((V&0x80) != (MMC3_cmd&0x80))
           FixMMC3CHR(V);
          MMC3_cmd = V;
          break;

        case 0x8001:
                {
                 int cbase=(MMC3_cmd&0x80)<<5;
                 DRegBuf[MMC3_cmd&0x7]=V;
                 switch(MMC3_cmd&0x07)
                 {
                  case 0: cwrap((cbase^0x000),V&(~1));
			  cwrap((cbase^0x400),V|1);
			  break;
                  case 1: cwrap((cbase^0x800),V&(~1));
			  cwrap((cbase^0xC00),V|1);
			  break;
                  case 2: cwrap(cbase^0x1000,V); break;
                  case 3: cwrap(cbase^0x1400,V); break;
                  case 4: cwrap(cbase^0x1800,V); break;
                  case 5: cwrap(cbase^0x1C00,V); break;
                  case 6: if (MMC3_cmd&0x40) pwrap(0xC000,V);
                          else pwrap(0x8000,V);
                          break;
                  case 7: pwrap(0xA000,V);
                          break;
                 }
                }
                break;

        case 0xA000:
	        if(mwrap) mwrap(V&1);
                break;
	case 0xA001:
		A001B=V;
		break;
 }
}

static void MMC3_hb(void)
{
      resetmode=0;
      if(IRQCount>=0)
      {
        IRQCount--;
        if(IRQCount<0)
        {
	 //printf("IRQ: %d\n",scanline);
         if(IRQa)
	  X6502_IRQBegin(FCEU_IQEXT);
        }
      }
}
static int isines;

static void genmmc3restore(int version)
{
 if(version>=56)
 {
  mwrap(A000B&1);
  FixMMC3PRG(MMC3_cmd);
  FixMMC3CHR(MMC3_cmd);
 }
 else if(isines)
  iNESStateRestore(version);
}

static void GENCWRAP(uint32 A, uint8 V)
{
 setchr1(A,V);
}

static void GENPWRAP(uint32 A, uint8 V)
{
 setprg8(A,V&0x3F);
}

static void GENMWRAP(uint8 V)
{
 A000B=V;
 setmirror(V^1);
}

static void GENNOMWRAP(uint8 V)
{
 A000B=V;
}

static void genmmc3ii(void (*PW)(uint32 A, uint8 V), 
		      void (*CW)(uint32 A, uint8 V), 
		      void (*MW)(uint8 V))
{
 pwrap=GENPWRAP;
 cwrap=GENCWRAP;
 mwrap=GENMWRAP;
 if(PW) pwrap=PW;
 if(CW) cwrap=CW;
 if(MW) mwrap=MW;
 A000B=(Mirroring&1)^1; // For hard-wired mirroring on some MMC3 games.
                        // iNES format needs to die or be extended...
 mmc3opts=0;
 SetWriteHandler(0x8000,0xBFFF,Mapper4_write);
 SetWriteHandler(0xC000,0xFFFF,MMC3_IRQWrite);

 GameHBIRQHook=MMC3_hb;
 GameStateRestore=genmmc3restore;
 if(!VROM_size)
  SetupCartCHRMapping(0, CHRRAM, 8192, 1);
 isines=1;
 MMC3RegReset();
 MapperReset=MMC3RegReset;
}

void Mapper4_init(void)
{
 genmmc3ii(0,0,0);
}

static void M47PW(uint32 A, uint8 V)
{
 V&=0xF;
 V|=PIRREGS[0]<<4;
 setprg8(A,V);
}

static void M47CW(uint32 A, uint8 V)
{
 uint32 NV=V;
 NV&=0x7F;
 NV|=PIRREGS[0]<<7;
 setchr1(A,NV);
}

static DECLFW(M47Write)
{
 PIRREGS[0]=V&1;
 FixMMC3PRG(MMC3_cmd);
 FixMMC3CHR(MMC3_cmd); 
}

void Mapper47_init(void)
{
 genmmc3ii(M47PW,M47CW,0);
 SetWriteHandler(0x6000,0x7FFF,M47Write);
 SetReadHandler(0x6000,0x7FFF,0);
}

static void M44PW(uint32 A, uint8 V)
{
 uint32 NV=V;
 if(PIRREGS[0]>=6) NV&=0x1F;
 else NV&=0x0F;
 NV|=PIRREGS[0]<<4;
 setprg8(A,NV);
}
static void M44CW(uint32 A, uint8 V)
{
 uint32 NV=V;
 if(PIRREGS[0]<6) NV&=0x7F;
 NV|=PIRREGS[0]<<7;
 setchr1(A,NV);
}

static DECLFW(Mapper44_write)
{
 if(A&1)
 {
  PIRREGS[0]=V&7;
  FixMMC3PRG(MMC3_cmd);
  FixMMC3CHR(MMC3_cmd);
 }
 else
  Mapper4_write(A,V);
}

void Mapper44_init(void)
{
 genmmc3ii(M44PW,M44CW,0);
 SetWriteHandler(0xA000,0xBFFF,Mapper44_write);
}

static void M52PW(uint32 A, uint8 V)
{
 uint32 NV=V;
 NV&=0x1F^((PIRREGS[0]&8)<<1);
 NV|=((PIRREGS[0]&6)|((PIRREGS[0]>>3)&PIRREGS[0]&1))<<4;
 setprg8(A,NV);
}

static void M52CW(uint32 A, uint8 V)
{
 uint32 NV=V;
 NV&=0xFF^((PIRREGS[0]&0x40)<<1);
 NV|=(((PIRREGS[0]>>3)&4)|((PIRREGS[0]>>1)&2)|((PIRREGS[0]>>6)&(PIRREGS[0]>>4)&1))<<7;
 setchr1(A,NV);
}

static DECLFW(Mapper52_write)
{
 if(PIRREGS[1]) 
 {
  (WRAM-0x6000)[A]=V;
  return;
 }
 PIRREGS[1]=1;
 PIRREGS[0]=V;
 FixMMC3PRG(MMC3_cmd);
 FixMMC3CHR(MMC3_cmd);
}

static void M52Reset(void)
{
 PIRREGS[0]=PIRREGS[1]=0;
 MMC3RegReset(); 
}

void Mapper52_init(void)
{
 genmmc3ii(M52PW,M52CW,0);
 SetWriteHandler(0x6000,0x7FFF,Mapper52_write);
 MapperReset=M52Reset;
}

static void M45CW(uint32 A, uint8 V)
{
 uint32 NV=V;
 if(PIRREGS[2]&8)
  NV&=(1<<( (PIRREGS[2]&7)+1 ))-1;
 else
  NV&=0;
 NV|=PIRREGS[0]|((PIRREGS[2]&0x10)<<4);
 setchr1(A,NV);
}

static void M45PW(uint32 A, uint8 V)
{
 V&=(PIRREGS[3]&0x3F)^0x3F;
 V|=PIRREGS[1];
 setprg8(A,V);
}

static DECLFW(Mapper45_write)
{
 if(PIRREGS[3]&0x40) return;
 PIRREGS[PIRREGS[4]]=V;
 PIRREGS[4]=(PIRREGS[4]+1)&3;
 FixMMC3PRG(MMC3_cmd);
 FixMMC3CHR(MMC3_cmd);
}

static void M45Reset(void)
{
 FCEU_dwmemset(PIRREGS,0,5);
 MMC3RegReset();
}

void Mapper45_init(void)
{
 genmmc3ii(M45PW,M45CW,0);
 SetWriteHandler(0x6000,0x7FFF,Mapper45_write);
 SetReadHandler(0x6000,0x7FFF,0);
 MapperReset=M45Reset;
}
static void M49PW(uint32 A, uint8 V)
{
 if(PIRREGS[0]&1)
 {
  V&=0xF;
  V|=(PIRREGS[0]&0xC0)>>2;
  setprg8(A,V);
 }
 else
  setprg32(0x8000,(PIRREGS[0]>>4)&3);
}

static void M49CW(uint32 A, uint8 V)
{
 uint32 NV=V;
 NV&=0x7F;
 NV|=(PIRREGS[0]&0xC0)<<1;
 setchr1(A,NV);
}

static DECLFW(M49Write)
{
 //printf("$%04x:$%02x\n",A,V);
 if(A001B&0x80)
 {
  PIRREGS[0]=V;
  FixMMC3PRG(MMC3_cmd);
  FixMMC3CHR(MMC3_cmd);
 }
}

static void M49Reset(void)
{
 PIRREGS[0]=0;
 MMC3RegReset();
}

void Mapper49_init(void)
{
 genmmc3ii(M49PW,M49CW,0);
 SetWriteHandler(0x6000,0x7FFF,M49Write);
 SetReadHandler(0x6000,0x7FFF,0);
 MapperReset=M49Reset;
}

static DECLFW(Mapper250_write)
{
	Mapper4_write((A&0xE000)|((A&0x400)>>10),A&0xFF);
}

static DECLFW(M250_IRQWrite)
{
	MMC3_IRQWrite((A&0xE000)|((A&0x400)>>10),A&0xFF);
}

void Mapper250_init(void)
{
 genmmc3ii(0,0,0);
 SetWriteHandler(0x8000,0xBFFF,Mapper250_write);
 SetWriteHandler(0xC000,0xFFFF,M250_IRQWrite);
}

static void FP_FASTAPASS(1) TKSPPU(uint32 A)
{
 //static uint8 z;
 //if(A>=0x2000 || type<0) return;
 //if(type<0) return;
 A&=0x1FFF;
 //if(scanline>=140 && scanline<=200) {setmirror(MI_1);return;}
 //if(scanline>=140 && scanline<=200)
 // if(scanline>=190 && scanline<=200) {setmirror(MI_1);return;}
 // setmirror(MI_1); 
 //printf("$%04x\n",A);

 A>>=10;
 PPUCHRBus=A;
 setmirror(MI_0+TKSMIR[A]);
}

static void TKSWRAP(uint32 A, uint8 V)
{
 TKSMIR[A>>10]=V>>7;
 setchr1(A,V&0x7F);
 if(PPUCHRBus==(A>>10))
  setmirror(MI_0+(V>>7));
}

void Mapper118_init(void)
{
  genmmc3ii(0,TKSWRAP,GENNOMWRAP);
  PPU_hook=TKSPPU;
}

static void TQWRAP(uint32 A, uint8 V)
{
 setchr1r((V&0x40)>>2,A,V&0x3F);
}

void Mapper119_init(void)
{
  genmmc3ii(0,TQWRAP,0);
  SetupCartCHRMapping(0x10, CHRRAM, 8192, 1);
}

static int wrams;

static void GenMMC3Close(void)
{
 UNIFOpenWRAM(UOW_WR,0,1);
 UNIFWriteWRAM(WRAM,wrams);
 UNIFCloseWRAM();
}

static DECLFW(MBWRAM)
{
  (WRAM-0x6000)[A]=V;
}

static DECLFR(MAWRAM)
{
 return((WRAM-0x6000)[A]);
}

static DECLFW(MBWRAMMMC6)
{
 WRAM[A&0x3ff]=V;
}

static DECLFR(MAWRAMMMC6)
{
 return(WRAM[A&0x3ff]);
}

static void GenMMC3Power(void)
{
 SetWriteHandler(0x8000,0xBFFF,Mapper4_write);
 SetReadHandler(0x8000,0xFFFF,CartBR);
 SetWriteHandler(0xC000,0xFFFF,MMC3_IRQWrite);

 if(mmc3opts&1)
 {
  if(wrams==1024)
  {
   FCEU_CheatAddRAM(1,0x7000,WRAM);
   SetReadHandler(0x7000,0x7FFF,MAWRAMMMC6);
   SetWriteHandler(0x7000,0x7FFF,MBWRAMMMC6);
  }
  else
  {
   FCEU_CheatAddRAM(wrams/1024,0x6000,WRAM);
   SetReadHandler(0x6000,0x6000+wrams-1,MAWRAM);
   SetWriteHandler(0x6000,0x6000+wrams-1,MBWRAM);
  }
  if(!(mmc3opts&2))
   FCEU_dwmemset(WRAM,0,wrams);
 }
 MMC3RegReset();
}

void GenMMC3_Init(int prg, int chr, int wram, int battery)
{
 pwrap=GENPWRAP;
 cwrap=GENCWRAP;
 mwrap=GENMWRAP;

 wrams=wram*1024;

 PRGmask8[0]&=(prg>>13)-1;
 CHRmask1[0]&=(chr>>10)-1;
 CHRmask2[0]&=(chr>>11)-1;

 if(wram)
 {
  mmc3opts|=1;
  AddExState(WRAM, wram*1024, 0, "WRAM");
 }

 if(battery)
 {
  mmc3opts|=2;
  BoardClose=GenMMC3Close;

  UNIFOpenWRAM(UOW_RD,0,1);
  UNIFReadWRAM(WRAM,wram*1024);
  UNIFCloseWRAM();
 }

 if(!chr)
 {
  CHRmask1[0]=7;
  CHRmask2[0]=3;
  SetupCartCHRMapping(0, CHRRAM, 8192, 1);
  AddExState(CHRRAM, 8192, 0, "CHRR");
 }
 AddExState(mapbyte1, 32, 0, "MPBY");
 AddExState(&IRQa, 1, 0, "IRQA");
 AddExState(&IRQCount, 4, 1, "IRQC");
 AddExState(&IRQLatch, 4, 1, "IQL1");

 BoardPower=GenMMC3Power;
 BoardReset=MMC3RegReset;

 GameHBIRQHook=MMC3_hb;
 GameStateRestore=genmmc3restore;
 isines=0;
}

// void GenMMC3_Init(int prg, int chr, int wram, int battery)

void TFROM_Init(void)
{
 GenMMC3_Init(512, 64, 0, 0);
}

void TGROM_Init(void)
{
 GenMMC3_Init(512, 0, 0, 0);
}

void TKROM_Init(void)
{
 GenMMC3_Init(512, 256, 8, 1);
}

void TLROM_Init(void)
{
 GenMMC3_Init(512, 256, 0, 0);
}

void TSROM_Init(void)
{
 GenMMC3_Init(512, 256, 8, 0);
}

void TLSROM_Init(void)
{
 GenMMC3_Init(512, 256, 8, 0);
 cwrap=TKSWRAP;
 mwrap=GENNOMWRAP;
}

void TKSROM_Init(void)
{
 GenMMC3_Init(512, 256, 8, 1);
 cwrap=TKSWRAP;
 mwrap=GENNOMWRAP;
}

void TQROM_Init(void)
{
 GenMMC3_Init(512, 64, 0, 0);
 cwrap=TQWRAP;
}

/* MMC6 board */
void HKROM_Init(void)
{
 GenMMC3_Init(512, 512, 1, 1);
}
