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

/*  TODO: Add (better) file io error checking */
/*  TODO: Change save state file format. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef GP2X
#include <unistd.h>
#endif

#include "types.h"
#include "x6502.h"
#include "version.h"
#include "fce.h"
#include "sound.h"
#define INESPRIV                // Take this out when old save state support is removed in a future version.
#include "ines.h"
#include "svga.h"
#include "endian.h"
#include "fds.h"
#include "general.h"
#include "state.h"
#include "memory.h"
#include "ppu.h"

static void (*SPreSave)(void) = 0;
static void (*SPostSave)(void) = 0;

#define SFMDATA_SIZE (64)
static SFORMAT SFMDATA[SFMDATA_SIZE];
static int SFEXINDEX;
static int stateversion;

extern SFORMAT FCEUPPU_STATEINFO[];  // 3
extern SFORMAT FCEUCTRL_STATEINFO[]; // 4

SFORMAT SFCPU[]={ // 1
 { &X.PC, 2|RLSB, "PC\0"},
 { &X.A, 1, "A\0\0"},
 { &X.P, 1, "P\0\0"},
 { &X.X, 1, "X\0\0"},
 { &X.Y, 1, "Y\0\0"},
 { &X.S, 1, "S\0\0"},
 { RAM, 0x800, "RAM"},
 { 0, }
};

SFORMAT SFCPUC[]={ // 2
 { &X.jammed, 1, "JAMM"},
 { &X.IRQlow, 1, "IRQL"},
 { &X.tcount, 4|RLSB, "ICoa"},
 { &X.count,  4|RLSB, "ICou"},
 { &timestamp, 4|RLSB, "TIME"},
 { &timestampbase, 8|RLSB, "TMEB"},
 // from 0.98.15
 { &timestampbase, sizeof(timestampbase) | RLSB, "TSBS"}, // size seems to match?
 { &X.mooPI, 1, "MooP"}, // alternative to the "quick and dirty hack"
 // TODO: IQLB?
 { 0, }
};

extern uint16 TempAddrT,RefreshAddrT;


SFORMAT SFSND[]={
 { &fhcnt, 4|RLSB,"FHCN"},
 { &fcnt, 1, "FCNT"},
 { PSG, 14, "PSG"},
 { &PSG[0x15], 1, "P15"},
 { &PSG[0x17], 1, "P17"},
 { decvolume, 3, "DECV"},
 { &sqnon, 1, "SQNO"},
 { &nreg, 2|RLSB, "NREG"},
 { &trimode, 1, "TRIM"},
 { &tricoop, 1, "TRIC"},
 { sweepon, 2, "SWEE"},
 { &curfreq[0], 4|RLSB,"CRF1"},
 { &curfreq[1], 4|RLSB,"CRF2"},
 { SweepCount, 2,"SWCT"},
 { DecCountTo1, 3,"DCT1"},
 { &PCMBitIndex, 1,"PBIN"},
 { &PCMAddressIndex, 4|RLSB, "PAIN"},
 { &PCMSizeIndex, 4|RLSB, "PSIN"},
 { 0, }
};



int WriteStateChunk(FILE *st, int type, SFORMAT *sf)
{
 int bsize, count;
 int x;

 count = x = 0;
 while (sf[x++].v) count++;

 fputc(type,st);

 for(x=bsize=0;x<count;x++)
  bsize+=sf[x].s&(~RLSB);
 bsize+=count<<3;
 write32(bsize,st);
 for(x=0;x<count;x++)
 {
  fwrite(sf[x].desc,1,4,st);
  write32(sf[x].s&(~RLSB),st);
  #ifdef LSB_FIRST
  fwrite((uint8 *)sf[x].v,1,sf[x].s&(~RLSB),st);
  #else
  {
  int z;
  if(sf[x].s&RLSB)
  {
   for(z=(sf[x].s&(~RLSB))-1;z>=0;z--)
   {
    fputc(*(uint8*)sf[x].v,st);
   }
  }
  else
   fwrite((uint8 *)sf[x].v,1,sf[x].s&(~RLSB),st);
  }
  #endif
 }
 return (bsize+5);
}

int ReadStateChunk(FILE *st, SFORMAT *sf, int size)
{
 uint8 tmpyo[16];
 int bsize, count;
 int x;

 // recalculate count ourselves
 count = x = 0;
 while (sf[x++].v) count++;

 for(x=bsize=0;x<count;x++)
  bsize+=sf[x].s&(~RLSB);
 if(stateversion>=53)
  bsize+=count<<3;
 else
 {
  if(bsize!=size)
  {
   fseek(st,size,SEEK_CUR);
   return 0;
  }
 }

 if(stateversion<56)
  memcpy(tmpyo,mapbyte3,16);

 if(stateversion>=53)
 {
  int temp;
  temp=ftell(st);

  while(ftell(st)<temp+size)
  {
   int tsize;
   char toa[4];

   if(fread(toa,1,4,st)<=0)
    return 0;
   read32(&tsize,st);

   for(x=0;x<count;x++)
   {
    if(!memcmp(toa,sf[x].desc,4))
    {
     if(tsize!=(sf[x].s&(~RLSB)))
     {
      printf("ReadStateChunk: sect \"%c%c%c%c\" has wrong size\n", toa[0], toa[1], toa[2], toa[3]);
      goto nkayo;
     }
     #ifndef LSB_FIRST
     if(sf[x].s&RLSB)
     {
      int z;
       for(z=(sf[x].s&(~RLSB))-1;z>=0;z--)
        *(uint8*)sf[x].v=fgetc(st);
     }
     else
     #endif
     {
       fread((uint8 *)sf[x].v,1,sf[x].s&(~RLSB),st);
     }
     goto bloo;
    }
   }
  printf("ReadStateChunk: sect \"%c%c%c%c\" not handled\n", toa[0], toa[1], toa[2], toa[3]);
  nkayo:
  fseek(st,tsize,SEEK_CUR);
  bloo:;
  } // while(...)
 }  // >=53
 else
 {
  for(x=0;x<count;x++)
  {
   #ifdef LSB_FIRST
   fread((uint8 *)sf[x].v,1,sf[x].s&(~RLSB),st);
   #else
   int z;
   if(sf[x].s&RLSB)
    for(z=(sf[x].s&(~RLSB))-1;z>=0;z--)
    {
     *(uint8*)sf[x].v=fgetc(st);
    }
   else
    fread((uint8 *)sf[x].v,1,sf[x].s&(~RLSB),st);
   #endif
  }
 }
 if(stateversion<56)
 {
  for(x=0;x<16;x++)
   #ifdef LSB_FIRST
   mapbyte1[x]=mapbyte1[x<<1];
   #else
   mapbyte1[x]=mapbyte1[(x<<1)+1];
   #endif
  memcpy(mapbyte3,tmpyo,16);
 }
 return 1;
}

static int ReadStateChunks(FILE *st)
{
 int t;
 uint32 size;
 int ret=1;

for(;;)
 {
  t=fgetc(st);
  if(t==EOF) break;
  if(!read32(&size,st)) break;

  // printf("ReadStateChunks: chunk %i\n", t);
  switch(t)
  {
   case 1:if(!ReadStateChunk(st,SFCPU,size)) ret=0;
#ifdef ASM_6502
          asmcpu_unpack();
#endif
	  break;
   case 2:if(!ReadStateChunk(st,SFCPUC,size)) ret=0;
          else
	  {
	   X.mooPI=X.P;	// Quick and dirty hack.
	  }
	  break;
   case 3:if(!ReadStateChunk(st,FCEUPPU_STATEINFO,size)) ret=0;break;
   case 4:if(!ReadStateChunk(st,FCEUCTRL_STATEINFO,size)) ret=0;break;
   case 5:if(!ReadStateChunk(st,SFSND,size)) ret=0;break;
   case 0x10:if(!ReadStateChunk(st,SFMDATA,size)) ret=0;break;
   default:printf("ReadStateChunks: unknown chunk: %i\n", t);
           if(fseek(st,size,SEEK_CUR)<0) goto endo;break;
  }
 }
 endo:
 return ret;
}


int CurrentState=0;
extern int geniestage;
void SaveState(void)
{
	FILE *st=NULL;

	TempAddrT=TempAddr;
	RefreshAddrT=RefreshAddr;

	if(geniestage==1)
	{
	 FCEU_DispMessage("Cannot save FCS in GG screen.");
	 return;
        }

	 st=fopen(FCEU_MakeFName(FCEUMKF_STATE,CurrentState,0),"wb");

	 if(st!=NULL)
	 {
	  static uint32 totalsize;
	  static uint8 header[16]="FCS";
	  memset(header+4,0,13);
	  header[3]=VERSION_NUMERIC;
	  fwrite(header,1,16,st);

#ifdef ASM_6502
          asmcpu_pack();
#endif
	  totalsize=WriteStateChunk(st,1,SFCPU);
	  totalsize+=WriteStateChunk(st,2,SFCPUC);
	  totalsize+=WriteStateChunk(st,3,FCEUPPU_STATEINFO);
	  totalsize+=WriteStateChunk(st,4,FCEUCTRL_STATEINFO);
	  totalsize+=WriteStateChunk(st,5,SFSND);


	  if(SPreSave) SPreSave();
	  totalsize+=WriteStateChunk(st,0x10,SFMDATA);
	  if(SPostSave) SPostSave();

	  fseek(st,4,SEEK_SET);
	  write32(totalsize,st);
	  SaveStateStatus[CurrentState]=1;
	  fclose(st);
#ifdef GP2X
	  sync();
#endif
	  FCEU_DispMessage("State %d saved.",CurrentState);
	 }
	 else
	  FCEU_DispMessage("State %d save error.",CurrentState);
}

static int LoadStateOld(FILE *st);
int FCEUSS_LoadFP(FILE *st, int make_backup)
{
	int x;
	if(st!=NULL)
	{
	 uint8 header[16];

         fread(&header,1,16,st);
         if(memcmp(header,"FCS",3))
         {
          fseek(st,0,SEEK_SET);
          if(!LoadStateOld(st))
           goto lerror;
          goto okload;
         }
         stateversion=header[3];
	 if(stateversion<53)
	 FixOldSaveStateSFreq();
	 x=ReadStateChunks(st);
	 if(GameStateRestore) GameStateRestore(header[3]);
	 if(x)
	 {
	  okload:
          TempAddr=TempAddrT;
          RefreshAddr=RefreshAddrT;

	  SaveStateStatus[CurrentState]=1;
	  FCEU_DispMessage("State %d loaded.",CurrentState);
	  SaveStateStatus[CurrentState]=1;
	 }
	 else
	 {
	  SaveStateStatus[CurrentState]=1;
	  FCEU_DispMessage("Error(s) reading state %d!",CurrentState);
	 }
	}
	else
	{
	 lerror:
	 FCEU_DispMessage("State %d load error.",CurrentState);
	 SaveStateStatus[CurrentState]=0;
	 return 0;
	}
	return 1;
}

void LoadState(void)
{
	FILE *st=NULL;

        if(geniestage==1)
        {
         FCEU_DispMessage("Cannot load FCS in GG screen.");
         return;
        }

	st=fopen(FCEU_MakeFName(FCEUMKF_STATE,CurrentState,0),"rb");
	if (st)
	{
	 FCEUSS_LoadFP(st, 0);
	 fclose(st);
	}
}

char SaveStateStatus[10];
void CheckStates(void)
{
	FILE *st=NULL;
	int ssel;

	if(SaveStateStatus[0]==(char)-1)
 	 for(ssel=0;ssel<10;ssel++)
	 {
	  st=fopen(FCEU_MakeFName(FCEUMKF_STATE,ssel,0),"rb");
          if(st)
          {
           SaveStateStatus[ssel]=1;
           fclose(st);
          }
          else
           SaveStateStatus[ssel]=0;
	 }
}

void SaveStateRefresh(void)
{
 SaveStateStatus[0]=-1;
}

void ResetExState(void (*PreSave)(void), void (*PostSave)(void))
{
 int x;
 for(x=0;x<SFEXINDEX;x++)
 {
  if(SFMDATA[x].desc)
   free(SFMDATA[x].desc);
 }
 SPreSave = PreSave;
 SPostSave = PostSave;
 SFEXINDEX=0;
}


void AddExState(void *v, uint32 s, int type, char *desc)
{
 if(desc)
 {
  SFMDATA[SFEXINDEX].desc=(char *)FCEU_malloc(5);
  strcpy(SFMDATA[SFEXINDEX].desc,desc);
 }
 else
//  SFMDATA[SFEXINDEX].desc=0;
  return; // do not support recursive save structures
 SFMDATA[SFEXINDEX].v=v;
 SFMDATA[SFEXINDEX].s=s;
 if(type) SFMDATA[SFEXINDEX].s|=RLSB;
 if(SFEXINDEX<SFMDATA_SIZE-1)
	 SFEXINDEX++;
 else
 {
	 static int once=1;
	 if(once)
	 {
		 once=0;
		 FCEU_PrintError("Error in AddExState: SFEXINDEX overflow.\nSomebody made SFMDATA_SIZE too small.");
	 }
 }
 SFMDATA[SFEXINDEX].v=0;		// End marker.
}


/* Old state loading code follows */

