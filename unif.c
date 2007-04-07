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

/* TODO:  Battery backup file saving, mirror force */
/* **INCOMPLETE** 				   */
/* Override stuff: CHR RAM instead of CHR ROM,
		   mirroring.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include	"types.h"
#include        "fce.h"
#include        "unif.h"
#include        "version.h"
#include	"svga.h"
#include        "general.h"
#include	"state.h"
#include	"endian.h"
#include	"file.h"
#include	"cart.h"
#include	"memory.h"
#include	"input.h"

typedef struct {
           char ID[4];
           uint32 info;
} UNIF_HEADER;

typedef struct {
           char *name;
           void (*init)(void);
	   int flags;
} BMAPPING;

typedef struct {
           char *name;
           int (*init)(int fp);
} BFMAPPING;

void (*BoardClose)(void);
void (*BoardPower)(void);
void (*BoardReset)(void);

static int vramo;
static int mirrortodo;
int UNIFbattery;
static char *boardname;
static char *sboardname;
char *UNIFchrrama;

static UNIF_HEADER unhead;
static UNIF_HEADER uchead;


static uint8 *malloced[32];

static int FixRomSize(uint32 size, uint32 minimum)
{
  int x=1;

  if(size<minimum)
   return minimum;
  while(x<size)
   x<<=1;
  return x;
}

static void FreeUNIF(void)
{
 int x;
 if(UNIFchrrama)
  {free(UNIFchrrama);UNIFchrrama=0;}
 if(boardname)
  {free(boardname);boardname=0;}
 for(x=0;x<32;x++)
 {
  if(malloced[x])
   {free(malloced[x]);malloced[x]=0;}
 }
}

static void ResetUNIF(void)
{
 int x;
 for(x=0;x<32;x++)
  malloced[x]=0;
 vramo=0;
 UNIFbattery=0;
 boardname=0;
 mirrortodo=0;
 BoardReset=BoardPower=BoardClose=0;
 UNIFchrrama=0;
}

static uint8 exntar[2048];

static void MooMirroring(void)
{
 if(mirrortodo<0x4)
  SetupCartMirroring(mirrortodo,1,0);
 else if(mirrortodo==0x4)
 {
  SetupCartMirroring(4,1,exntar);
  AddExState(exntar, 2048, 0,"EXNR");
 }
 else
  SetupCartMirroring(0,0,0);
}

static int DoMirroring(int fp)
{
 uint8 t;
 t=FCEU_fgetc(fp);
 mirrortodo=t; 

 {
  static char *stuffo[6]={"Horizontal","Vertical","$2000","$2400","\"Four-screen\"","Controlled by Mapper Hardware"};
  if(t<6)
   printf(" Name/Attribute Table Mirroring: %s\n",stuffo[t]);
 }
 return(1);
}

static int CTRL(int fp)
{
 int t;

 if((t=FCEU_fgetc(fp))==EOF)
  return(0);
 /* The information stored in this byte isn't very helpful, but it's
    better than nothing...maybe.
 */

 if(t&1) FCEUGameInfo.input[0]=FCEUGameInfo.input[1]=SI_GAMEPAD;
 else FCEUGameInfo.input[0]=FCEUGameInfo.input[1]=SI_NONE;

 if(t&2) FCEUGameInfo.input[1]=SI_ZAPPER;
 else if(t&0x10) FCEUGameInfo.input[1]=SI_POWERPAD;

 return(1);
}

static int TVCI(int fp)
{
 int t;
 if( (t=FCEU_fgetc(fp)) ==EOF)
  return(0);
 if(t<=2)
 {
  char *stuffo[3]={"NTSC","PAL","NTSC and PAL"};
  if(t==0)
   FCEUGameInfo.vidsys=GIV_NTSC;
  else if(t==1)
   FCEUGameInfo.vidsys=GIV_PAL;
  printf(" TV Standard Compatibility: %s\n",stuffo[t]);
 }
 return(1);
}

static int EnableBattery(int fp)
{
 puts(" Battery-backed.");
 if(FCEU_fgetc(fp)==EOF)
  return(0);
 UNIFbattery=1;
 return(1);
}

