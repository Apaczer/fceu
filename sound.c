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

/********************************************************/
/*******		sound.c				*/
/*******						*/
/*******  Sound emulation code and waveform synthesis 	*/
/*******  routines.  A few ideas were inspired		*/
/*******  by code from Marat Fayzullin's EMUlib		*/
/*******						*/
/********************************************************/

#include <stdlib.h>
#include <stdio.h>

#include <string.h>

#include "types.h"
#include "x6502.h"

#include "fce.h"
#include "svga.h"
#include "sound.h"
#include "state.h"

uint32 Wave[2048+512];
int32 WaveHi[40000]; // unused
int16 WaveFinalMono[2048+512];

EXPSOUND GameExpSound={0,0,0,0,0,0};

uint8 trimode=0;
uint8 tricoop=0;

static uint8 IRQFrameMode=0;	/* $4017 / xx000000 */
uint8 PSG[0x18];
static uint8 RawDALatch=0;	/* $4011 0xxxxxxx */

uint8 EnabledChannels=0;		/* Byte written to $4015 */

uint8 decvolume[3];
uint8 realvolume[3];

static int32 count[5];
static int32 sqacc[2]={0,0};
uint8 sqnon=0;

uint32 soundtsoffs=0;

#undef printf
uint16 nreg;

static int32 lengthcount[4];

extern int soundvol;

static const uint8 Slengthtable[0x20]=
{
 0x5,0x7f,0xA,0x1,0x14,0x2,0x28,0x3,0x50,0x4,0x1E,0x5,0x7,0x6,0x0E,0x7,
 0x6,0x08,0xC,0x9,0x18,0xa,0x30,0xb,0x60,0xc,0x24,0xd,0x8,0xe,0x10,0xf
};

static uint32 lengthtable[0x20];

static const uint32 SNoiseFreqTable[0x10]=
{
 2,4,8,0x10,0x20,0x30,0x40,0x50,0x65,0x7f,0xbe,0xfe,0x17d,0x1fc,0x3f9,0x7f2
};
static uint32 NoiseFreqTable[0x10];

int32 nesincsize;
uint32 soundtsinc;
uint32 soundtsi;


static const uint8 NTSCPCMTable[0x10]=
{
 0xd6,0xbe,0xaa,0xa0,0x8f,0x7f,0x71,0x6b,
 0x5f,0x50,0x47,0x40,0x35,0x2a,0x24,0x1b
};

static const uint8 PALPCMTable[0x10]=	// These values are just guessed.
{
 0xc6,0xb0,0x9d,0x94,0x84,0x75,0x68,0x63,
 0x58,0x4a,0x41,0x3b,0x31,0x27,0x21,0x19
};

static const uint32 NTSCDMCTable[0x10]=
{
 428,380,340,320,286,254,226,214,
 190,160,142,128,106, 84 ,72,54
};

/* Previous values for PAL DMC was value - 1,
 * I am not certain if this is if FCEU handled
 * PAL differently or not, the NTSC values are right,
 * so I am assuming that the current value is handled
 * the same way NTSC is handled. */

static const uint32 PALDMCTable[0x10]=
{
	398, 354, 316, 298, 276, 236, 210, 198,
	176, 148, 132, 118,  98,  78,  66,  50
};

// $4010        -        Frequency
// $4011        -        Actual data outputted
// $4012        -        Address register: $c000 + V*64
// $4013        -        Size register:  Size in bytes = (V+1)*64

int32 DMCacc=1;
int32 DMCPeriod=0;
uint8 DMCBitCount=0;

uint8 DMCAddressLatch=0,DMCSizeLatch=0; /* writes to 4012 and 4013 */
uint8 DMCFormat=0;	/* Write to $4010 */

static uint32 DMCAddress=0;
static int32 DMCSize=0;
static uint8 DMCShift=0;
static uint8 SIRQStat=0;

static char DMCHaveDMA=0;
static uint8 DMCDMABuf=0;
char DMCHaveSample=0;

uint32 PSG_base;

static void Dummyfunc(int end) {};