uint8 *StateBuffer;
unsigned int intostate;

static void afread(void *ptr, size_t _size, size_t _nelem)
{
	memcpy(ptr,StateBuffer+intostate,_size*_nelem);
	intostate+=_size*_nelem;
}


static void areadlower8of16(int8 *d)
{
#ifdef LSB_FIRST
	*d=StateBuffer[intostate++];
#else
	d[1]=StateBuffer[intostate++];
#endif
}


static void areadupper8of16(int8 *d)
{
#ifdef LSB_FIRST
	d[1]=StateBuffer[intostate++];
#else
	*d=StateBuffer[intostate++];
#endif
}


static void aread16(int8 *d)
{
#ifdef LSB_FIRST
	*d=StateBuffer[intostate++];
	d[1]=StateBuffer[intostate++];
#else
	d[1]=StateBuffer[intostate++];
	*d=StateBuffer[intostate++];
#endif
}


static void aread32(int8 *d)
{
#ifdef LSB_FIRST
	*d=StateBuffer[intostate++];
	d[1]=StateBuffer[intostate++];
	d[2]=StateBuffer[intostate++];
	d[3]=StateBuffer[intostate++];
#else
	d[3]=StateBuffer[intostate++];
	d[2]=StateBuffer[intostate++];
	d[1]=StateBuffer[intostate++];
	*d=StateBuffer[intostate++];
#endif
}

