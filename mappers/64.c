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

#define cmd   mapbyte1[0]
#define mir   mapbyte1[1]
#define rmode mapbyte1[2]
#define regsl mapbyte2
#define regsh mapbyte3

static void RAMBO1_hb(void)
{
      rmode=0;
      if(IRQCount>=0)
      {
        IRQCount--;
        if(IRQCount<0)
        {
         if(IRQa)
         {
//	    printf("IRQ: %d\n",scanline);
            rmode = 1;
            X6502_IRQBegin(FCEU_IQEXT);
         }
        }
      }
}

static void Synco(void)
{
 int x;

 if(cmd&0x20)
 {
  setchr1(0x0000,regsl[0]);
  setchr1(0x0800,regsl[1]);
  setchr1(0x0400,regsh[0]);
  setchr1(0x0c00,regsh[1]);
 }
 else
 {
  setchr2(0x0000,regsl[0]>>1);
  setchr2(0x0800,regsl[1]>>1);
 }

 for(x=0;x<4;x++)
  setchr1(0x1000+x*0x400,regsl[2+x]);

 setprg8(0x8000,regsl[6]);
 setprg8(0xA000,regsl[7]);

 setprg8(0xC000,regsh[7]);
}


static DECLFW(RAMBO1_write)
{
 //if(A>=0xC000 && A<=0xFFFF) printf("$%04x:$%02x, %d, %d\n",A,V,scanline,timestamp);
 switch(A&0xF001)
 {
        case 0xa000:mir=V&1;
		    setmirror(mir^1);
		    break;
        case 0x8000:cmd = V;
		    break;
        case 0x8001:
		    if((cmd&15)<8)
		     regsl[cmd&7]=V;
		    else
		     regsh[cmd&7]=V;
		    Synco();
		    break;
        case 0xc000:IRQLatch=V;
                    if(rmode==1)
                     {
                      IRQCount=IRQLatch;
                     }
                    break;
        case 0xc001:rmode=1;
                    IRQCount=IRQLatch;
                    break;
        case 0xE000:IRQa=0;X6502_IRQEnd(FCEU_IQEXT);
                    if(rmode==1)
                     {IRQCount=IRQLatch;}
                    break;
        case 0xE001:IRQa=1;
                    if(rmode==1)
                     {IRQCount=IRQLatch;}
                    break;
  }	
}

static void RAMBO1_Restore(int version)
{
 if(version<74)
 {
  int x;

  x=mapbyte1[1];        // was MMC3_cmd
  cmd=x;

  regsl[0]=CHRBankList[0];
  regsl[1]=CHRBankList[2];
  regsh[0]=CHRBankList[1];
  regsh[1]=CHRBankList[3];

  for(x=0;x<4;x++)
   regsl[2+x]=CHRBankList[4+x];

  regsl[6]=PRGBankList[0];
  regsl[7]=PRGBankList[1];
  regsh[7]=PRGBankList[2];
  mir=Mirroring^1;
 }
 Synco();
 setmirror(mir^1);
}

void Mapper64_init(void)
{
	int x;

	for(x=0;x<8;x++)
	 regsl[x]=regsh[x]=~0;
	cmd=0;
	mir=0;
	setmirror(1);
	Synco();
	GameHBIRQHook=RAMBO1_hb;
	GameStateRestore=RAMBO1_Restore;
	SetWriteHandler(0x8000,0xffff,RAMBO1_write);
}
