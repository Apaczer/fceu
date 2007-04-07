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

#include "types.h"
#include "x6502.h"
#include "version.h"
#include "fce.h"
#include "fds.h"
#include "svga.h"
#include "sound.h"
#include "general.h"
#include "state.h"
#include "file.h"
#include "memory.h"
#include "cart.h"

/*	TODO:  Add code to put a delay in between the time a disk is inserted
	and the when it can be successfully read/written to.  This should
	prevent writes to wrong places OR add code to prevent disk ejects
	when the virtual motor is on(mmm...virtual motor).
*/

static DECLFR(FDSRead4030);
static DECLFR(FDSRead4031);
static DECLFR(FDSRead4032);
static DECLFR(FDSRead4033);
static DECLFW(FDSWrite4020);
static DECLFW(FDSWrite4021);
static DECLFW(FDSWrite4022);
static DECLFW(FDSWrite4023);
static DECLFW(FDSWrite4024);
static DECLFW(FDSWrite4025);
static DECLFW(FDSWaveWrite);
static DECLFR(FDSWaveRead);

static DECLFR(FDSSRead);
static DECLFW(FDSSWrite);
static DECLFR(FDSBIOSRead);
static DECLFR(FDSRAMRead);
static DECLFW(FDSRAMWrite);
static void FDSInit(void);
static void FDSReset(void);
static void FP_FASTAPASS(1) FDSFix(int a);

#define FDSRAM GameMemBlock
#define mapbyte1 (GameMemBlock+32768)
#define mapbyte2 (GameMemBlock+32768+8)
#define mapbyte3 (GameMemBlock+32768+16)
#define mapbyte4 (GameMemBlock+32768+24)        // 8 bytes
#define CHRRAM   (GameMemBlock+32768+32+64)

#define IRQLatch       (*(int32*)(CHRRAM+8192))
#define IRQCount       (*(int32*)(CHRRAM+8192+4))
#define IRQa           (*(CHRRAM+8192+8))

static void FDSClose(void);
static uint8 header[16];
#define writeskip mapbyte2[0]

static char FDSSaveName[2048];

uint8 FDSBIOS[8192];
uint8 *diskdata[4]={0,0,0,0};

#define SideWrite mapbyte2[1]
#define DiskPtr (*(uint32*)(mapbyte2+4))
#define dsr0 mapbyte2[2]
#define dsr1 mapbyte2[3]

#define DC_INC		1

#define DiskSeekIRQ (*(int32*)(mapbyte3+4))
#define SelectDisk mapbyte3[0]
#define InDisk	   mapbyte3[1]

static void FDSReset(void)
{
        memset(mapbyte1,0,8);
        memset(mapbyte2,0,8);
        memset(mapbyte3+4,0,4);
        memset(mapbyte4,0,8);
}

void FDSGI(int h)
{
 switch(h)
 {
  case GI_CLOSE: FDSClose();break;
  case GI_POWER: FDSReset();FDSInit();break;
 }
}

static void FDSStateRestore(int version)
{ 
 setmirror(((mapbyte1[5]&8)>>3)^1);
}

void FDSSound();
void FDSSoundReset(void);
void FDSSoundStateAdd(void);
static void RenderSound(void);

static void FDSInit(void)
{
 dsr0=0;
 dsr1=0x41;
 setmirror(1);
 if(InDisk!=255)
  dsr1&=0xFE;

 setprg8r(0,0xe000,0);          // BIOS
 setprg32r(1,0x6000,0);         // 32KB RAM
 setchr8(0);           // 8KB CHR RAM

 MapIRQHook=FDSFix;
 GameStateRestore=FDSStateRestore;

 SetReadHandler(0x4030,0x4030,FDSRead4030);
 SetReadHandler(0x4031,0x4031,FDSRead4031);
 SetReadHandler(0x4032,0x4032,FDSRead4032);
 SetReadHandler(0x4033,0x4033,FDSRead4033);

 SetWriteHandler(0x4020,0x4020,FDSWrite4020); 
 SetWriteHandler(0x4021,0x4021,FDSWrite4021);
 SetWriteHandler(0x4022,0x4022,FDSWrite4022);
 SetWriteHandler(0x4023,0x4023,FDSWrite4023);
 SetWriteHandler(0x4024,0x4024,FDSWrite4024);
 SetWriteHandler(0x4025,0x4025,FDSWrite4025);

 SetWriteHandler(0x6000,0xdfff,FDSRAMWrite);
 SetReadHandler(0x6000,0xdfff,FDSRAMRead);
 SetReadHandler(0xE000,0xFFFF,FDSBIOSRead);

 IRQCount=IRQLatch=IRQa=0;

 FDSSoundReset();
}

