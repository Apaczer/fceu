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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "types.h"
#include "x6502.h"
#include "fce.h"
#define INESPRIV
#include "ines.h"
#include "version.h"
#include "svga.h"
#include "general.h"
#include "state.h"
#include "file.h"
#include "memory.h"
#include "cart.h"
#include "crc32.h"
#include "cheat.h"

static DECLFR(VSRead);

static uint8 *trainerpoo=0;
static uint8 *ROM=NULL;
uint8 *VROM=NULL;


static uint32 ROM_size;
uint32 VROM_size;

static void CheckVSUni(void);
static int MMC_init(int type);
void (*MapClose)(void);
void (*MapperReset)(void);

static int MapperNo;
static int SaveGame=0;

static iNES_HEADER head;

/*  MapperReset() is called when the NES is reset(with the reset button).  
    Mapperxxx_init is called when the NES has been powered on.
*/

static void iNESGI(int h)
{
 switch(h)
 {
  case GI_RESETM2:
		if(MapperReset)
		 MapperReset();
		break;
  case GI_POWER:
		SetReadHandler(0x8000,0xFFFF,CartBR);
                MMC_init(MapperNo);
		break;
  case GI_CLOSE:
		{
	         FILE *sp;
	        
	 	 if(ROM) {free(ROM);ROM=0;}
		 if(VROM) {free(VROM);VROM=0;}

        	 if(SaveGame)
        	 {
	 	  char *soot;
	          SaveGame=0;
	          soot=FCEU_MakeFName(FCEUMKF_SAV,0,"sav");
	          sp=fopen(soot,"wb");
	          if (sp==NULL)
 	           FCEU_PrintError("WRAM file \"%s\" cannot be written to.\n",soot);
	          else
	          {
		   void *ptr;
		   uint32 amount;
		   ptr=WRAM;
		   amount=8192;

	           if(MapperNo==1)
         	   {
	             extern uint8 MMC1WRAMsize;
         	     if(MMC1WRAMsize==2) ptr=WRAM+8192;
	           }
	           else if(MapperNo==5)
        	   {
	            extern uint8 MMC5WRAMsize;
        	    if(MMC5WRAMsize==4)
	             amount=32768;
        	   }

		   fwrite(ptr,1,amount,sp);
	           fclose(sp);
	          }
        	 }
	         if(MapClose) MapClose();
		 if(trainerpoo) {free(trainerpoo);trainerpoo=0;}
	        }
        	break;
     }
}

uint32 iNESGameCRC32;

struct CRCMATCH	{
	uint32 crc;
	char *name;
};

static void CheckBad(void)
{
 int x;
 #define CRCNT	7
 struct CRCMATCH tab[CRCNT]={
	{0x28d183ac,"Antarctic Adventure"},
	{0x7095ac65,"Akumajo Densetsu"},
	{0x1bd7ed5a,"Gradius 2"},
	{0x81c3aa66,"Crisis Force"},
	{0xfe3088df,"Fire Emblem Gaiden"},
	{0xfa8339a5,"Bucky O'Hare"},
	{0x3c539d78,"Ganbare Goemon 2"},
	};
  for(x=0;x<CRCNT;x++)
   if(tab[x].crc == iNESGameCRC32)
   {
    FCEU_PrintError("The copy of the game you have loaded, %s, is a bad dump, has been hacked, or both.  It will not work correctly on FCE Ultra.  Use a good copy of the ROM image instead.",tab[x].name);
    break;
   }
}

struct INPSEL {
	uint32 crc32;
	int input1;
	int input2;
	int inputfc;
};

