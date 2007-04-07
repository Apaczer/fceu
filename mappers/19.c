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

#include "mapinc.h"

#define dopol mapbyte1[0]
#define gorfus mapbyte1[1]
#define gorko mapbyte1[2]

static void NamcoSound(int Count);
static void NamcoSoundHack(void);

static int32 inc;
void FP_FASTAPASS(1) NamcoIRQHook(int a)
{
  if(IRQa)
  {
   IRQCount+=a;
   if(IRQCount>=0x7fff)
   {
    X6502_IRQBegin(FCEU_IQEXT);
    IRQa=0;
    IRQCount=0x7fff;
   }
  }
}

DECLFR(Namco_Read4800)
{
	uint8 ret=MapperExRAM[dopol&0x7f];
	if(dopol&0x80)
	 dopol=(dopol&0x80)|((dopol+1)&0x7f);
	return ret;
}

DECLFR(Namco_Read5000)
{
 return(IRQCount);
}

DECLFR(Namco_Read5800)
{
 return(IRQCount>>8);
}

static void FASTAPASS(2) DoNTARAMROM(int w, uint8 V)
{
        mapbyte2[w]=V;
        //if(V>=0xE0)
	// setntamem(NTARAM+((V&1)<<10), 1, w);
	if((V>=0xE0)) // || ((gorko>>(6+(w>>1)))&1) )
	 setntamem(NTARAM+((V&1)<<10), 1, w);
	else 
	{
	 V&=CHRmask1[0];
	 setntamem(VROM+(V<<10), 0, w);
	}
//	else
//	 setntamem(NTARAM+((V&1)<<10), 1, w);
}

static void FixNTAR(void)
{
 int x;

 for(x=0;x<4;x++)
  DoNTARAMROM(x,mapbyte2[x]);
}

static void FASTAPASS(2) DoCHRRAMROM(int x, uint8 V)
{
         mapbyte3[x]=V;
         if(!((gorfus>>((x>>2)+6))&1) && (V>=0xE0))
          VRAM_BANK1(x<<10,V&7);
         else
          VROM_BANK1(x<<10,V);
}

static void FixCRR(void)
{
 int x;
 for(x=0;x<8;x++)
  DoCHRRAMROM(x,mapbyte3[x]);
}

static DECLFW(Mapper19_write)
{
	A&=0xF800;

	if(A>=0x8000 && A<=0xb800)
	 DoCHRRAMROM((A-0x8000)>>11,V);
	else if(A>=0xC000 && A<=0xd800)
 	 DoNTARAMROM((A-0xC000)>>11,V);
        else switch(A)
	{
	 case 0x4800:
		   if(dopol&0x40)
                   {
                    if(FSettings.SndRate)
	  	    {
		     NamcoSoundHack();
                     GameExpSound.Fill=NamcoSound;
		    }
                   }
		   MapperExRAM[dopol&0x7f]=V;
                   if(dopol&0x80)
                    dopol=(dopol&0x80)|((dopol+1)&0x7f);
                   break;

        case 0xf800: dopol=V;break;
        case 0x5000: IRQCount&=0xFF00;IRQCount|=V;X6502_IRQEnd(FCEU_IQEXT);break;
        case 0x5800: IRQCount&=0x00ff;IRQCount|=(V&0x7F)<<8;
                     IRQa=V&0x80;
                     X6502_IRQEnd(FCEU_IQEXT);
                     break;

        case 0xE000:gorko=V&0xC0;
		    //FixNTAR();
                    ROM_BANK8(0x8000,V);
                    break;
        case 0xE800:gorfus=V&0xC0;
		    FixCRR();
                    ROM_BANK8(0xA000,V);
                    break;
        case 0xF000:
                    ROM_BANK8(0xC000,V);
                    break;
        }
}

static int dwave=0;
static void DoNamcoSound(uint32 *Wave, int Count);

static void NamcoSoundHack(void)
{
 int32 z,a;

 z=((timestamp<<16)/soundtsinc)>>4;  
 a=z-dwave;
 if(a)
  DoNamcoSound(&Wave[dwave], a);
 dwave+=a;
}