void FDSControl(int what)
{
 switch(what)
 {
  case FDS_IDISK:dsr1&=0xFE;
                 if(InDisk==255)
		 {
		  FCEU_DispMessage("Disk %d Side %s Inserted",
		  SelectDisk>>1,(SelectDisk&1)?"B":"A");
		  InDisk=SelectDisk;
		 }
		 else
		  FCEU_DispMessage("Jamming disks is a BAD IDEA");
		 break;
  case FDS_EJECT:
		 if(InDisk!=255)
	 	  FCEU_DispMessage("Disk Ejected");
		 else
		  FCEU_DispMessage("Cannot Eject Air");
		 dsr1|=1;
		 InDisk=255;
		 break;
  case FDS_SELECT:
		  if(InDisk!=255)
		  {
		   FCEU_DispMessage("Eject disk before selecting.");
		   break;
		  }
		  SelectDisk=((SelectDisk+1)%header[4])&3;
		  FCEU_DispMessage("Disk %d Side %s Selected",
			SelectDisk>>1,(SelectDisk&1)?"B":"A");
		  break;
 }
}

static void FP_FASTAPASS(1) FDSFix(int a)
{
 if(IRQa)
 {
  IRQCount-=a;
  if(IRQCount<=0)
  {
   IRQa=0;
   dsr0|=1;
   dsr0&=~2;
   IRQCount=0xFFFF;
   X6502_IRQBegin(FCEU_IQEXT);
  }
 }
 if(DiskSeekIRQ>0) 
 {
  DiskSeekIRQ-=a;
  if(DiskSeekIRQ<=0)
  {
   if(mapbyte1[5]&0x80)
   {
    dsr0&=~1;
    dsr0|=2;
    TriggerIRQ();
   }
  }
 }
}

void DiskControl(int which)
{
if(mapbyte1[5]&1)
 {
  switch(which)
  {
  case DC_INC:
	       if(DiskPtr<64999) DiskPtr++;
	       //DiskSeekIRQ=160+100;
	       //DiskSeekIRQ=140;
	       //DiskSeekIRQ=160;
		DiskSeekIRQ=150;
 	       break;
  }
 }
}
static DECLFR(FDSRead4030)
{
	X6502_IRQEnd(FCEU_IQEXT);
	return dsr0;
}

static DECLFR(FDSRead4031)
{
	static uint8 z=0;
	if(InDisk!=255)
	{
         z=diskdata[InDisk][DiskPtr];
         DiskControl(DC_INC);
	}
        return z;
}

static DECLFR(FDSRead4032)
{
	return dsr1;
}

static DECLFR(FDSRead4033)
{
	return 0x80; // battery
}

static DECLFW(FDSRAMWrite)
{
 (FDSRAM-0x6000)[A]=V;
}

static DECLFR(FDSBIOSRead)
{
 return (FDSBIOS-0xE000)[A];
}

static DECLFR(FDSRAMRead)
{
 return (FDSRAM-0x6000)[A];
}

/* Begin FDS sound */

#define FDSClock (1789772.7272727272727272/8)

typedef struct {
        int64 cycles;           // Cycles per PCM sample
        int64 count;		// Cycle counter
	int64 envcount;		// Envelope cycle counter
	uint32 b19shiftreg60;
	uint32 b24adder66;
	uint32 b24latch68;
	uint32 b17latch76;
        int32 clockcount;	// Counter to divide frequency by 8.
	uint8 b8shiftreg88;	// Modulation register.
	uint8 amplitude[2];	// Current amplitudes.
        uint8 mwave[0x20];      // Modulation waveform
        uint8 cwave[0x40];      // Game-defined waveform(carrier)
        uint8 SPSG[0xB];
} FDSSOUND;