static int LoadStateOld(FILE *st)
{
	int x;
	int32 nada;
        uint8 version;
	nada=0;

	printf("LoadStateOld\n");

	StateBuffer=FCEU_malloc(59999);
	if(StateBuffer==NULL)
         return 0;
        if(!fread(StateBuffer,59999,1,st))
        {
            fclose(st);
            free(StateBuffer);
            return 0;
        }

	intostate=0;

	{
	 uint8 a[2];
 	 afread(&a[0],1,1);
	 afread(&a[1],1,1);
	 X.PC=a[0]|(a[1]<<8);
	}
	afread(&X.A,1,1);
	afread(&X.P,1,1);
	afread(&X.X,1,1);
	afread(&X.Y,1,1);
	afread(&X.S,1,1);
	afread(&version,1,1);
	afread(&nada,1,1);
	afread(&nada,1,1);
	afread(&nada,1,1);
	afread(&nada,1,1);
	aread32((int8 *)&X.count);
	afread(&nada,1,1);
	afread(&nada,1,1);
	afread(&nada,1,1);
	afread(&nada,1,1);
	aread32((int8 *)&nada);
	afread(&nada,1,1);
	afread(&nada,1,1);
	afread(&nada,1,1);
	afread(&nada,1,1);

	for(x=0;x<8;x++)
		areadupper8of16((int8 *)&CHRBankList[x]);
	afread(PRGBankList,4,1);
	for(x=0;x<8;x++)
		areadlower8of16((int8 *)&CHRBankList[x]);
        afread(CHRRAM,1,0x2000);
        afread(NTARAM,1,0x400);
        afread(ExtraNTARAM,1,0x400);
        afread(NTARAM+0x400,1,0x400);
        afread(ExtraNTARAM+0x400,1,0x400);

        for(x=0;x<0xF00;x++)
         afread(&nada,1,1);
        afread(PALRAM,1,0x20);
        for(x=0;x<256-32;x++)
         afread(&nada,1,1);
        for(x=0x00;x<0x20;x++)
         PALRAM[x]&=0x3f;
	afread(PPU,1,4);
	afread(SPRAM,1,0x100);
	afread(WRAM,1,8192);
	afread(RAM,1,0x800);
	aread16((int8 *)&scanline);
	aread16((int8 *)&RefreshAddr);
	afread(&VRAMBuffer,1,1);

	afread(&IRQa,1,1);
	aread32((int8 *)&IRQCount);
	aread32((int8 *)&IRQLatch);
	afread(&Mirroring,1,1);
	afread(PSG,1,0x17);
	PSG[0x11]&=0x7F;
        afread(MapperExRAM,1,193);
        if(version>=31)
         PSG[0x17]=MapperExRAM[115];
        else
         PSG[0x17]|=0x40;
        PSG[0x15]&=0xF;
        sqnon=PSG[0x15];

        X.IRQlow=0;
        afread(&nada,1,1);
        afread(&nada,1,1);
        afread(&nada,1,1);
        afread(&nada,1,1);
        afread(&nada,1,1);
        afread(&nada,1,1);
	afread(&XOffset,1,1);
        PPUCHRRAM=0;
        for(x=0;x<8;x++)
        {
         nada=0;
         afread(&nada,1,1);
         PPUCHRRAM|=(nada?1:0)<<x;
        }

         afread(mapbyte1,1,8);
         afread(mapbyte2,1,8);
         afread(mapbyte3,1,8);
         afread(mapbyte4,1,8);
         for(x=0;x<4;x++)
          aread16((int8 *)&nada);

         PPUNTARAM=0;
         for(x=0;x<4;x++)
         {
          nada=0;
          aread16((int8 *)&nada);
          PPUNTARAM|=((nada&0x800)?0:1)<<x;
         }
         afread(MapperExRAM,1,32768);
         afread(&vtoggle,1,1);
         aread16((int8 *)&TempAddrT);
         aread16((int8 *)&RefreshAddrT);

         if(GameStateRestore) GameStateRestore(version);
         free(StateBuffer);
	 FixOldSaveStateSFreq();
	 X.mooPI=X.P;
         return 1;
}