static void (*DoNoise)(int end)=Dummyfunc;
static void (*DoTriangle)(int end)=Dummyfunc;
static void (*DoPCM)(int end)=Dummyfunc;
static void (*DoSQ1)(int end)=Dummyfunc;
static void (*DoSQ2)(int end)=Dummyfunc;

uint8 sweepon[2]={0,0};
int32 curfreq[2]={0,0};

uint8 SweepCount[2];
uint8 DecCountTo1[3];

uint8 fcnt=0;
int32 fhcnt=0;
int32 fhinc;

static uint8 laster;

/* Instantaneous?  Maybe the new freq value is being calculated all of the time... */
static int FASTAPASS(2) CheckFreq(uint32 cf, uint8 sr)
{
 uint32 mod;
 if(!(sr&0x8))
 {
  mod=cf>>(sr&7);
  if((mod+cf)&0x800)
   return(0);
 }
 return(1);
}

static uint8 DutyCount[2]={0,0};

static void LoadDMCPeriod(uint8 V)
{
 if(PAL)
  DMCPeriod=PALDMCTable[V];
 else
  DMCPeriod=NTSCDMCTable[V];
}

static void PrepDPCM()
{
 DMCAddress=0x4000+(DMCAddressLatch<<6);
 DMCSize=(DMCSizeLatch<<4)+1;
}

static void SQReload(int x, uint8 V)
{
           if(EnabledChannels&(1<<x))
           {
            if(x)
             DoSQ2(0);
            else
             DoSQ1(0);
            lengthcount[x]=lengthtable[(V>>3)&0x1f];
            sqnon|=1<<x;
	   }

           sweepon[x]=PSG[(x<<2)|1]&0x80;
           curfreq[x]=PSG[(x<<2)|0x2]|((V&7)<<8);
	   decvolume[x]=0xF;
           SweepCount[x]=((PSG[(x<<2)|0x1]>>4)&7)+1;
	   DutyCount[x]=0;
           sqacc[x]=((int32)curfreq[0]+1)<<18;

           //RectDutyCount[x]=7;
	   //EnvUnits[x].reloaddec=1;
	   //reloadfreq[x]=1;
}

static DECLFW(Write_PSG)
{
 //if((A>=0x4004 && A<=0x4007) || A==0x4015)
  //printf("$%04x:$%02x, %d\n",A,V,SOUNDTS);
 A&=0x1f;
 switch(A)
 {
  case 0x0:
           DoSQ1(0);
           if(V&0x10)
            realvolume[0]=V&0xF;
           break;
  case 0x1:
           sweepon[0]=V&0x80;
           break;
  case 0x2:
           DoSQ1(0);
           curfreq[0]&=0xFF00;
           curfreq[0]|=V;
           break;
  case 0x3:
           SQReload(0,V);
           break;

  case 0x4:
	   DoSQ2(0);
           if(V&0x10)
            realvolume[1]=V&0xF;
	   break;
  case 0x5:
          sweepon[1]=V&0x80;
          break;
  case 0x6:
          DoSQ2(0);
          curfreq[1]&=0xFF00;
          curfreq[1]|=V;
          break;
  case 0x7:
          SQReload(1,V);
          break;
  case 0x8:
          DoTriangle(0);
	  if(laster&0x80)
	  {
            tricoop=V&0x7F;
            trimode=V&0x80;
          }
          if(!(V&0x7F))
           tricoop=0;
          laster=V&0x80;
          break;
  case 0xa:DoTriangle(0);
	   break;
  case 0xb:
	  if(EnabledChannels&0x4)
	  {
	   DoTriangle(0);
           sqnon|=4;
           lengthcount[2]=lengthtable[(V>>3)&0x1f];
	  }
          laster=0x80;
          tricoop=PSG[0x8]&0x7f;
          trimode=PSG[0x8]&0x80;
          break;
  case 0xC:DoNoise(0);
           if(V&0x10)
            realvolume[2]=V&0xF;
           break;
  case 0xE:DoNoise(0);break;
  case 0xF:
           if(EnabledChannels&8)
           {
	    DoNoise(0);
            sqnon|=8;
	    lengthcount[3]=lengthtable[(V>>3)&0x1f];
	   }
           decvolume[2]=0xF;
	   DecCountTo1[2]=(PSG[0xC]&0xF)+1;
           break;
 }
 PSG[A]=V;
}