/* This is mostly for my personal use.  So HA. */
static void SetInput(void)
{
 static struct INPSEL moo[]=
	{
	 {0xad9c63e2,SI_GAMEPAD,-1,SIFC_SHADOW},	/* Space Shadow */
	 {0x24598791,-1,SI_ZAPPER,0},	/* Duck Hunt */
	 {0xff24d794,-1,SI_ZAPPER,0},   /* Hogan's Alley */
	 {0xbeb8ab01,-1,SI_ZAPPER,0},	/* Gumshoe */
	 {0xde8fd935,-1,SI_ZAPPER,0},	/* To the Earth */
	 {0xedc3662b,-1,SI_ZAPPER,0},	/* Operation Wolf */
	 {0x23d17f5e,SI_GAMEPAD,SI_ZAPPER,0},	/* The Lone Ranger */
	 {0xb8b9aca3,-1,SI_ZAPPER,0},  /* Wild Gunman */
	 {0x5112dc21,-1,SI_ZAPPER,0},  /* Wild Gunman */
	 {0x4318a2f8,-1,SI_ZAPPER,0},  /* Barker Bill's Trick Shooting */
	 {0x5ee6008e,-1,SI_ZAPPER,0},  /* Mechanized Attack */
	 {0x3e58a87e,-1,SI_ZAPPER,0},  /* Freedom Force */
	 {0x851eb9be,SI_GAMEPAD,SI_ZAPPER,0},	/* Shooting Range */
	 {0x74bea652,SI_GAMEPAD,SI_ZAPPER,0},	/* Supergun 3-in-1 */
	 {0x32fb0583,-1,SI_ARKANOID,0}, /* Arkanoid(NES) */
	 {0xd89e5a67,-1,-1,SIFC_ARKANOID}, /* Arkanoid (J) */
	 {0x0f141525,-1,-1,SIFC_ARKANOID}, /* Arkanoid 2(J) */

	 {0xf7606810,-1,-1,SIFC_FKB},	/* Family BASIC 2.0A */
	 {0x895037bc,-1,-1,SIFC_FKB},	/* Family BASIC 2.1a */
	 {0xb2530afc,-1,-1,SIFC_FKB},	/* Family BASIC 3.0 */
	 {0,-1,-1,-1}
	};
 int x=0;

 while(moo[x].input1>=0 || moo[x].input2>=0 || moo[x].inputfc>=0)
 {
  if(moo[x].crc32==iNESGameCRC32)
  {
   FCEUGameInfo.input[0]=moo[x].input1;
   FCEUGameInfo.input[1]=moo[x].input2;
   FCEUGameInfo.inputfc=moo[x].inputfc;
   break;
  }
  x++;
 }
}

struct CHINF {
	uint32 crc32;
	int32 mapper;
	int32 mirror;
};

