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

#include "mapinc.h"

#define MMC1_reg mapbyte1
#define MMC1_buf mapbyte2[0]
#define MMC1_sft mapbyte3[0]
#define lastn    mapbyte2[1]

#define NWCDIP 0xE

static void FP_FASTAPASS(1) NWCIRQHook(int a)
{
	if(!(MMC1_reg[1]&0x10))
	{
	 IRQCount+=a;
 	 if((IRQCount|(NWCDIP<<25))>=0x3e000000)
          {
	   IRQCount=0;
	   X6502_IRQBegin(FCEU_IQEXT);
	  }
	}
}


static void MMC1PRG(void)
{
	if(MMC1_reg[1]&8)
	{	
        switch(MMC1_reg[0]&0xC)
         { 
           case 0xC: ROM_BANK16(0x8000,8+(MMC1_reg[3]&7));
                     ROM_BANK16(0xC000,15);
                     break;
           case 0x8: ROM_BANK16(0xC000,8+((MMC1_reg[3])&7));
                     ROM_BANK16(0x8000,8);
                     break;
           case 0x0:
           case 0x4:
                     ROM_BANK16(0x8000,8+(MMC1_reg[3]&6));
                     ROM_BANK16(0xc000,8+((MMC1_reg[3]&6)+1));
                     break;
         }
	}
	else
	{
	 ROM_BANK32((MMC1_reg[1]>>1)&3);
	}
}

DECLFW(Mapper105_write)
{
        int n=(A>>13)-4;

        if (V&0x80)
        {
         MMC1_sft=MMC1_buf=0;
         return;
        }

        if(lastn!=n)
        {
         MMC1_sft=MMC1_buf=0;
        }
        lastn=n;

	//MMC1_reg[n]&=~((1)<<(MMC1_sft));
        MMC1_buf|=(V&1)<<(MMC1_sft++);

  if (MMC1_sft==5) 
	{
        if(n==3) V&=0xF;
        else     V&=0x1F;

        MMC1_reg[n]=V=MMC1_buf;
        MMC1_sft = MMC1_buf=0;

        switch(n){
        case 0:
                switch(MMC1_reg[0]&3)
                {
                 case 2: MIRROR_SET(0);break;
                 case 3: MIRROR_SET(1);break;
                 case 0: onemir(0);break;
                 case 1: onemir(1);break;
                }
                MMC1PRG();
                break;
        case 1:
		if(MMC1_reg[1]&0x10)
		 {IRQCount=0;X6502_IRQEnd(FCEU_IQEXT);}
                MMC1PRG();
                break;
        case 3:
                MMC1PRG();
                break;
        }
  }
}


void Mapper105_init(void)
{
        int i;
        for(i=0;i<4;i++) MMC1_reg[i]=0;
        MMC1_sft = MMC1_buf =0;
        MMC1_reg[0]=0xC;
        ROM_BANK32(0);
	SetWriteHandler(0x8000,0xFFFF,Mapper105_write);
	MapIRQHook=NWCIRQHook;
}