static DECLFW(Write_DMCRegs)
{
 A&=0xF;

 switch(A)
 {
  case 0x00:DoPCM(0);
            LoadDMCPeriod(V&0xF);

            if(SIRQStat&0x80)
            {
             if(!(V&0x80))
             {
              X6502_IRQEnd(FCEU_IQDPCM);
              SIRQStat&=~0x80;
             }
             else X6502_IRQBegin(FCEU_IQDPCM);
            }
	    DMCFormat=V;
	    break;
  case 0x01:DoPCM(0);
	    RawDALatch=V&0x7F;
	    break;
  case 0x02:DMCAddressLatch=V;break;
  case 0x03:DMCSizeLatch=V;break;
 }
}

static DECLFW(StatusWrite)
{
	int x;
        int32 end=(SOUNDTS<<16)/soundtsinc;
	int t=V^EnabledChannels;

            if(t&1)
             DoSQ1(end);
            if(t&2)
             DoSQ2(end);
            if(t&4)
             DoTriangle(end);
            if(t&8)
             DoNoise(end);
            if(t&0x10)
             DoPCM(end);
            sqnon&=V;
        for(x=0;x<4;x++)
         if(!(V&(1<<x))) lengthcount[x]=0;   /* Force length counters to 0. */

        if(V&0x10)
        {
         if(!DMCSize)
          PrepDPCM();
        }
	else
	{
	 DMCSize=0;
	}
	SIRQStat&=~0x80;
        X6502_IRQEnd(FCEU_IQDPCM);
	EnabledChannels=V&0x1F;
}

DECLFR(StatusRead)
{
   //int x;
   uint8 ret;

   ret=SIRQStat;

   //for(x=0;x<4;x++) ret|=lengthcount[x]?(1<<x):0;
   ret|=EnabledChannels&sqnon;
   if(DMCSize) ret|=0x10;

   #ifdef FCEUDEF_DEBUGGER
   if(!fceuindbg)
   #endif
   {
    SIRQStat&=~0x40;
    X6502_IRQEnd(FCEU_IQFCOUNT);
   }
   return ret;
}

