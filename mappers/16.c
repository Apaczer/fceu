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

static void FP_FASTAPASS(1) BandaiIRQHook(int a)
{
  if(IRQa)
  {
   IRQCount-=a;
   if(IRQCount<0)
   {
    X6502_IRQBegin(FCEU_IQEXT);
    //printf("IRQ: %d, %d\n",scanline,timestamp);
    IRQa=0;
    IRQCount=0xFFFF;
   }
  }
}

static DECLFW(Mapper16_write)
{
        A&=0xF;

        if(A<=0x7)
         VROM_BANK1(A<<10,V);
        else if(A==0x8)
         ROM_BANK16(0x8000,V);
        else switch(A) {
         case 0x9: switch(V&3) {
                    case 0x00:MIRROR_SET2(1);break;
                    case 0x01:MIRROR_SET2(0);break;
                    case 0x02:onemir(0);break;
                    case 0x03:onemir(1);break;
                   }
                   break;
         case 0xA:X6502_IRQEnd(FCEU_IQEXT);
                  IRQa=V&1;
                  IRQCount=IRQLatch;
                  break;
         case 0xB:IRQLatch&=0xFF00;
                  IRQLatch|=V;
                  break;
         case 0xC:IRQLatch&=0xFF;
                  IRQLatch|=V<<8;
                  break;
         case 0xD: break;/* Serial EEPROM control port */
 }
}

// Famicom jump 2:
// 0-7: Lower bit of data selects which 256KB PRG block is in use.
// This seems to be a hack on the developers' part, so I'll make emulation
// of it a hack(I think the current PRG block would depend on whatever the
// lowest bit of the CHR bank switching register that corresponds to the
// last CHR address read).

static void PRGO(void)
{
 uint32 base=(mapbyte1[0]&1)<<4;
 ROM_BANK16(0x8000,(mapbyte2[0]&0xF)|base);
 ROM_BANK16(0xC000,base|0xF);
}

static DECLFW(Mapper153_write)
{
	A&=0xF;
        if(A<=0x7) 
	{
	 mapbyte1[A&7]=V;
	 PRGO();
	}
        else if(A==0x8) 
	{
	 mapbyte2[0]=V;
	 PRGO();
	}
	else switch(A) {
	 case 0x9: switch(V&3) {
       	            case 0x00:MIRROR_SET2(1);break;
                    case 0x01:MIRROR_SET2(0);break;
                    case 0x02:onemir(0);break;
                    case 0x03:onemir(1);break;
                   }
                   break;
         case 0xA:X6502_IRQEnd(FCEU_IQEXT);
  		  IRQa=V&1;
		  IRQCount=IRQLatch;
		  break;
         case 0xB:IRQLatch&=0xFF00;
		  IRQLatch|=V;
 		  break;
         case 0xC:IRQLatch&=0xFF;
 		  IRQLatch|=V<<8;
		  break;
        }
}

void Mapper16_init(void)
{
 MapIRQHook=BandaiIRQHook;
 SetWriteHandler(0x6000,0xFFFF,Mapper16_write);
}

void Mapper153_init(void)
{
 MapIRQHook=BandaiIRQHook;
 SetWriteHandler(0x8000,0xFFFF,Mapper153_write);
 /* This mapper/board seems to have WRAM at $6000-$7FFF, so I'll let the
    main ines code take care of that memory region. */
}