static void NamcoSound(int Count)
{
 int32 z,a;

 z=((timestamp<<16)/soundtsinc)>>4;
 a=z-dwave;
 if(a)
   DoNamcoSound(&Wave[dwave], a);
 dwave=0;
}

static uint8 PlayIndex[8];
static int32 vcount[8];

static void DoNamcoSound(uint32 *Wave, int Count)
{
	int P,V;


      //FCEU_DispMessage("%d",MapperExRAM[0x7F]>>4);
      for(P=7;P>=7-((MapperExRAM[0x7F]>>4)&7);P--)
      {
       if((MapperExRAM[0x44+(P<<3)]&0xE0) && (MapperExRAM[0x47+(P<<3)]&0xF))
       {
	uint32 freq;
	int32 vco;
	uint32 duff,duff2,lengo,envelope;
        //uint64 ta;

        vco=vcount[P];
        freq=MapperExRAM[0x40+(P<<3)];
        freq|=MapperExRAM[0x42+(P<<3)]<<8;
        freq|=(MapperExRAM[0x44+(P<<3)]&3)<<16;

        if(!freq) continue;

	{
	 int c=((MapperExRAM[0x7F]>>4)&7)+1;

	 inc=(long double)(FSettings.SndRate<<15)/((long double)freq*
	 	21477272/((long double)0x400000*c*45));
	}

        envelope=((MapperExRAM[0x47+(P<<3)]&0xF)<<18)/15;
        duff=MapperExRAM[(((MapperExRAM[0x46+(P<<3)]+PlayIndex[P])>>1)&0xFF)];
        if((MapperExRAM[0x46+(P<<3)]+PlayIndex[P])&1)
         duff>>=4;
        duff&=0xF;
        duff2=(duff*envelope)>>14;
  
        lengo=((8-((MapperExRAM[0x44+(P<<3)]>>2)&7)))<<2;
        for(V=0;V<Count OVERSAMPLE;V++)
        {
         if(vco>=inc)
         {
          PlayIndex[P]++;
          if(PlayIndex[P]>=lengo)
           PlayIndex[P]=0;
          vco-=inc;
          duff=MapperExRAM[(((MapperExRAM[0x46+(P<<3)]+PlayIndex[P])&0xFF)>>1)];
          if((MapperExRAM[0x46+(P<<3)]+PlayIndex[P])&1)
           duff>>=4;
          duff&=0xF;
          duff2=(duff*envelope)>>14;
         }
          Wave[V>>4]+=duff2;
          vco+=0x8000;
        }
        vcount[P]=vco;
       }
      }
}

static void Mapper19_StateRestore(int version)
{
 FixNTAR();
 if(version>=80)
  FixCRR();
}

static void M19SC(void)
{
 if(FSettings.SndRate)
  Mapper19_ESI();
}

void Mapper19_ESI(void)
{
  GameExpSound.RChange=M19SC;
  SetWriteHandler(0xf800,0xffff,Mapper19_write);
  SetWriteHandler(0x4800,0x4fff,Mapper19_write);
  SetReadHandler(0x4800,0x4fff,Namco_Read4800);
}

void Mapper19_init(void)
{
        if(!Mirroring)
        {
         DoNTARAMROM(0,0xE0);
         DoNTARAMROM(1,0xE0);
         DoNTARAMROM(2,0xE1);
         DoNTARAMROM(3,0xE1);
        }
        else
        {
         DoNTARAMROM(0,0xE0);
         DoNTARAMROM(2,0xE0);
         DoNTARAMROM(1,0xE1);
         DoNTARAMROM(3,0xE1);
        }
        VROM_BANK8(~0);
        SetWriteHandler(0x8000,0xffff,Mapper19_write);
        SetWriteHandler(0x4020,0x5fff,Mapper19_write);
        SetReadHandler(0x4800,0x4fff,Namco_Read4800);
	SetReadHandler(0x5000,0x57ff,Namco_Read5000);
	SetReadHandler(0x5800,0x5fff,Namco_Read5800);

        MapIRQHook=NamcoIRQHook;
        MapStateRestore=Mapper19_StateRestore;
        GameExpSound.RChange=M19SC;
        if(FSettings.SndRate)
         Mapper19_ESI();
        gorfus=0xFF;
}