static void FASTAPASS(1) FrameSoundStuff(int V)
{
 int P;
 uint32 end = (SOUNDTS<<16)/soundtsinc;

 DoSQ1(end);
 DoSQ2(end);
 DoNoise(end);

 switch((V&1))
 {
  case 1:       /* Envelope decay, linear counter, length counter, freq sweep */
        if(EnabledChannels&4 && sqnon&4)
         if(!(PSG[8]&0x80))
         {
          if(lengthcount[2]>0)
          {
            lengthcount[2]--;
            if(lengthcount[2]<=0)
             {
              DoTriangle(0);
              sqnon&=~4;
             }
           }
         }

        for(P=0;P<2;P++)
        {
         if(EnabledChannels&(P+1) && sqnon&(P+1))
 	 {
          if(!(PSG[P<<2]&0x20))
          {
           if(lengthcount[P]>0)
           {
            lengthcount[P]--;
            if(lengthcount[P]<=0)
             {
              sqnon&=~(P+1);
             }
           }
          }
	 }
		/* Frequency Sweep Code Here */
		/* xxxx 0000 */
		/* xxxx = hz.  120/(x+1)*/
	  if(sweepon[P])
          {
           int32 mod=0;

	   if(SweepCount[P]>0) SweepCount[P]--;
	   if(SweepCount[P]<=0)
	   {
	    SweepCount[P]=((PSG[(P<<2)+0x1]>>4)&7)+1; //+1;
            {
             if(PSG[(P<<2)+0x1]&0x8)
             {
              mod-=(P^1)+((curfreq[P])>>(PSG[(P<<2)+0x1]&7));

              if(curfreq[P] && (PSG[(P<<2)+0x1]&7)/* && sweepon[P]&0x80*/)
              {
               curfreq[P]+=mod;
              }
             }
             else
             {
              mod=curfreq[P]>>(PSG[(P<<2)+0x1]&7);
              if((mod+curfreq[P])&0x800)
              {
               sweepon[P]=0;
               curfreq[P]=0;
              }
              else
              {
               if(curfreq[P] && (PSG[(P<<2)+0x1]&7)/* && sweepon[P]&0x80*/)
               {
                curfreq[P]+=mod;
               }
              }
             }
            }
	   }
          }
         }

       if(EnabledChannels&0x8 && sqnon&8)
        {
         if(!(PSG[0xC]&0x20))
         {
          if(lengthcount[3]>0)
          {
           lengthcount[3]--;
           if(lengthcount[3]<=0)
           {
            sqnon&=~8;
           }
          }
         }
        }

  case 0:       /* Envelope decay + linear counter */
         if(!trimode)
         {
           laster=0;
           if(tricoop)
           {
            if(tricoop==1) DoTriangle(0);
            tricoop--;
           }
         }

        for(P=0;P<2;P++)
        {
	  if(DecCountTo1[P]>0) DecCountTo1[P]--;
          if(DecCountTo1[P]<=0)
          {
	   DecCountTo1[P]=(PSG[P<<2]&0xF)+1;
           if(decvolume[P] || PSG[P<<2]&0x20)
           {
            decvolume[P]--;
	    /* Step from 0 to full volume seems to take twice as long
	       as the other steps.  I don't know if this is the correct
	       way to double its length, though(or if it even matters).
	    */
            if((PSG[P<<2]&0x20) && (decvolume[P]==0))
             DecCountTo1[P]<<=1;
	    decvolume[P]&=15;
           }
          }
          if(!(PSG[P<<2]&0x10))
           realvolume[P]=decvolume[P];
        }

         if(DecCountTo1[2]>0) DecCountTo1[2]--;
         if(DecCountTo1[2]<=0)
         {
          DecCountTo1[2]=(PSG[0xC]&0xF)+1;
          if(decvolume[2] || PSG[0xC]&0x20)
          {
            decvolume[2]--;
            /* Step from 0 to full volume seems to take twice as long
               as the other steps.  I don't know if this is the correct
               way to double its length, though(or if it even matters).
            */
            if((PSG[0xC]&0x20) && (decvolume[2]==0))
             DecCountTo1[2]<<=1;
            decvolume[2]&=15;
          }
         }
         if(!(PSG[0xC]&0x10))
          realvolume[2]=decvolume[2];

        break;
 }

}

void FrameSoundUpdate(void)
{
 // Linear counter:  Bit 0-6 of $4008
 // Length counter:  Bit 4-7 of $4003, $4007, $400b, $400f

 if(!fcnt && !(IRQFrameMode&0x3))
 {
         SIRQStat|=0x40;
         X6502_IRQBegin(FCEU_IQFCOUNT);
 }

 if(fcnt==3)
 {
	if(IRQFrameMode&0x2)
	 fhcnt+=fhinc;
 }
 FrameSoundStuff(fcnt);
 fcnt=(fcnt+1)&3;
}

static INLINE void tester(void)
{
 if(DMCBitCount==0)
 {
  if(!DMCHaveDMA)
   DMCHaveSample=0;
  else
  {
   DMCHaveSample=1;
   DMCShift=DMCDMABuf;
   DMCHaveDMA=0;
  }
 }
}

static uint32 ChannelBC[5];

static uint32 RectAmp[2][8];

static void FASTAPASS(1) CalcRectAmp(int P)
{
  static int tal[4]={1,2,4,6};
  int V;
  int x;
  uint32 *b=RectAmp[P];
  int m;

  //if(PSG[P<<2]&0x10)
   V=realvolume[P]<<4;
  //V=(PSG[P<<2]&15)<<4;
  //else
  // V=decvolume[P]<<4;
  m=tal[(PSG[P<<2]&0xC0)>>6];
  for(x=0;x<m;x++,b++)
   *b=0;
  for(;x<8;x++,b++)
   *b=V;
}