static FDSSOUND fdso;

#define	SPSG	fdso.SPSG
#define b19shiftreg60	fdso.b19shiftreg60
#define b24adder66	fdso.b24adder66
#define b24latch68	fdso.b24latch68
#define b17latch76	fdso.b17latch76
#define b8shiftreg88	fdso.b8shiftreg88
#define clockcount	fdso.clockcount
#define amplitude	fdso.amplitude

void FDSSoundStateAdd(void)
{
 AddExState(fdso.cwave,64,0,"WAVE");
 AddExState(fdso.mwave,32,0,"MWAV");
 AddExState(amplitude,2,0,"AMPL");
 AddExState(SPSG,0xB,0,"SPSG");

 AddExState(&b8shiftreg88,1,0,"B88");

 AddExState(&clockcount, 4, 1, "CLOC");
 AddExState(&b19shiftreg60,4,1,"B60");
 AddExState(&b24adder66,4,1,"B66");
 AddExState(&b24latch68,4,1,"B68");
 AddExState(&b17latch76,4,1,"B76");

}

static DECLFR(FDSSRead)
{
 switch(A&0xF)
 {
  case 0x0:return(amplitude[0]|(X.DB&0xC0));
  case 0x2:return(amplitude[1]|(X.DB&0xC0));
 }
 return(X.DB);
}

static DECLFW(FDSSWrite)
{
 if(FSettings.SndRate)
  RenderSound();
 A-=0x4080;
 switch(A)
 {
  case 0x0: 
  case 0x4:
	    if(!(V&0x80))
	    {
 	      // if(V&0x40) amplitude[(A&0xF)>>2]=0;
	      // else amplitude[(A&0xF)>>2]=0x3F;
	    }
	    else 
	     amplitude[(A&0xF)>>2]=V&0x3F;
	    break;
  case 0x7: b17latch76=0;SPSG[0x5]=0;break;
  case 0x8:
	   //printf("%d:$%02x\n",SPSG[0x5],V);
	   fdso.mwave[SPSG[0x5]&0x1F]=V&0x7;
           SPSG[0x5]=(SPSG[0x5]+1)&0x1F;
	   break;
 }
 //if(A>=0x7 && A!=0x8 && A<=0xF)
 //if(A==0xA || A==0x9) printf("$%04x:$%02x\n",A,V);
 SPSG[A]=V;
}

// $4080 - Fundamental wave amplitude data register 92
// $4082 - Fundamental wave frequency data register 58
// $4083 - Same as $4082($4083 is the upper 4 bits).

// $4084 - Modulation amplitude data register 78
// $4086 - Modulation frequency data register 72
// $4087 - Same as $4086($4087 is the upper 4 bits)


static void DoEnv()
{
 int x;

 for(x=0;x<2;x++)
  if(!(SPSG[x<<2]&0x80) && !(SPSG[0x9+x]&0x80))
  {
   static int counto[2]={0,0};

   if(counto[x]<=0)
   {
    if(SPSG[x<<2]&0x40)
    {
     if(amplitude[x]<0x3F)
      amplitude[x]++;
    }
    else
    {
     if(amplitude[x]>0)
      amplitude[x]--;
    }
    counto[x]=(SPSG[x<<2]&0x3F);
   }
   else
    counto[x]--;
  }
}

static DECLFR(FDSWaveRead)
{
 return(fdso.cwave[A&0x3f]|(X.DB&0xC0));
}

static DECLFW(FDSWaveWrite)
{
 if(SPSG[0x9]&0x80)
  fdso.cwave[A&0x3f]=V&0x3F;
}

