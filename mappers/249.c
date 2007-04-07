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

uint8 doh;

static DECLFW(Mapper249_write)
{
 switch(A&0xe001)
 {
  case 0x8000:doh=V;break;
  case 0x8001:switch(doh&7)
		{
		 case 0:VROM_BANK2(0x0000,V>>1);break;
		 case 1:VROM_BANK2(0x0800,V>>1);break;
		 case 2:VROM_BANK1(0x1000,V);break;
//		 case 6:ROM_BANK8(0xa000,V);break;
//		 case 2:ROM_BANK8(0x8000,V);break;
		}
 }
// printf("$%04x:$%02x\n",A,V);
}

void Mapper249_init(void)
{
  SetWriteHandler(0x8000,0xffff,Mapper249_write);
}