static INLINE void DMCDMA(void)
{
  if(DMCSize && !DMCHaveDMA)
  {
#if 0
   X6502_DMR(0x8000+DMCAddress);
   X6502_DMR(0x8000+DMCAddress);
   X6502_DMR(0x8000+DMCAddress);
   DMCDMABuf=X6502_DMR(0x8000+DMCAddress);
#else
   X6502_AddCycles(4);
   DMCDMABuf=X.DB=ARead[0x8000+DMCAddress](0x8000+DMCAddress);
#endif
   DMCHaveDMA=1;
   DMCAddress=(DMCAddress+1)&0x7fff;
   DMCSize--;
   if(!DMCSize)
   {
    if(DMCFormat&0x40)
     PrepDPCM();
    else
    {
     SIRQStat|=0x80;
     if(DMCFormat&0x80)
      X6502_IRQBegin(FCEU_IQDPCM);
    }
   }
 }
}

void FCEU_SoundCPUHook(int cycles48)
{
 fhcnt-=cycles48;
 if(fhcnt<=0)
 {
  FrameSoundUpdate();
  fhcnt+=fhinc;
 }

 DMCDMA();
 DMCacc-=cycles48/48;

 while(DMCacc<=0)
 {
  if(DMCHaveSample)
  {
   uint8 bah=RawDALatch;
   int t=((DMCShift&1)<<2)-2;

   /* Unbelievably ugly hack */
   if(FSettings.SndRate)
   {
    soundtsoffs+=DMCacc;
    DoPCM(0);
    soundtsoffs-=DMCacc;
   }
   RawDALatch+=t;
   if(RawDALatch&0x80)
    RawDALatch=bah;
  }

  DMCacc+=DMCPeriod;
  DMCBitCount=(DMCBitCount+1)&7;
  DMCShift>>=1;
  tester();
 }
}

static void RDoPCM(int32 end)
{
 int32 V;
 int32 start;

 start=ChannelBC[4];
 if(end==0) end=(SOUNDTS<<16)/soundtsinc;
 if(end<=start) return;
 ChannelBC[4]=end;

 for(V=start;V<end;V++)
  //WaveHi[V]+=(((RawDALatch<<16)/256) * FSettings.PCMVolume)&(~0xFFFF); // TODO get rid of floating calculations to binary. set log volume scaling.
  Wave[V>>4]+=RawDALatch<<3;
}

static void RDoSQ1(int32 end)
{
   int32 V;
   int32 start;
   int32 freq;

   start=ChannelBC[0];
   if(end==0) end=(SOUNDTS<<16)/soundtsinc;
   if(end<=start) return;
   ChannelBC[0]=end;

   if(!(EnabledChannels&1 && sqnon&1))
    return;

   if(curfreq[0]<8 || curfreq[0]>0x7ff)
    return;
   if(!CheckFreq(curfreq[0],PSG[0x1]))
    return;

   CalcRectAmp(0);

   {
    uint32 out=RectAmp[0][DutyCount[0]];
    freq=curfreq[0]+1;
    {
      freq<<=18;
      for(V=start;V<end;V++)
      {
       Wave[V>>4]+=out;
       sqacc[0]-=nesincsize;
       if(sqacc[0]<=0)
       {
        rea:
        sqacc[0]+=freq;
        DutyCount[0]++;
        if(sqacc[0]<=0) goto rea;

        DutyCount[0]&=7;
        out=RectAmp[0][DutyCount[0]];
       }
      }
    }
   }
}

static void RDoSQ2(int32 end)
{
   int32 V;
   int32 start;
   int32 freq;

   start=ChannelBC[1];
   if(end==0) end=(SOUNDTS<<16)/soundtsinc;
   if(end<=start) return;
   ChannelBC[1]=end;

   if(!(EnabledChannels&2 && sqnon&2))
    return;

   if(curfreq[1]<8 || curfreq[1]>0x7ff)
    return;
   if(!CheckFreq(curfreq[1],PSG[0x5]))
    return;

   CalcRectAmp(1);

   {
    uint32 out=RectAmp[1][DutyCount[1]];
    freq=curfreq[1]+1;

    {
      freq<<=18;
      for(V=start;V<end;V++)
      {
       Wave[V>>4]+=out;
       sqacc[1]-=nesincsize;
       if(sqacc[1]<=0)
       {
        rea:
        sqacc[1]+=freq;
        DutyCount[1]++;
	if(sqacc[1]<=0) goto rea;

        DutyCount[1]&=7;
	out=RectAmp[1][DutyCount[1]];
       }
      }
    }
   }
}