static INLINE void ClockRise(void)
{
 if(!clockcount)
 {
  b19shiftreg60=(SPSG[0x2]|((SPSG[0x3]&0xF)<<8));
  b17latch76=(SPSG[0x6]|((SPSG[0x07]&0x3)<<8))+b17latch76;

  if(!(SPSG[0x7]&0x80))
  {
   b8shiftreg88=(amplitude[1]*((fdso.mwave[(b17latch76>>11)&0x1F]&7)));
   //b8shiftreg88=((fdso.mwave[(b17latch76>>11)&0x1F]&7))|(amplitude[1]<<3);
  }
  else
  { b8shiftreg88=0;}
 }
 else
 {
  b19shiftreg60<<=1;  
  b8shiftreg88>>=1;
 }
 b24adder66=(b24latch68+b19shiftreg60)&0xFFFFFF;
}

static INLINE void ClockFall(void)
{
// if(!(SPSG[0x7]&0x80))
 {
  if(!(b8shiftreg88&1))
   b24latch68=b24adder66;
 }
 clockcount=(clockcount+1)&7;
}

static INLINE int32 FDSDoSound(void)
{
 fdso.count+=fdso.cycles;
 if(fdso.count>=((int64)1<<40))
 {
  dogk:
  fdso.count-=(int64)1<<40;
  ClockRise();
  ClockFall();
 }
 if(fdso.count>=32768) goto dogk;

 fdso.envcount-=fdso.cycles;
 if(fdso.envcount<=0)
 {
  // Fix this?
  fdso.envcount+=((int64)1<<40)*FDSClock/1024;
  DoEnv();
 }

 // Might need to emulate applying the amplitude to the waveform a bit better...
 return (fdso.cwave[b24latch68>>18]*amplitude[0]);
}

static int32 FBC=0;

static void RenderSound(void)
{
 int32 end, start;
 int32 x;

 start=FBC;
 end=(timestamp<<16)/soundtsinc;
 if(end<=start)
  return;
 FBC=end;

 if(!(SPSG[0x9]&0x80))
  for(x=start;x<end;x++)
  {
   uint32 t=FDSDoSound();
   Wave[x>>4]+=t>>3;
  }
}

void FDSSound(int c)
{
  RenderSound();
  FBC=c;
}

static void FDS_ESI(void)
{
 if(FSettings.SndRate)
 {
  fdso.cycles=((int64)1<<40)*FDSClock;
  fdso.cycles/=FSettings.SndRate OVERSAMPLE;
 }
//  fdso.cycles=(int64)32768*FDSClock/(FSettings.SndRate OVERSAMPLE);
 SetReadHandler(0x4040,0x407f,FDSWaveRead);
 SetWriteHandler(0x4040,0x407f,FDSWaveWrite);
 SetWriteHandler(0x4080,0x408A,FDSSWrite);
 SetReadHandler(0x4090,0x4092,FDSSRead);
}

void FDSSoundReset(void)
{
 memset(&fdso,0,sizeof(fdso));
 FDS_ESI();
 GameExpSound.Fill=FDSSound;
 GameExpSound.RChange=FDS_ESI;
}


static DECLFW(FDSWrite4020)
{
	X6502_IRQEnd(FCEU_IQEXT);
	IRQLatch&=0xFF00;
	IRQLatch|=V;
	mapbyte1[0]=V;
}
static DECLFW(FDSWrite4021)
{
	X6502_IRQEnd(FCEU_IQEXT);
	IRQLatch&=0xFF;
	IRQLatch|=V<<8;
	mapbyte1[1]=V;
}
static DECLFW(FDSWrite4022)
{
	X6502_IRQEnd(FCEU_IQEXT);
	IRQCount=IRQLatch;
	IRQa=V&2;
	mapbyte1[2]=V;
}
static DECLFW(FDSWrite4023)
{
	mapbyte1[3]=V;
}
static DECLFW(FDSWrite4024)
{
        if(InDisk!=255 && !(mapbyte1[5]&0x4) && mapbyte1[3]&0x1)
        {
         if(DiskPtr>=0 && DiskPtr<65000)
         {
          if(writeskip) writeskip--;
          else if(DiskPtr>=2)
          {
           SideWrite|=1<<InDisk;
           diskdata[InDisk][DiskPtr-2]=V;
          }
         }
        }
}
static DECLFW(FDSWrite4025)
{
	if(InDisk!=255)
	{
         if(!(V&0x40))
         {
          if(mapbyte1[5]&0x40 && !(V&0x10))
          {
           DiskSeekIRQ=200;
           DiskPtr-=2;
          }
          if(DiskPtr<0) DiskPtr=0;
         }
         if(!(V&0x4)) writeskip=2;
         if(V&2) {DiskPtr=0;DiskSeekIRQ=200;}
         if(V&0x40) DiskSeekIRQ=200;
	}
	mapbyte1[5]=V;
        setmirror(((V>>3)&1)^1);
}
static void FreeFDSMemory(void)
{
 int x;

 for(x=0;x<header[4];x++)
  if(diskdata[x])
  {
   free(diskdata[x]);
   diskdata[x]=0;
  }
}

