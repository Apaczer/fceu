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


#define dbarray mapbyte1
static void FP_FASTAPASS(1) dragonbust_ppu(uint32 A)
{
 static int last=-1;
 static uint8 z;

 if(A>=0x2000) return;

 A>>=13;

 z=dbarray[A];

 if(z!=last)
 {
  onemir(z);
  last=z;
 }
}


DECLFW(Mapper95_write)
{
        switch(A&0xF001){

        case 0x8000:
        MMC3_cmd = V;
        break;

        case 0x8001:
                switch(MMC3_cmd&7){
                case 0: dbarray[0]=dbarray[1]=(V&0x20)>>4;onemir((V&0x20)>>4);V>>=1;VROM_BANK2(0x0000,V);break;
                case 1: dbarray[2]=dbarray[3]=(V&0x20)>>4;onemir((V&0x20)>>4);V>>=1;VROM_BANK2(0x0800,V);break;
                case 2: dbarray[4]=(V&0x20)>>4;onemir((V&0x20)>>4);VROM_BANK1(0x1000,V); break;
                case 3: dbarray[5]=(V&0x20)>>4;onemir((V&0x20)>>4);VROM_BANK1(0x1400,V); break;
                case 4: dbarray[6]=(V&0x20)>>4;onemir((V&0x20)>>4);VROM_BANK1(0x1800,V); break;
                case 5: dbarray[7]=(V&0x20)>>4;onemir((V&0x20)>>4);VROM_BANK1(0x1C00,V); break;
                case 6:
                        ROM_BANK8(0x8000,V);
                        break;
                case 7:
                        ROM_BANK8(0xA000,V);
                        break;
                }
                break;
}
}

void Mapper95_init(void)
{
  SetWriteHandler(0x8000,0xffff,Mapper95_write);
  PPU_hook=dragonbust_ppu;
}