#define SAVCNT	14
static void CheckHInfo(void)
{
 /* ROM images that have the battery-backed bit set in the header that really
    don't have battery-backed RAM is not that big of a problem, so I'll
    treat this differently.
 */

 static uint32 savie[SAVCNT]=
	{
	 0x7cab2e9b,0x3ee43cda,0xe1383deb,	/* Mouryou Senki Madara */
	 0x3b3f88f0,0x2545214c,			/* Dragon Warrior PRG 0 and 1 */
	 0x8c5a784e,	/* DW 2 */
	 0xa86a5318,	/* DW 3 */
	 0x506e259d,    /* DW 4 */
	 0xcebd2a31,0xb8b88130, /* Final Fantasy */
	 0x466efdc2,	/* FF(J) */
	 0xc9556b36,	/* FF1+2*/
	 0xd29db3c7,	/* FF2 */
	 0x57e220d0,	/* FF3 */
	};

 static struct CHINF moo[]=
	{
	 {0xc68363f6,180,0},	/* Crazy Climber */
	 {0xbe939fce,9,1},	/* Punchout*/
	 {0x5e66eaea,13,1},	/* Videomation */
	 {0xaf5d7aa2,-1,0},	/* Clu Clu Land */

	 {0xc2730c30,34,0},	/* Deadly Towers */
	 {0x932ff06e,34,1},	/* Classic Concentration */
	 {0x4c7c1af3,34,1},	/* Caesar's Palace */
	 {0x9ea1dc76,2,0},	/* Rainbow Islands */
	 
	 {0x9eefb4b4,4,8},	/* Pachi Slot Adventure 2 */
	 {0x5337f73c,4,8},	/* Niji no Silk Road */
	 {0x7474ac92,4,8},	/* Kabuki: Quantum Fighter */

	 {0x970bd9c2,1,8},	/* Hanjuku Hero */

	 {0xbb7c5f7a,89,8},	/* Mito Koumon or something similar */
	 /* Probably a Namco MMC3-workalike */
	 {0xa5e6baf9,4,1|4},	/* Dragon Slayer 4 */
	 {0xe40b4973,4,1|4},	/* Metro Cross */
         {0xd97c31b0,4,1|4},    /* Rasaaru Ishii no Childs Quest */

	 {0x84382231,9,0},	/* Punch Out (J) */

	 {0xfcdaca80,0,0},	/* Elevator Action */
	 {0xe492d45a,0,0},	/* Zippy Race */
	 {0x32fa246f,0,0},	/* Tag Team Pro Wrestling */
	 {0x6d65cac6,2,0},	/* Terra Cresta */
	 {0x28c11d24,2,1},	/* Sukeban Deka */
         {0x02863604,2,1},      /* Sukeban Deka */
         {0x2bb6a0f8,2,1},      /* Sherlock Holmes */
	 {0x55773880,2,1},	/* Gilligan's Island */
	 {0x419461d0,2,1},	/* Super Cars */
	 {0x6e0eb43e,2,1},	/* Puss n Boots */
	 {0xfc3e5c86,2,1},	/* Trojan */

	 {0x291bcd7d,1,8},	/* Pachio Kun 2 */
	 {0xf74dfc91,1,-1},	/* Win, Lose, or Draw */

	 {0x59280bec,4,8},	/* Jackie Chan */

	 {0xfe364be5,1,8},	/* Deep Dungeon 4 */
	 {0xd8ee7669,1,8},	/* Adventures of Rad Gravity */
	 {0xa5e8d2cd,1,8},	/* Breakthru */
	 {0xf6fa4453,1,8},	/* Bigfoot */
	 {0x37ba3261,1,8},	/* Back to the Future 2 and 3 */
	 {0x934db14a,1,-1},	/* All-Pro Basketball */
	 {0xe94d5181,1,8},	/* Mirai Senshi - Lios */
	 {0x7156cb4d,1,8},	/* Muppet Adventure Carnival thingy */
	 {0x5b6ca654,1,8},	/* Barbie rev X*/
	 {0x57c12280,1,8},	/* Demon Sword */

	 {0xcf322bb3,3,1},	/* John Elway's Quarterback */
	 {0x9bde3267,3,1},	/* Adventures of Dino Riki */
	 {0x02cc3973,3,8},	/* Ninja Kid */
	 {0xb5d28ea2,3,1},	/* Mystery Quest - mapper 3?*/
	 {0xbc065fc3,3,1},	/* Pipe Dream */

	 {0x5b837e8d,1,8},	/* Alien Syndrome */
	 {0x283ad224,32,8},	/* Ai Sensei no Oshiete */
	 {0x5555fca3,32,8},	/* "" ""		*/
	 {0x243a8735,32,0x10|4}, /* Major League */

	 {0x6bc65d7e,66,1},	/* Youkai Club*/
	 {0xc2df0a00,66,1},	/* Bio Senshi Dan(hacked) */
	 {0xbde3ae9b,66,1},	/* Doraemon */
	 {0xd26efd78,66,1},	/* SMB Duck Hunt */
	 {0x811f06d9,66,1},	/* Dragon Power */

	 {0x3293afea,66,1},	/* Mississippi Satsujin Jiken */
	 {0xe84274c5,66,1},	/* "" "" */


	 {0x6e68e31a,16,8},	/* Dragon Ball 3*/

	 {0xba51ac6f,78,2},
	 {0x3d1c3137,78,8},

	 {0xbda8f8e4,152,8},	/* Gegege no Kitarou 2 */
	 {0x026c5fca,152,8},	/* Saint Seiya Ougon Densetsu */
	 {0x0f141525,152,8},	/* Arkanoid 2 (Japanese) */
	 {0xb1a94b82,152,8},	/* Pocket Zaurus */

	 {0x3f15d20d,153,8},	/* Famicom Jump 2 */

	 {0xbba58be5,70,-1},	/* Family Trainer - Manhattan Police */
	 {0x370ceb65,70,-1},	/* Family Trainer - Meiro Dai Sakusen */
	 {0xdd8ed0f7,70,1},	/* Kamen Rider Club */

	 {0x90c773c1,118,-1},	/* Goal! 2 */
	 {0xb9b4d9e0,118,-1},	/* NES Play Action Football */
	 {0x78b657ac,118,-1},	/* Armadillo */
	 {0x37b62d04,118,-1},	/* Ys 3 */
	 {0x07d92c31,118,-1},   /* RPG Jinsei Game */
	 {0x2705eaeb,234,-1},	/* Maxi 15 */
	 {0x404b2e8b,4,2},	/* Rad Racer 2 */

	 {0x1356f6a6,4,8},	/* "Cattou Ninden Teyandee" English tranlation.
				    Should I have even bothered to do this? */
	 {0x50fd4fd6,4,8},	/* "" "" */

	 {0xa912b064,51|0x800,8},	/* 11-in-1 Ball Games(has CHR ROM when it shouldn't) */
	 {0,-1,-1}
	};
 int tofix=0;
 int x=0;

 do
 {
  if(moo[x].crc32==iNESGameCRC32)
  {
   if(moo[x].mapper>=0)
   {
    if(moo[x].mapper&0x800 && VROM_size)
    {
     VROM_size=0;
     free(VROM);
     tofix|=8;
    }
    if(MapperNo!=(moo[x].mapper&0xFF))
    {
     tofix|=1;
     MapperNo=moo[x].mapper&0xFF;
    }
   }
   if(moo[x].mirror>=0)
   {
    if(moo[x].mirror==8)
    {
     if(Mirroring==2)	/* Anything but hard-wired(four screen). */
     {
      tofix|=2;
      Mirroring=0;
     }
    }
    else if(Mirroring!=moo[x].mirror)
    {
     if(Mirroring!=(moo[x].mirror&~4))
      if((moo[x].mirror&~4)<=2)	/* Don't complain if one-screen mirroring
				   needs to be set(the iNES header can't
				   hold this information).
				*/
       tofix|=2;
     Mirroring=moo[x].mirror;
    }
   }
   break;
  }
  x++;
 } while(moo[x].mirror>=0 || moo[x].mapper>=0);

 for(x=0;x<SAVCNT;x++)
 {
  if(savie[x]==iNESGameCRC32)
  {
   if(!(head.ROM_type&2))
   {
    tofix|=4;
    head.ROM_type|=2;
   }
  }
 }

 /* Games that use these iNES mappers tend to have the four-screen bit set
    when it should not be.
 */
 if((MapperNo==118 || MapperNo==24 || MapperNo==26) && (Mirroring==2))
 {
  Mirroring=0;
  tofix|=2;
 }

 if(tofix)
 {
  char gigastr[768];
  strcpy(gigastr,"The iNES header contains incorrect information.  For now, the information will be corrected in RAM.  ");
  if(tofix&1)
   sprintf(gigastr+strlen(gigastr),"The mapper number should be set to %d.  ",MapperNo);
  if(tofix&2)
  {
   char *mstr[3]={"Horizontal","Vertical","Four-screen"};
   sprintf(gigastr+strlen(gigastr),"Mirroring should be set to \"%s\".  ",mstr[Mirroring&3]);
  }
  if(tofix&4)
   strcat(gigastr,"The battery-backed bit should be set.  ");  
  if(tofix&8)
   strcat(gigastr,"This game should not have any CHR ROM.  ");
  strcat(gigastr,"\n");
  FCEUD_PrintError(gigastr);
 }
}

