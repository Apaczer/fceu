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




DECLFW(Mapper15_write)
{
switch(A)
 {
  case 0x8000:
        if(V&0x80)
        {
        ROM_BANK8(0x8000,(V<<1)+1);
        ROM_BANK8(0xA000,(V<<1));
        ROM_BANK8(0xC000,(V<<1)+2);
        ROM_BANK8(0xE000,(V<<1)+1);
        }
        else
        {
        ROM_BANK16(0x8000,V);
        ROM_BANK16(0xC000,V+1);
        }
        MIRROR_SET((V>>6)&1);
        break;
  case 0x8001:
        MIRROR_SET(0);
        ROM_BANK16(0x8000,V);
        ROM_BANK16(0xc000,~0);
        break;
  case 0x8002:
        if(V&0x80)
        {
         ROM_BANK8(0x8000,((V<<1)+1));
         ROM_BANK8(0xA000,((V<<1)+1));
         ROM_BANK8(0xC000,((V<<1)+1));
         ROM_BANK8(0xE000,((V<<1)+1));
        }
        else
        {
         ROM_BANK8(0x8000,(V<<1));
         ROM_BANK8(0xA000,(V<<1));
         ROM_BANK8(0xC000,(V<<1));
         ROM_BANK8(0xE000,(V<<1));
        }
        break;
  case 0x8003:
        MIRROR_SET((V>>6)&1);
        if(V&0x80)
        {
         ROM_BANK8(0xC000,(V<<1)+1);
         ROM_BANK8(0xE000,(V<<1));
        }
        else
        {
         ROM_BANK16(0xC000,V);
        }
        break;
 }
}

void Mapper15_init(void)
{
        ROM_BANK32(0);
	SetWriteHandler(0x8000,0xFFFF,Mapper15_write);
}