static void RDoTriangle(int32 end)
{
   static uint32 tcout=0;
   int32 V;
   int32 start; //,freq;
   int32 freq=(((PSG[0xa]|((PSG[0xb]&7)<<8))+1));

   start=ChannelBC[2];
   if(end==0) end=(SOUNDTS<<16)/soundtsinc;
   if(end<=start) return;
   ChannelBC[2]=end;

    if(! (EnabledChannels&0x4 && sqnon&4 && tricoop) )
    {	// Counter is halted, but we still need to output.
     for(V=start;V<end;V++)
       Wave[V>>4]+=tcout;
    }
    else if(freq<=4) // 55.9Khz - Might be barely audible on a real NES, but
 	       // it's too costly to generate audio at this high of a frequency
 	       // (55.9Khz * 32 for the stepping).
 	       // The same could probably be said for ~27.8Khz, so we'll
 	       // take care of that too.  We'll just output the average
 	       // value(15/2 - scaled properly for our output format, of course).
	       // We'll also take care of ~18Khz and ~14Khz too, since they should be barely audible.
	       // (Some proof or anything to confirm/disprove this would be nice.).
    {
     for(V=start;V<end;V++)
      Wave[V>>4]+=((0xF<<4)+(0xF<<2))>>1;
    }
    else
    {
     static int32 triacc=0;
     static uint8 tc=0;

      freq<<=17;
      for(V=start;V<end;V++)
      {
       triacc-=nesincsize;
       if(triacc<=0)
       {
        rea:
        triacc+=freq; //t;
        tc=(tc+1)&0x1F;
        if(triacc<=0) goto rea;

        tcout=(tc&0xF);
        if(tc&0x10) tcout^=0xF;
        tcout=(tcout<<4)+(tcout<<2);
       }
       Wave[V>>4]+=tcout;
      }
    }
}

static void RDoNoise(int32 end)
{
   int32 inc,V;
   int32 start;

   start=ChannelBC[3];
   if(end==0) end=(SOUNDTS<<16)/soundtsinc;
   if(end<=start) return;
   ChannelBC[3]=end;

   if(EnabledChannels&0x8 && sqnon&8)
   {
      uint32 outo;
      uint32 amptab[2];
      uint8 amplitude;

      amplitude=realvolume[2];
      //if(PSG[0xC]&0x10)
      // amplitude=(PSG[0xC]&0xF);
      //else
      // amplitude=decvolume[2]&0xF;

      inc=NoiseFreqTable[PSG[0xE]&0xF];
      amptab[0]=((amplitude<<2)+amplitude+amplitude)<<1;
      amptab[1]=0;
      outo=amptab[nreg&1];

      if(amplitude)
      {
       if(PSG[0xE]&0x80)	// "short" noise
        for(V=start;V<end;V++)
        {
         Wave[V>>4]+=outo;
         if(count[3]>=inc)
         {
          uint8 feedback;

          feedback=((nreg>>8)&1)^((nreg>>14)&1);
          nreg=(nreg<<1)+feedback;
          nreg&=0x7fff;
          outo=amptab[nreg&1];
          count[3]-=inc;
         }
         count[3]+=0x1000;
        }
       else
        for(V=start;V<end;V++)
        {
         Wave[V>>4]+=outo;
         if(count[3]>=inc)
         {
          uint8 feedback;

          feedback=((nreg>>13)&1)^((nreg>>14)&1);
          nreg=(nreg<<1)+feedback;
          nreg&=0x7fff;
          outo=amptab[nreg&1];
          count[3]-=inc;
         }
         count[3]+=0x1000;
        }
      }
   }
}

DECLFW(Write_IRQFM)
{
 V=(V&0xC0)>>6;
 fcnt=0;
 if(V&0x2)
  FrameSoundUpdate();
 fcnt=1;
 fhcnt=fhinc;
 X6502_IRQEnd(FCEU_IQFCOUNT);
 SIRQStat&=~0x40;
 IRQFrameMode=V;
}