int iNESLoad(char *name, int fp)
{
        FILE *sp;

	if(FCEU_fread(&head,1,16,fp)!=16)
 	 return 0;

        if(memcmp(&head,"NES\x1a",4))
         return 0;

        if(!memcmp((char *)(&head)+0x7,"DiskDude",8))
        {
         memset((char *)(&head)+0x7,0,0x9);
        }

        if(!memcmp((char *)(&head)+0x7,"demiforce",9))
        {
         memset((char *)(&head)+0x7,0,0x9);
        }

        if(!memcmp((char *)(&head)+0xA,"Ni03",4))
        {
         if(!memcmp((char *)(&head)+0x7,"Dis",3))
          memset((char *)(&head)+0x7,0,0x9);
         else
          memset((char *)(&head)+0xA,0,0x6);
        }

        if(!head.ROM_size)
         head.ROM_size++;

        ROM_size = head.ROM_size;
        VROM_size = head.VROM_size;
	ROM_size=uppow2(ROM_size);

        if(VROM_size)
	 VROM_size=uppow2(VROM_size);

        MapperNo = (head.ROM_type>>4);
        MapperNo|=(head.ROM_type2&0xF0);

        Mirroring = (head.ROM_type&1);
	if(head.ROM_type&8) Mirroring=2;

        if(!(ROM=(uint8 *)FCEU_malloc(ROM_size<<14)))
	 return 0;
		
        if (VROM_size) 
         if(!(VROM=(uint8 *)FCEU_malloc(VROM_size<<13)))
	 {
	  free(ROM);
	  return 0;
	 }

        memset(ROM,0xFF,ROM_size<<14);
        if(VROM_size) memset(VROM,0xFF,VROM_size<<13);
        if(head.ROM_type&4) 	/* Trainer */
	{
	 if(!(trainerpoo=FCEU_malloc(512)))
	  return(0);
 	 FCEU_fread(trainerpoo,512,1,fp);
	}
	ResetCartMapping();
	SetupCartPRGMapping(0,ROM,ROM_size*0x4000,0);
        SetupCartPRGMapping(1,WRAM,8192,1);

        FCEU_fread(ROM,0x4000,head.ROM_size,fp);

	if(VROM_size)
	 FCEU_fread(VROM,0x2000,head.VROM_size,fp);

        printf("File %s loaded.\n",name);

	iNESGameCRC32=CalcCRC32(0,ROM,ROM_size<<14);
	if(VROM_size)
	 iNESGameCRC32=CalcCRC32(iNESGameCRC32,VROM,VROM_size<<13);
        printf("\n PRG ROM:  %3d x 16k\n CHR ROM:  %3d x  8k\n ROM CRC32:  %08lx\n Mapper:  %d\n Mirroring: %s\n",head.ROM_size,head.VROM_size,iNESGameCRC32,MapperNo,Mirroring==2?"None(Four-screen)":Mirroring?"Vertical":"Horizontal");
        if(head.ROM_type&2) puts(" Battery-backed.");
        if(head.ROM_type&4) puts(" Trained.");
	puts("");

	CheckBad();
	SetInput();
	CheckHInfo();
	CheckVSUni();
        //if(MapperNo!=4 && MapperNo!=118 && MapperNo!=119) exit(1); // Testing

	/* Must remain here because above functions might change value of
	   VROM_size and free(VROM).
	*/
	if(VROM_size)
         SetupCartCHRMapping(0,VROM,VROM_size*0x2000,0);

        if(Mirroring==2)
         SetupCartMirroring(4,1,ExtraNTARAM);
        else if(Mirroring>=0x10)
	 SetupCartMirroring(2+(Mirroring&1),1,0);
	else
         SetupCartMirroring(Mirroring&1,(Mirroring&4)>>2,0);

        if(MapperNo==5) DetectMMC5WRAMSize();
	else if(MapperNo==1) DetectMMC1WRAMSize();

        if(head.ROM_type&2)
        {         
         char *soot;

         SaveGame=1;
         soot=FCEU_MakeFName(FCEUMKF_SAV,0,"sav");
         sp=fopen(soot,"rb");
         if(sp!=NULL)
         {
	  void *ptr;
	  uint32 amount;

	  ptr=WRAM;
	  amount=8192;
	  if(MapperNo==1)
	  {
	   extern uint8 MMC1WRAMsize;
	   if(MMC1WRAMsize==2) ptr=WRAM+8192;
	  }
	  else if(MapperNo==5)
	  {
	   extern uint8 MMC5WRAMsize;
           if(MMC5WRAMsize==4)
	    amount=32768;
	  }
          if(fread(ptr,1,amount,sp)==amount)
           printf("  WRAM Save File \"%s\" loaded...\n",soot);
          fclose(sp);
         }

   }
	GameInterface=iNESGI;
        return 1;
}