int FDSLoad(char *name, int fp)
{
 FILE *zp;
 int x;

 FCEU_fseek(fp,0,SEEK_SET);
 FCEU_fread(header,16,1,fp);

 if(memcmp(header,"FDS\x1a",4)) 
 {
  if(!(memcmp(header+1,"*NINTENDO-HVC*",14)))
  {
   long t;   
   t=FCEU_fgetsize(fp);
   if(t<65500)
    t=65500;
   header[4]=t/65500;
   header[0]=0;
   FCEU_fseek(fp,0,SEEK_SET);
  }
  else
   return 0; 
 } 

 if(header[4]>4) header[4]=4;
 if(!header[4]) header[4]|=1;
 for(x=0;x<header[4];x++)
 {
  diskdata[x]=FCEU_malloc(65500);
  if(!diskdata[x]) 
  {
   int zol;
   for(zol=0;zol<x;zol++)
    free(diskdata[zol]);
   return 0;
  }
  FCEU_fread(diskdata[x],1,65500,fp);
 }

 if(!(zp=fopen(FCEU_MakeFName(FCEUMKF_FDSROM,0,0),"rb"))) 
 {
  FCEU_PrintError("FDS BIOS ROM image missing!");
  FreeFDSMemory();
  return 0;
 }

 if(fread(FDSBIOS,1,8192,zp)!=8192)
 {
  fclose(zp);
  FreeFDSMemory();
  FCEU_PrintError("Error reading FDS BIOS ROM image");
  return 0;
 }

 fclose(zp);

 FCEUGameInfo.type=GIT_FDS;
 strcpy(FDSSaveName,name);
 GameInterface=FDSGI;

 SelectDisk=0;
 InDisk=255;

 ResetExState();
 FDSSoundStateAdd();

 for(x=0;x<header[4];x++)
 {
  char temp[5];
  sprintf(temp,"DDT%d",x);
  AddExState(diskdata[x],65500,0,temp);
 }

 AddExState(FDSRAM,32768,0,"FDSR");
 AddExState(mapbyte1,32,0,"MPBY");
 AddExState(CHRRAM,8192,0,"CHRR");
 AddExState(&IRQCount, 4, 1, "IRQC");
 AddExState(&IRQLatch, 4, 1, "IQL1");
 AddExState(&IRQa, 1, 0, "IRQA");


 ResetCartMapping();

 SetupCartCHRMapping(0,CHRRAM,8192,1);
 SetupCartMirroring(0,0,0);

 return 1;
}

void FDSClose(void)
{
  int fp;
  int x;

  fp=FCEU_fopen(FDSSaveName,"wb");
  if(!fp) return;

  if(header[0])			// Does it have a 16-byte FWNES-style header?
   if(FCEU_fwrite(header,1,16,fp)!=16) // Write it out.  Should be nicer than fseek()'ing if the file is compressed.  Hopefully...
    goto fdswerr;

  for(x=0;x<header[4];x++)
  {
   if(FCEU_fwrite(diskdata[x],1,65500,fp)!=65500) 
   {
    fdswerr:
    FCEU_PrintError("Error writing FDS image \"%s\"!",FDSSaveName);
    FCEU_fclose(fp);
    return;
   }
  }
  FreeFDSMemory();
  FCEU_fclose(fp);
}