void SetNESSoundMap(void)
{
  SetWriteHandler(0x4000,0x400F,Write_PSG);
  SetWriteHandler(0x4010,0x4013,Write_DMCRegs);
  SetWriteHandler(0x4017,0x4017,Write_IRQFM);

  SetWriteHandler(0x4015,0x4015,StatusWrite);
  SetReadHandler(0x4015,0x4015,StatusRead);
}

int32 highp;                   // 0 through 65536, 0 = no high pass, 65536 = max high pass

int32 lowp;                    // 0 through 65536, 65536 = max low pass(total attenuation)
				// 65536 = no low pass
static int32 flt_acc=0, flt_acc2=0;

static void FilterSound(uint32 *in, int16 *outMono, int count)
{
// static int min=0, max=0;
 int sh=2;
 if (soundvol < 5) sh += 5 - soundvol;

 for(;count;count--,in++,outMono++)
 {
  int32 diff;

  diff = *in - flt_acc;

  flt_acc += (diff*highp)>>16;
  flt_acc2+= (int32) (((int64)((diff-flt_acc2)*lowp))>>16);
  *in=0;

  *outMono = flt_acc2*7 >> sh; // * 7 >> 2 = * 1.75
//  if (acc2 < min) { printf("min: %i %04x\n", acc2, acc2); min = acc2; }
//  if (acc2 > max) { printf("max: %i %04x\n", acc2, acc2); max = acc2; }
 }
}



static int32 inbuf=0;
int FlushEmulateSound(void)
{
  int x;
  uint32 end;

  if(!timestamp) return(0);

  if(!FSettings.SndRate || (soundvol == 0))
  {
   end=0;
   goto nosoundo;
  }

  end=(SOUNDTS<<16)/soundtsinc;
  DoSQ1(end);
  DoSQ2(end);
  DoTriangle(end);
  DoNoise(end);
  DoPCM(end);

  if(GameExpSound.Fill)
   GameExpSound.Fill(end&0xF);

  FilterSound(Wave,WaveFinalMono,end>>4);

  if(end&0xF)
   Wave[0]=Wave[(end>>4)];
  Wave[(end>>4)]=0;

  nosoundo:
  for(x=0;x<5;x++)
   ChannelBC[x]=end&0xF;
  soundtsoffs=(soundtsinc*(end&0xF))>>16;
  end>>=4;
  inbuf=end;
  return(end);
}

int GetSoundBuffer(int16 **W)
{
 *W=WaveFinalMono;
 return inbuf;
}

void FCEUSND_Power(void)
{
	int x;

        SetNESSoundMap();
        memset(PSG,0x00,sizeof(PSG));
	FCEUSND_Reset();

	memset(Wave,0,sizeof(Wave));
        memset(WaveHi,0,sizeof(WaveHi));
	//memset(&EnvUnits,0,sizeof(EnvUnits));

        for(x=0;x<5;x++)
         ChannelBC[x]=0;
        soundtsoffs=0;
        LoadDMCPeriod(DMCFormat&0xF);
}

void FCEUSND_Reset(void)
{
        int x;
        for(x=0;x<0x16;x++)
         if(x!=1 && x!=5 && x!=0x14) BWrite[0x4000+x](0x4000+x,0);

	IRQFrameMode=0x0;
        fhcnt=fhinc;
        fcnt=0;
        nreg=1;

	DMCHaveDMA=DMCHaveSample=0;
	SIRQStat=0x00;

	RawDALatch=0x00;
	//TriCount=0;
	//TriMode=0;
	//tristep=0;
	EnabledChannels=0;
	for(x=0;x<4;x++)
	 lengthcount[x]=0;

	DMCAddressLatch=0;
	DMCSizeLatch=0;
	DMCFormat=0;
	DMCAddress=0;
	DMCSize=0;
	DMCShift=0;
	DMCacc=1;
	DMCBitCount=0;
}