#include	"banksw.h"


void FASTAPASS(1) onemir(uint8 V)
{
	if(Mirroring==2) return;
        if(V>1)
         V=1;
	Mirroring=0x10|V;
	setmirror(MI_0+V);
}

void FASTAPASS(1) MIRROR_SET2(uint8 V)
{
	if(Mirroring==2) return;
	Mirroring=V;
	setmirror(V);
}

void FASTAPASS(1) MIRROR_SET(uint8 V)
{
	if(Mirroring==2) return;
	V^=1;
	Mirroring=V;
	setmirror(V);
}

static void NONE_init(void)
{
        ROM_BANK16(0x8000,0);
        ROM_BANK16(0xC000,~0);

        if(VROM_size) 
	 VROM_BANK8(0);
        else
	 setvram8(CHRRAM);
}

static uint8 secdata[2][32]=
{
 {
  0xff, 0xbf, 0xb7, 0x97, 0x97, 0x17, 0x57, 0x4f,
  0x6f, 0x6b, 0xeb, 0xa9, 0xb1, 0x90, 0x94, 0x14,
  0x56, 0x4e, 0x6f, 0x6b, 0xeb, 0xa9, 0xb1, 0x90,
  0xd4, 0x5c, 0x3e, 0x26, 0x87, 0x83, 0x13, 0x00
 },
 {
  0x00, 0x00, 0x00, 0x00, 0xb4, 0x00, 0x00, 0x00,
  0x00, 0x6F, 0x00, 0x00, 0x00, 0x00, 0x94, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
 }
};

