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



DECLFW(Mapper43_write)
{
  uint32 m;
  int z;

  if(A&0x400)
   onemir(0);
  else
   MIRROR_SET((A>>13)&1);
  m=A&0x1f;

  z=(A>>8)&3;

  switch(CHRmask8[0])
  {
   default:
   case 0xFF:
             if(z&2)
              m|=0x20;
             break;
   case 0x1FF:
             m|=z<<5;
             break;
  }

   if(A&0x800)
   {
    ROM_BANK16(0x8000,(m<<1)|((A&0x1000)>>12));
    ROM_BANK16(0xC000,(m<<1)|((A&0x1000)>>12));
   }
   else
    ROM_BANK32(m);
}

void Mapper43_init(void)
{
 ROM_BANK32(0);
 SetWriteHandler(0x8000,0xffff,Mapper43_write);
}