static int LoadPRG(int fp)
{
 int z,t;
 z=uchead.ID[3]-'0';

 if(z<0 || z>15)
  return(0);
 printf(" PRG ROM %d size: %d",z,(int) uchead.info);
 if(malloced[z])
  free(malloced[z]);
 t=FixRomSize(uchead.info,2048);
 if(!(malloced[z]=FCEU_malloc(t)))
  return(0);
 memset(malloced[z]+uchead.info,0xFF,t-uchead.info);
 if(FCEU_fread(malloced[z],1,uchead.info,fp)!=uchead.info)
 {
  puts("Read Error!");
  return(0);
 }
 else
  puts("");

 SetupCartPRGMapping(z,malloced[z],t,0); 
 return(1);
}

static int SetBoardName(int fp)
{
 if(!(boardname=FCEU_malloc(uchead.info+1)))
  return(0);
 FCEU_fread(boardname,1,uchead.info,fp);
 boardname[uchead.info]=0;
 printf(" Board name: %s\n",boardname);
 sboardname=boardname;
 if(!memcmp(boardname,"NES-",4) || !memcmp(boardname,"UNL-",4) || !memcmp(boardname,"HVC-",4) || !memcmp(boardname,"BTL-",4) || !memcmp(boardname,"BMC-",4))
  sboardname+=4;
 return(1);
}

static int LoadCHR(int fp)
{
 int z,t;
 z=uchead.ID[3]-'0';
 if(z<0 || z>15)
  return(0);
 printf(" CHR ROM %d size: %d",z,(int) uchead.info);
 if(malloced[16+z])
  free(malloced[16+z]);
 t=FixRomSize(uchead.info,8192);
 if(!(malloced[16+z]=FCEU_malloc(t)))
  return(0);
 memset(malloced[16+z]+uchead.info,0xFF,t-uchead.info);
 if(FCEU_fread(malloced[16+z],1,uchead.info,fp)!=uchead.info)
 {
  puts("Read Error!");
  return(0);
 }
 else
  puts("");

 SetupCartCHRMapping(z,malloced[16+z],t,0);
 return(1);
}


#define BMCFLAG_FORCE4	1
#define BMCFLAG_CHRROK	2	// Ok for generic UNIF code to make available
				// 8KB of CHR RAM if no CHR ROM is present.
#define BMC 48

BMAPPING bmap[BMC] = {

/* Sachen Carts */
 { "TC-U01-1.5M", TCU01_Init,0},
 { "Sachen-8259B", S8259B_Init,BMCFLAG_CHRROK},
 { "Sachen-8259A", S8259A_Init,BMCFLAG_CHRROK},
 { "Sachen-74LS374N", S74LS374N_Init,0},
 { "SA-016-1M", SA0161M_Init,0},
 { "SA-72007", SA72007_Init,0},
 { "SA-72008", SA72008_Init,0},
 { "SA-0036", SA0036_Init,0},
 { "SA-0037", SA0037_Init,0},

 { "H2288", H2288_Init,0},
// /* AVE carts. */
// { "MB-91", MB91_Init,0},	// DeathBots
// { "NINA-06", NINA06_Init,0},	// F-15 City War
// { "NINA-03", NINA03_Init,0},	// Tiles of Fate
// { "NINA-001", NINA001_Init,0}, // Impossible Mission 2

 { "HKROM", HKROM_Init,0},

 { "EWROM", EWROM_Init,0},
 { "EKROM", EKROM_Init,0},
 { "ELROM", ELROM_Init,0},
 { "ETROM", ETROM_Init,0},

 { "SAROM", SAROM_Init,0},
 { "SBROM", SBROM_Init,0},
 { "SCROM", SCROM_Init,0},
 { "SEROM", SEROM_Init,0},
 { "SGROM", SGROM_Init,0},
 { "SKROM", SKROM_Init,0},
 { "SLROM", SLROM_Init,0},
 { "SL1ROM", SL1ROM_Init,0},
 { "SNROM", SNROM_Init,0},
 { "SOROM", SOROM_Init,0},

 { "TGROM", TGROM_Init,0},
 { "TR1ROM", TFROM_Init,BMCFLAG_FORCE4},
 { "TFROM", TFROM_Init,0},
 { "TLROM", TLROM_Init,0},
 { "TKROM", TKROM_Init,0},
 { "TSROM", TSROM_Init,0},

 { "TLSROM", TLSROM_Init,0},
 { "TKSROM", TKSROM_Init,0},
 { "TQROM", TQROM_Init,0},
 { "TVROM", TLROM_Init,BMCFLAG_FORCE4},

 { "CPROM", CPROM_Init,0},
 { "CNROM", CNROM_Init,0},
 { "NROM", NROM256_Init,0 },
 { "RROM", NROM128_Init,0 },
 { "RROM-128", NROM128_Init,0 },
 { "NROM-128", NROM128_Init,0 },
 { "NROM-256", NROM256_Init,0 },
 { "MHROM", MHROM_Init,0},
 { "UNROM", UNROM_Init,0},
 { "MARIO1-MALEE2", MALEE_Init,0},
 { "Supervision16in1", Supervision16_Init,0},
 { "NovelDiamond9999999in1", Novel_Init,0},
 { "Super24in1SC03", Super24_Init,0}
};