static uint8 *secptr;

static void CheckVSUni(void)
{
        FCEUGameInfo.type=GIT_VSUNI;
	
	/* FCE Ultra doesn't complain when headers for these games are bad. */
	secptr=0;
	switch(iNESGameCRC32)
	{
	 default:FCEUGameInfo.type=0;break;

	 case 0xffbef374: pale=1;break;	/* Castlevania */

	 case 0x98e3c75a: 
	 case 0x7cff0f84: pale=2;break;	/* SMB */
	
	 case 0xef7af338: pale=2;break; /* Ice Climber */
	 case 0xabe1a0c2: FCEUGameInfo.input[0]=SI_ZAPPER;break; /*Duck hunt */
	 case 0x16aa4e2d: pale=7;FCEUGameInfo.input[0]=SI_ZAPPER;break; /* hoganal ^_^ */
	 case 0x2b85420e: pale=3;break; /* Dr Mario */

	 case 0xfb0ddde7: pale=2;break;
	 case 0xc95321a8: pale=6;break; /* Excitebike */
	 case 0xb1c4c508: pale=1;break; /* Golf */

	 case 0x66471efe: break;
	 case 0xca3e9b1a: pale=7;break;	/* Pinball*/

	 case 0xf735d926: pale=7; break; /* Gradius */

	 case 0x2019fe65:
	 case 0x31678411:MapperNo=68;pale=7;break; /* Platoon */

	 case 0x74f713b4:pale=4;break; /* Goonies */
	 case 0x9ae2baa0:pale=5;break; /* Slalom */
	 case 0xe45485a5:secptr=secdata[1];vsdip=0x20;MapperNo=4;break; /* RBI Baseball */
	 case 0x21a653c7:vsdip=0x20;MapperNo=4;break; /* Super Sky Kid */
         case 0xe9a6f17d:vsdip=0x20;break; /* Tetris */

         case 0x159ef3c1:break; /* Star Luster */
         case 0x9768e5e0:break; /* Stroke and Match Golf */
	
	 /* FCE Ultra doesn't have correct palettes for the following games. */
	 case 0x0fa322c2:pale=2;break; /* Clu Clu Land */

	 case 0x766c2cac:	/* Soccer */
	 case 0x01357944:	/* Battle City */
	 case 0xb2816bf9:break; /* Battle City (Bootleg) */

	 case 0x832cf592:FCEUGameInfo.input[0]=SI_ZAPPER;break; /* Freedom Force */
	 case 0x63abf889:break; /* Ladies Golf */
	 case 0x8c0c2df5:pale=2;MapperNo=1;Mirroring=1;break; /* Top Gun */
	 case 0x52c501d0:vsdip=0x80;MapperNo=4;secptr=secdata[0];break; 
			/* TKO Boxing */
	}

	if(MapperNo==99)
	 Mirroring=2;
}


