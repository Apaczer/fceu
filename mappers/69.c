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
static void AYSound(int Count);
static void DoAYSQ(int x);
static void DoAYNoise(void);

#define sunselect mapbyte1[0]
#define sungah    mapbyte1[1]
#define sunindex  mapbyte1[2]

static uint16 znreg;
static int32 inc;

DECLFW(SUN5BWRAM)
{
 if((sungah&0xC0)==0xC0)
  (WRAM-0x6000)[A]=V;
}

DECLFR(SUN5AWRAM)
{
 if((sungah&0xC0)==0x40)
  return X.DB; 
 return CartBR(A);
}

DECLFW(Mapper69_SWL)
{
  sunindex=V%14;
}
DECLFW(Mapper69_SWH)
{
             GameExpSound.Fill=AYSound;
             switch(sunindex)
             {
              case 0:
              case 1:
              case 8:DoAYSQ(0);break;
              case 2:
              case 3:
              case 9:DoAYSQ(1);break;
              case 4:
              case 5:
              case 10:DoAYSQ(2);break;
              case 6:DoAYNoise();znreg=0xFFFF;break;
              case 7:DoAYNoise();
                     DoAYSQ(0);
                     DoAYSQ(1);
                     DoAYSQ(2);break;
             }
             MapperExRAM[sunindex]=V; 
}

DECLFW(Mapper69_write)
{
 switch(A&0xE000)
 {
  case 0x8000:sunselect=V;break;
  case 0xa000:
              sunselect&=0xF;
              if(sunselect<=7)
               VROM_BANK1(sunselect<<10,V);
              else
               switch(sunselect&0x0f)
               {
                case 8:
                       sungah=V;
                       if(V&0x40)
                        {
                         if(V&0x80) // Select WRAM
                          setprg8r(0x10,0x6000,0);
                        }
                        else
                         setprg8(0x6000,V);
                        break;
                case 9:ROM_BANK8(0x8000,V);break;
                case 0xa:ROM_BANK8(0xa000,V);break;
                case 0xb:ROM_BANK8(0xc000,V);break;
                case 0xc:
                         switch(V&3)
                         {
                          case 0:MIRROR_SET2(1);break;
                          case 1:MIRROR_SET2(0);break;
                          case 2:onemir(0);break;
                          case 3:onemir(1);break;
                         }
                         break;
             case 0xd:IRQa=V;break;
             case 0xe:IRQCount&=0xFF00;IRQCount|=V;break;
             case 0xf:IRQCount&=0x00FF;IRQCount|=V<<8;break;
             }
             break;
 }
}

static int32 vcount[4];
static int CAYBC[4]={0,0,0,0};
static void DoAYSQ(int x)
{
    int V;
    uint32 freq;
    unsigned char amp;
    int32 start,end;

    start=CAYBC[x];    
    end=(timestamp<<16)/soundtsinc;
    if(end<=start) return;
    CAYBC[x]=end;

    if(!(MapperExRAM[0x7]&(1<<x)))
    {
      long vcoo;
      freq=(MapperExRAM[x<<1]|((MapperExRAM[(x<<1)+1]&15)<<8))+1;
      inc=(long double)((unsigned long)((FSettings.SndRate OVERSAMPLE)<<12))/
        ((long double)PSG_base/freq);
                            amp=MapperExRAM[0x8+x]&15;
              amp<<=3;
              vcoo=vcount[x];
             if(amp)
              for(V=start;V<end;V++)
               {
                if(vcoo<(inc>>1))
                  Wave[V>>4]+=amp;
                vcoo+=0x1000;
                if(vcoo>=inc) vcoo-=inc;
                }
              vcount[x]=vcoo;
     }
}
static void DoAYNoise(void)
{
    int V;
    uint32 freq;
    unsigned char amp;
    int32 start,end;

    start=CAYBC[3];    
    end=(timestamp<<16)/soundtsinc;
    if(end<=start) return;
    CAYBC[3]=end;

        amp=0;
        for(V=0;V<3;V++)
         {
          if(!(MapperExRAM[0x7]&(8<<V)))
          {
          //if(MapperExRAM[0x8+V]&0x10) amp+=MapperExRAM[0x20]&15;
                                //else
                                amp+=MapperExRAM[0x8+V]&15;
          }
         }
     amp<<=3;

     if(amp)
       {
        freq=PSG_base/(MapperExRAM[0x6]+1);
        if(freq>44100)
         inc=((freq<<11)/(FSettings.SndRate OVERSAMPLE))<<4;
        else
         inc=(freq<<15)/(FSettings.SndRate OVERSAMPLE);

         for(V=start;V<end;V++)
          {
             static uint32 mixer;

             if(vcount[3]>=32768)
             {
               unsigned char feedback;
               mixer=0;
               if(znreg&1) mixer+=amp;
               feedback=((znreg>>13)&1)^((znreg>>14)&1);
               znreg=(znreg<<1)+(feedback);
               vcount[3]-=32768;
             }
             Wave[V>>4]+=mixer;
             vcount[3]+=inc;
           }
       }


}
static void AYSound(int Count)
{
    int x;
    DoAYSQ(0);
    DoAYSQ(1);
    DoAYSQ(2);
    DoAYNoise();
    for(x=0;x<4;x++)
     CAYBC[x]=Count;
}

static void FP_FASTAPASS(1) SunIRQHook(int a)
{
  if(IRQa)
  {
   IRQCount-=a;
   if(IRQCount<=0)
   {TriggerIRQ();IRQa=0;IRQCount=0xFFFF;}
  }
}

void Mapper69_StateRestore(int version)
{
   if(version>=19)
   {
    if(mapbyte1[1]&0x40)
    {
     if(mapbyte1[1]&0x80) // Select WRAM
      setprg8r(0x10,0x6000,0);
    }
    else
     setprg8(0x6000,mapbyte1[1]);
   }
   else
    mapbyte1[1]=0xC0;
}

static void M69SC(void)
{
 if(FSettings.SndRate)
  Mapper69_ESI();
 else 
  SetWriteHandler(0xc000,0xffff,(writefunc)0);
}

void Mapper69_ESI(void)
{
 GameExpSound.RChange=M69SC; 
 if(FSettings.SndRate)
 {
  SetWriteHandler(0xc000,0xdfff,Mapper69_SWL);
  SetWriteHandler(0xe000,0xffff,Mapper69_SWH);
 }
}

void Mapper69_init(void)
{
 SetupCartPRGMapping(0x10,WRAM,8192,1);

 SetWriteHandler(0x8000,0xbfff,Mapper69_write);
 SetWriteHandler(0x6000,0x7fff,SUN5BWRAM);
 SetReadHandler(0x6000,0x7fff,SUN5AWRAM);
 Mapper69_ESI();
 MapIRQHook=SunIRQHook;
 MapStateRestore=Mapper69_StateRestore;
 znreg=0;
}

