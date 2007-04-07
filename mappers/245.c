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

static void Synco(void)
{
 ROM_BANK8(0x8000,mapbyte2[0]);
 ROM_BANK8(0xA000,mapbyte2[1]);
 ROM_BANK8(0xc000,0x3e);
 ROM_BANK8(0xe000,0x3f);
}
static DECLFW(Mapper245_write)
{
 switch(A&0xe001)
 {
  case 0xa000:mapbyte1[1]=V;Synco();break;
  case 0x8000:mapbyte1[0]=V;break;
  case 0x8001:switch(mapbyte1[0]&7)
		{
//		 default:printf("ark\n");break;
		 case 6:mapbyte2[0]=V;Synco();break;
		 case 7:mapbyte2[1]=V;Synco();break;
		}break;
  //case 0xa001:MIRROR_SET2(V>>7);break;
 }
// printf("$%04x:$%02x\n",A,V);
}

void Mapper245_init(void)
{
  SetWriteHandler(0x8000,0xffff,Mapper245_write);
}

