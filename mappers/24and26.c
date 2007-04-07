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

#define vrctemp mapbyte1[0]
#define regb000 mapbyte3[0]
#define regb001 mapbyte3[1]
#define regb002 mapbyte3[2]
#define exchstat mapbyte4[0]
#define VPSG2 mapbyte3
#define VPSG mapbyte2

static void DoSQV1(void);
static void DoSQV2(void);
static void DoSawV(void);

static int swaparoo;
static int32 inc;
static DECLFW(VRC6PSGW90)
{
 DoSQV1();VPSG[0]=V;
}
static DECLFW(VRC6PSGW91)
{
 DoSQV1();VPSG[2]=V;
}
static DECLFW(VRC6PSGW92)
{
 DoSQV1();VPSG[3]=V;
}
static DECLFW(VRC6PSGWA0)
{
 DoSQV2();VPSG[4]=V;
}
static DECLFW(VRC6PSGWA1)
{
 DoSQV2();VPSG[6]=V;
}
static DECLFW(VRC6PSGWA2)
{
 DoSQV2();VPSG[7]=V;
}
static DECLFW(VRC6PSGWB0)
{
 DoSawV();VPSG2[0]=V;
}
static DECLFW(VRC6PSGWB1)
{
 DoSawV();VPSG2[1]=V;
}
static DECLFW(VRC6PSGWB2)
{
 DoSawV();VPSG2[2]=V;
}

static int acount=0;

static void FP_FASTAPASS(1) KonamiIRQHook(int a)
{
  #define LCYCS 114
  if(IRQa)
  {
   acount+=a;
   if(acount>=LCYCS)
   {
    doagainbub:acount-=LCYCS;IRQCount++;
    if(IRQCount==0x100) {TriggerIRQ();IRQCount=IRQLatch;}
    if(acount>=LCYCS) goto doagainbub;
   }
 }
}

DECLFW(Mapper24_write)
{
	if(swaparoo)
	 A=(A&0xFFFC)|((A>>1)&1)|((A<<1)&2);

        switch(A&0xF003)
	{
         case 0x8000:ROM_BANK16(0x8000,V);break;
         case 0xB003:
	   	 switch(V&0xF)
	         {
	          case 0x0:MIRROR_SET2(1);break;
	          case 0x4:MIRROR_SET2(0);break;
	          case 0x8:onemir(0);break;
	          case 0xC:onemir(1);break;
	         }
	         break;
         case 0xC000:ROM_BANK8(0xC000,V);break;
         case 0xD000:VROM_BANK1(0x0000,V);break;
         case 0xD001:VROM_BANK1(0x0400,V);break;
         case 0xD002:VROM_BANK1(0x0800,V);break;
         case 0xD003:VROM_BANK1(0x0c00,V);break;
         case 0xE000:VROM_BANK1(0x1000,V);break;
         case 0xE001:VROM_BANK1(0x1400,V);break;
         case 0xE002:VROM_BANK1(0x1800,V);break;
         case 0xE003:VROM_BANK1(0x1c00,V);break;
         case 0xF000:IRQLatch=V;break;
         case 0xF001:IRQa=V&2;
                     vrctemp=V&1;
                     if(V&2) {IRQCount=IRQLatch;}
		     //acount=0;
                     break;
         case 0xf002:IRQa=vrctemp;break;
         case 0xF003:break;
  }
}

static int CVBC[3]={0,0,0};
static int32 vcount[2];

static void DoSQV1(void)
{
    uint8 amp;
    int32 freq;
    int V;
    int32 start,end;

    start=CVBC[0];    
    end=(timestamp<<16)/soundtsinc;
    if(end<=start) return;
    CVBC[0]=end;

    if(VPSG[0x3]&0x80)
    {
     amp=(VPSG[0]&15)<<4;
     if(VPSG[0]&0x80)
     {
      for(V=start;V<end;V++)
       Wave[V>>4]+=amp;
     }
     else
     {
      unsigned long dcycs;      
      freq=(((VPSG[0x2]|((VPSG[0x3]&15)<<8))+1));
      inc=(long double)((unsigned long)((FSettings.SndRate OVERSAMPLE)<<12))/((long double)PSG_base/freq);
      switch(VPSG[0]&0x70)
      {
       default:
       case 0x00:dcycs=inc>>4;break;
       case 0x10:dcycs=inc>>3;break;
       case 0x20:dcycs=(inc*3)>>4;break;
       case 0x30:dcycs=inc>>2;break;
       case 0x40:dcycs=(inc*5)>>4;break;
       case 0x50:dcycs=(inc*6)>>4;break;
       case 0x60:dcycs=(inc*7)>>4;break;
       case 0x70:dcycs=inc>>1;break;
      }
             for(V=start;V<end;V++)
              {
               if(vcount[0]<dcycs)
                 Wave[V>>4]+=amp;
               vcount[0]+=0x1000;
               if(vcount[0]>=inc) vcount[0]-=inc;
               }
     }
    }
}
static void DoSQV2(void)
{
    uint8 amp;
    int32 freq;
    int V;
    int32 start,end;

    start=CVBC[1];
    end=(timestamp<<16)/soundtsinc;   
    if(end<=start) return;
    CVBC[1]=end;

    if(VPSG[0x7]&0x80)
    {
     amp=(VPSG[4]&15)<<4;
     if(VPSG[4]&0x80)
     {
      for(V=start;V<end;V++)
       Wave[V>>4]+=amp;
     }
     else
     {
      unsigned long dcycs;
      freq=(((VPSG[0x6]|((VPSG[0x7]&15)<<8))+1));
      inc=(long double)((unsigned long)((FSettings.SndRate OVERSAMPLE)<<12))/((long double)PSG_base/freq);
      switch(VPSG[4]&0x70)
      {
       default:
       case 0x00:dcycs=inc>>4;break;
       case 0x10:dcycs=inc>>3;break;
       case 0x20:dcycs=(inc*3)>>4;break;
       case 0x30:dcycs=inc>>2;break;
       case 0x40:dcycs=(inc*5)>>4;break;
       case 0x50:dcycs=(inc*6)>>4;break;
       case 0x60:dcycs=(inc*7)>>4;break;
       case 0x70:dcycs=inc>>1;break;
      }
             for(V=start;V<end;V++)
              {
               if(vcount[1]<dcycs)
                 Wave[V>>4]+=amp;
               vcount[1]+=0x1000;
               if(vcount[1]>=inc) vcount[1]-=inc;
               }
     }
    }
}