void SetSoundVariables(void)
{
  int x;

  fhinc=PAL?16626:14915;	// *2 CPU clock rate
  fhinc*=24;
  for(x=0;x<0x20;x++)
   lengthtable[x]=Slengthtable[x]<<1;

  if(FSettings.SndRate)
  {
   DoNoise=RDoNoise;
   DoTriangle=RDoTriangle;
   DoPCM=RDoPCM;
   DoSQ1=RDoSQ1;
   DoSQ2=RDoSQ2;
  }
  else
  {
   DoNoise=DoTriangle=DoPCM=DoSQ1=DoSQ2=Dummyfunc;
  }

  if(!FSettings.SndRate) return;
  if(GameExpSound.RChange)
   GameExpSound.RChange();

  // nesincsizeLL=(int64)((int64)562949953421312*(double)(PAL?PAL_CPU:NTSC_CPU)/(FSettings.SndRate OVERSAMPLE));
  nesincsize=(int32)(((int64)1<<17)*(double)(PAL?PAL_CPU:NTSC_CPU)/(FSettings.SndRate * 16)); // 308845 - 1832727
  PSG_base=(uint32)(PAL?(long double)PAL_CPU/16:(long double)NTSC_CPU/16);

  for(x=0;x<0x10;x++)
  {
   long double z;
   z=SNoiseFreqTable[x]<<1;
   z=(PAL?PAL_CPU:NTSC_CPU)/z;
   z=(long double)((uint32)((FSettings.SndRate OVERSAMPLE)<<12))/z;
   NoiseFreqTable[x]=z;
  }
  soundtsinc=(uint32)((uint64)(PAL?(long double)PAL_CPU*65536:(long double)NTSC_CPU*65536)/(FSettings.SndRate OVERSAMPLE));
  memset(Wave,0,sizeof(Wave));
  for(x=0;x<5;x++)
   ChannelBC[x]=0;
  highp=(250<<16)/FSettings.SndRate;  // Arbitrary
  lowp=(25000<<16)/FSettings.SndRate; // Arbitrary

  if(highp>(1<<16)) highp=1<<16;
  if(lowp>(1<<16)) lowp=1<<16;

  flt_acc=flt_acc2=0;
}

void FixOldSaveStateSFreq(void)
{
        int x;
        for(x=0;x<2;x++)
        {
         curfreq[x]=PSG[0x2+(x<<2)]|((PSG[0x3+(x<<2)]&7)<<8);
        }
}

void FCEUI_Sound(int Rate)
{
 FSettings.SndRate=Rate;
 SetSoundVariables();
}

void FCEUI_SetSoundVolume(uint32 volume)
{
 FSettings.SoundVolume=volume;
}

SFORMAT FCEUSND_STATEINFO[]={
 { &fhcnt, 4|FCEUSTATE_RLSB,"FHCN"},
 { &fcnt, 1, "FCNT"},
 { PSG, 0x10, "PSG"},
 { &EnabledChannels, 1, "ENCH"},
 { &IRQFrameMode, 1, "IQFM"},

 { decvolume, 3, "DECV"},
 { &sqnon, 1, "SQNO"},
 { &nreg, 2|RLSB, "NREG"},
 { &trimode, 1, "TRIM"},
 { &tricoop, 1, "TRIC"},
 { DecCountTo1, 3,"DCT1"},

 { sweepon, 2, "SWEE"},
 { &curfreq[0], 4|FCEUSTATE_RLSB,"CRF1"},
 { &curfreq[1], 4|FCEUSTATE_RLSB,"CRF2"},
 { SweepCount, 2,"SWCT"},

 { &SIRQStat, 1, "SIRQ"},

 { &DMCacc, 4|FCEUSTATE_RLSB, "5ACC"},
 { &DMCBitCount, 1, "5BIT"},
 { &DMCAddress, 4|FCEUSTATE_RLSB, "5ADD"},
 { &DMCSize, 4|FCEUSTATE_RLSB, "5SIZ"},
 { &DMCShift, 1, "5SHF"},

 { &DMCHaveDMA, 1, "5HVDM"},
 { &DMCHaveSample, 1, "5HVSP"},

 { &DMCSizeLatch, 1, "5SZL"},
 { &DMCAddressLatch, 1, "5ADL"},
 { &DMCFormat, 1, "5FMT"},
 { &RawDALatch, 1, "RWDA"},
 { 0, }
};

void FCEUSND_SaveState(void)
{

}

void FCEUSND_LoadState(int version)
{
 LoadDMCPeriod(DMCFormat&0xF);
 RawDALatch&=0x7F;
 DMCAddress&=0x7FFF;
}
