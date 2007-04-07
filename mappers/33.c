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

static DECLFW(Mapper33_write)
{
        A&=0xF003;

	//printf("$%04x:$%02x, %d\n",A,V,scanline);
        if(A>=0xA000 && A<=0xA003)
         VROM_BANK1(0x1000+((A&3)<<10),V);
        else switch(A){
        case 0x8000:if(!mapbyte1[0])
                     MIRROR_SET((V>>6)&1);
                     ROM_BANK8(0x8000,V);
                    break;
        case 0x8001:ROM_BANK8(0xA000,V); break;
        case 0x8002:VROM_BANK2(0x0000,V);break;
        case 0x8003:VROM_BANK2(0x0800,V);break;
	case 0xc000:IRQLatch=V;break;
	case 0xc001:IRQCount=IRQLatch;break;
        case 0xc003:IRQa=0;break;
        case 0xc002:IRQa=1;break;
        case 0xe000:mapbyte1[0]=1;MIRROR_SET((V>>6)&1);break;
        }
}

static void heho(void)
{
 if(IRQa)
 {
   IRQCount++;
   if(IRQCount==0x100) 
   {
    TriggerIRQ();
    IRQa=0;
   }
 }
}

void Mapper33_init(void)
{
	SetWriteHandler(0x8000,0xffff,Mapper33_write);
	GameHBIRQHook=heho;
}