static int VSindex;

static DECLFR(VSRead)
{
 //printf("$%04x, $%04x\n",A,X.PC);
 switch(A)
 {
  default:
  case 0x5e00: VSindex=0;return 0xFF;
  case 0x5e01: return(secptr[(VSindex++)&0x1F]);
 }
}

static void DoVSHooks(void)
{
 if(secptr)
  SetReadHandler(0x5e00,0x5e01,VSRead);
}

void (*MapInitTab[256])(void)=
{
	0,
	Mapper1_init,Mapper2_init,Mapper3_init,Mapper4_init,
	Mapper5_init,Mapper6_init,Mapper7_init,Mapper8_init,
	Mapper9_init,Mapper10_init,Mapper11_init,0,
	Mapper13_init,0,Mapper15_init,Mapper16_init,
	Mapper17_init,Mapper18_init,Mapper19_init,0,
	Mapper21_init,Mapper22_init,Mapper23_init,Mapper24_init,
	Mapper25_init,Mapper26_init,0,0,
	0,0,0,Mapper32_init,
	Mapper33_init,Mapper34_init,0,0,
	0,0,0,Mapper40_init,
	Mapper41_init,Mapper42_init,Mapper43_init,Mapper44_init,
	Mapper45_init,Mapper46_init,Mapper47_init,Mapper33_init,Mapper49_init,0,Mapper51_init,Mapper52_init,
	0,0,0,0,0,0,0,0,
	0,0,0,Mapper64_init,
	Mapper65_init,Mapper66_init,Mapper67_init,Mapper68_init,
	Mapper69_init,Mapper70_init,Mapper71_init,Mapper72_init,
	Mapper73_init,0,Mapper75_init,Mapper76_init,
	Mapper77_init,Mapper78_init,Mapper79_init,Mapper80_init,
	0,Mapper82_init,Mapper83_init,0,
	Mapper85_init,Mapper86_init,Mapper87_init,Mapper88_init,
	Mapper89_init,Mapper90_init,0,Mapper92_init,
	Mapper93_init,Mapper94_init,Mapper95_init,Mapper96_init,
	Mapper97_init,0,Mapper99_init,0,
	0,0,0,0,Mapper105_init,0,0,0,
	0,0,0,Mapper112_init,Mapper113_init,0,0,0,
	Mapper117_init,Mapper118_init,Mapper119_init,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,Mapper140_init,
	0,0,0,0,0,0,0,0,
	0,0,Mapper151_init,Mapper152_init,Mapper153_init,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,Mapper180_init,
	0,Mapper182_init,0,Mapper184_init,Mapper185_init,0,0,0,
	Mapper189_init,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,Mapper225_init,Mapper226_init,Mapper227_init,Mapper228_init,
	Mapper229_init,0,0,Mapper232_init,0,Mapper234_init,Mapper43_init,0,
	0,0,0,Mapper240_init,0,Mapper242_init,0,0,
	Mapper245_init,Mapper246_init,0,Mapper248_init,Mapper249_init,Mapper250_init,0,0,0,0,0
};