#define BMF 7
BFMAPPING bfunc[BMF] = {
 { "CTRL", CTRL },
 { "TVCI", TVCI },
 { "BATR", EnableBattery },
 { "MIRR", DoMirroring },
 { "PRG",  LoadPRG },
 { "CHR",  LoadCHR },
 { "MAPR", SetBoardName }
};

int LoadUNIFChunks(int fp)
{
   int x;
   int t;
   for(;;)
   {
    t=FCEU_fread(&uchead,1,4,fp);
    if(t<4) 
    {
     if(t>0)
      return 0; 
     return 1;
    }
    if(!(FCEU_read32(&uchead.info,fp))) 
     return 0;
    t=0;
    for(x=0;x<BMF;x++)
    {
     if(memcmp(&uchead,bfunc[x].name,strlen(bfunc[x].name)))
      continue;
     if(!bfunc[x].init(fp))
      return 0;
     t=1;
     break;     
    }
    if(!t)
     if(FCEU_fseek(fp,uchead.info,SEEK_CUR))
      return(0);
   }
}

static int InitializeBoard(void)
{
   int x;

   for(x=0;x<BMC;x++)
   {
    if(strcmp(sboardname,bmap[x].name)) continue;
    if(!malloced[16] && (bmap[x].flags&BMCFLAG_CHRROK))
    {
     if((malloced[16]=UNIFchrrama=FCEU_malloc(8192)))
     {
      SetupCartCHRMapping(0,UNIFchrrama,8192,1);
      AddExState(UNIFchrrama, 8192, 0,"CHRR");
     }
     else
      return(-1);
    }
    if(bmap[x].flags&BMCFLAG_FORCE4)
     mirrortodo=4;
    MooMirroring();
    bmap[x].init();
    return(1);
   }
   FCEU_PrintError("Board type not supported.");
   return(0);
}

static void UNIFGI(int h)
{
 switch(h)
 {
  case GI_RESETM2:
		if(BoardReset) BoardReset();
		break;
  case GI_POWER:
                if(BoardPower) BoardPower();
		if(UNIFchrrama) memset(UNIFchrrama,0,8192);
		break;
  case GI_CLOSE:
                if(BoardClose)
                 BoardClose();
                FreeUNIF();
                break;
 }
}

int UNIFLoad(char *name, int fp)
{
        FCEU_fseek(fp,0,SEEK_SET);
        FCEU_fread(&unhead,1,4,fp);
        if(memcmp(&unhead,"UNIF",4))
         return 0;        

	ResetCartMapping();

        ResetExState();
        ResetUNIF();
        if(!FCEU_read32(&unhead.info,fp))
	 goto aborto;
        if(FCEU_fseek(fp,0x20,SEEK_SET)<0)
	 goto aborto;
        if(!LoadUNIFChunks(fp))
	 goto aborto;
        if(!InitializeBoard())
	 goto aborto;

        GameInterface=UNIFGI;
        return 1;
	
	aborto:

	FreeUNIF();
	ResetUNIF();
	return 0;
}

static FILE *fssp=0;

void UNIFOpenWRAM(int t, char *ext, int override)
{
 if(UNIFbattery|override)
 {
  fssp=fopen(FCEU_MakeFName(FCEUMKF_SAV,0,(ext?ext:"sav")),(t==UOW_RD)?"rb":"wb");
 }
}

void UNIFWriteWRAM(uint8 *p, int size)
{
 if(fssp)
  fwrite(p, size, 1, fssp);
}

void UNIFReadWRAM(uint8 *p, int size)
{
 if(fssp)
  fread(p, size, 1, fssp);
}
void UNIFCloseWRAM(void)
{
 if(fssp)
  fclose(fssp);
 fssp=0;
}