static void DoSawV(void)
{
    int V;
    int32 start,end;

    start=CVBC[2];
    end=(timestamp<<16)/soundtsinc;   
    if(end<=start) return;
    CVBC[2]=end;

   if(VPSG2[2]&0x80)
   {
    static int64 saw1phaseacc=0;
    uint32 freq3;
    static uint8 b3=0;
    static int32 phaseacc=0;
    static uint32 duff=0;

    freq3=(VPSG2[1]+((VPSG2[2]&15)<<8)+1);

    for(V=start;V<end;V++)
    {
     saw1phaseacc-=nesincsizeLL;
     if(saw1phaseacc<=0)
     {
      int64 t;
      rea:
      t=freq3;
      t<<=50;	// 49 + 1
      saw1phaseacc+=t;
      phaseacc+=VPSG2[0]&0x3f;
      b3++;
      if(b3==7)
      {
       b3=0;
       phaseacc=0;
      }
      if(saw1phaseacc<=0) 
       goto rea;
      duff=(((phaseacc>>3)&0x1f)<<4);
      }
     Wave[V>>4]+=duff;
    }
   }
}

void VRC6Sound(int Count)
{
    int x;

    DoSQV1();
    DoSQV2();
    DoSawV();
    for(x=0;x<3;x++)
     CVBC[x]=Count;
}

static int satype=0;

void VRC6SoundC(void)
{
  int x;

  if(FSettings.SndRate)
   VRC6_ESI(satype);
  else
  {
   for(x=000;x<0x1000;x+=4)
   {
          SetWriteHandler(0x9000+x,0x9002+x,0); 
          SetWriteHandler(0xa000+x,0xa002+x,0); 
          SetWriteHandler(0xb000+x,0xb002+x,0); 
   }
  }
}

void VRC6_ESI(int t)
{
        int x;

	satype=t;

        GameExpSound.RChange=VRC6SoundC;
        GameExpSound.Fill=VRC6Sound;
        if(FSettings.SndRate)
         for(x=000;x<0x1000;x+=4)
         {
          uint32 a;

          a=0x9000+x;
          SetWriteHandler(a,a,VRC6PSGW90);
          SetWriteHandler(a+(1^t),a+(1^t),VRC6PSGW91);
          SetWriteHandler(a+(2^t),a+(2^t),VRC6PSGW92);

          a=0xa000+x;
          SetWriteHandler(a,a,VRC6PSGWA0);
          SetWriteHandler(a+(1^t),a+(1^t),VRC6PSGWA1);
          SetWriteHandler(a+(2^t),a+(2^t),VRC6PSGWA2);

          a=0xb000+x;
          SetWriteHandler(a,a,VRC6PSGWB0);
          SetWriteHandler(a+(1^t),a+(1^t),VRC6PSGWB1);
          SetWriteHandler(a+(2^t),a+(2^t),VRC6PSGWB2);
         }
}

void Mapper24_init(void)
{
        SetWriteHandler(0x8000,0xffff,Mapper24_write);
        if(FSettings.SndRate)
         VRC6_ESI(0);
        GameExpSound.RChange=VRC6SoundC;
        MapIRQHook=KonamiIRQHook;
	swaparoo=0;
}

void Mapper26_init(void)
{
        SetWriteHandler(0x8000,0xffff,Mapper24_write);
	if(FSettings.SndRate)
         VRC6_ESI(3);
	GameExpSound.RChange=VRC6SoundC;
        MapIRQHook=KonamiIRQHook;
	swaparoo=1;
}