static DECLFW(BWRAM)
{
                WRAM[A-0x6000]=V;
}

static DECLFR(AWRAM)
{
                return WRAM[A-0x6000];
}

void (*MapStateRestore)(int version);
void iNESStateRestore(int version)
{
 int x;
 
 if(!MapperNo) return;

 for(x=0;x<4;x++)
  setprg8(0x8000+x*8192,PRGBankList[x]);

 if(VROM_size)
  for(x=0;x<8;x++) 
    setchr1(0x400*x,CHRBankList[x]);

 switch(Mirroring)
 {
   case 0:setmirror(MI_H);break;
   case 1:setmirror(MI_V);break;
   case 0x12:
   case 0x10:setmirror(MI_0);break;
   case 0x13:
   case 0x11:setmirror(MI_1);break;
 }
 if(MapStateRestore) MapStateRestore(version);
}

static int MMC_init(int type)
{
        int x;

        GameStateRestore=iNESStateRestore;
        MapClose=0;
	MapperReset=0;
        MapStateRestore=0;

        setprg8r(1,0x6000,0);

        SetReadHandler(0x6000,0x7FFF,AWRAM);
        SetWriteHandler(0x6000,0x7FFF,BWRAM);
        FCEU_CheatAddRAM(8,0x6000,WRAM);

	/* This statement represents atrocious code.  I need to rewrite
	   all of the iNES mapper code... */
	IRQCount=IRQLatch=IRQa=0;
        if(head.ROM_type&2)
	{
	 extern uint8 MMC5WRAMsize,MMC1WRAMsize;
	 if(type==5 && MMC5WRAMsize==4)
	  memset(GameMemBlock+32768,0,sizeof(GameMemBlock)-32768);
	 else if(type==1 && MMC1WRAMsize==2)
	 {
	  memset(GameMemBlock,0,8192);
	  memset(GameMemBlock+16384,0,sizeof(GameMemBlock)-16384);
	 }
	 else
          memset(GameMemBlock+8192,0,sizeof(GameMemBlock)-8192);
	}
	else
         memset(GameMemBlock,0,sizeof(GameMemBlock));
        if(head.ROM_type&4)
	 memcpy(WRAM+4096,trainerpoo,512);

        NONE_init();

	if(FCEUGameInfo.type==GIT_VSUNI)
	 DoVSHooks();
        if(type==5)
        {
          MMC5HackVROMMask=CHRmask4[0];
          MMC5HackExNTARAMPtr=MapperExRAM+0x6000;
          MMC5Hack=1;
          MMC5HackVROMPTR=VROM;
          MMC5HackCHRMode=0;
        }
        ResetExState();
        AddExState(WRAM, 8192, 0, "WRAM");
        if(type==19 || type==5 || type==6 || type==69 || type==85)
         AddExState(MapperExRAM, 32768, 0, "MEXR");
        if((!VROM_size || type==6 || type==19 || type==119) &&
	   (type!=13 && type!=96))
         AddExState(CHRRAM, 8192, 0, "CHRR");
        if(head.ROM_type&8)
         AddExState(ExtraNTARAM, 2048, 0, "EXNR");

	/* Exclude some mappers whose emulation code handle save state stuff 
	   themselves. */
	if(type && type!=13 && type!=96)
	{
         AddExState(mapbyte1, 32, 0, "MPBY");
         AddExState(&Mirroring, 1, 0, "MIRR");
         AddExState(&IRQCount, 4, 1, "IRQC");
         AddExState(&IRQLatch, 4, 1, "IQL1");
         AddExState(&IRQa, 1, 0, "IRQA");
         AddExState(PRGBankList, 4, 0, "PBL");
         for(x=0;x<8;x++)
         {
          char tak[8];
          sprintf(tak,"CBL%d",x);         
          AddExState(&CHRBankList[x], 2, 1,tak);
         }
	}

        if(MapInitTab[type]) MapInitTab[type]();
        else if(type)
        {
         FCEU_PrintError("iNES mapper #%d is not supported at all.",type);
        }
        return 1;
}
