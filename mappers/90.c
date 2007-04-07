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

#define tkcom1 mapbyte1[1]
#define tkcom2 mapbyte1[2]

#define prgb	mapbyte2
#define unkl	(mapbyte2+4)
#define chrlow  mapbyte3
#define chrhigh mapbyte4

static uint8 tekker=0x80;

static DECLFR(tekread)
{
 return tekker;
}

static void tekprom(void)
{
 switch(tkcom1&3)
  {
   case 1:              // 16 KB
          ROM_BANK16(0x8000,prgb[0]);
          ROM_BANK16(0xC000,prgb[2]);
          break;

   case 2:              //2 = 8 KB ??
   case 3:
          ROM_BANK8(0x8000,prgb[0]);
          ROM_BANK8(0xa000,prgb[1]);
          ROM_BANK8(0xc000,prgb[2]);
          ROM_BANK8(0xe000,prgb[3]);
          break;
  }
}

static void tekvrom(void)
{
 int x;
 switch(tkcom1&0x18)
  {
   case 0x00:      // 8KB
           VROM_BANK8(chrlow[0]|(chrhigh[0]<<8));
	   break;
   case 0x08:      // 4KB
          for(x=0;x<8;x+=4)
           VROM_BANK4(x<<10,chrlow[x]|(chrhigh[x]<<8));
	  break;
   case 0x10:      // 2KB
	  for(x=0;x<8;x+=2)
           VROM_BANK2(x<<10,chrlow[x]|(chrhigh[x]<<8));
	  break;
   case 0x18:      // 1KB
	   for(x=0;x<8;x++)
	    VROM_BANK1(x<<10,chrlow[x]|(chrhigh[x]<<8));
	   break;
 }
}

static DECLFW(Mapper90_write)
{
 A&=0xF007;

 if(A>=0x8000 && A<=0x8003)
 {
  prgb[A&3]=V;
  tekprom();
 }
 else if(A>=0x9000 && A<=0x9007)
 {
  chrlow[A&7]=V;
  tekvrom();
 }
 else if(A>=0xa000 && A<=0xa007)
 {
  chrhigh[A&7]=V;
  tekvrom();
 }
 else if(A>=0xb000 && A<=0xb003)
 {
  unkl[A&3]=V;
 }
 else switch(A)
 {
   case 0xc004:
   case 0xc000:IRQLatch=V;break;

   case 0xc005:
   case 0xc001:X6502_IRQEnd(FCEU_IQEXT);
               IRQCount=V;break;
   case 0xc006:
   case 0xc002:X6502_IRQEnd(FCEU_IQEXT);
               IRQa=0;
               IRQCount=IRQLatch;
               break;
   case 0xc007:
   case 0xc003:IRQa=1;break;

   case 0xd000:tkcom1=V;break;
   case 0xd001:switch(V&3){
                  case 0x00:MIRROR_SET(0);break;
                  case 0x01:MIRROR_SET(1);break;
                  case 0x02:onemir(0);break;
                  case 0x03:onemir(1);break;
               }
	       break;
   break;
 }
}

static void Mapper90_hb(void)
{
 if(IRQa)
 {
  if(IRQCount) 
  {
   IRQCount--;
   if(!IRQCount)
   {
    X6502_IRQBegin(FCEU_IQEXT);
    IRQCount=IRQLatch;
   }
  }
 }
}

void Mapper90_init(void)
{
  tekker^=0x80;
  SetWriteHandler(0x8000,0xffff,Mapper90_write);
  SetReadHandler(0x5000,0x5000,tekread);
  GameHBIRQHook=Mapper90_hb;
}

