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



DECLFW(Mapper80_write)
{
switch(A)
 {
  case 0x7ef0: VROM_BANK1(0x000,V);VROM_BANK1(0x400,(V+1));break;
  case 0x7ef1: VROM_BANK1(0x800,V);VROM_BANK1(0xC00,(V+1));break;

  case 0x7ef2: VROM_BANK1(0x1000,V);break;
  case 0x7ef3: VROM_BANK1(0x1400,V);break;
  case 0x7ef4: VROM_BANK1(0x1800,V);break;
  case 0x7ef5: VROM_BANK1(0x1c00,V);break;
  case 0x7efa:
  case 0x7efb: ROM_BANK8(0x8000,V);break;
  case 0x7efd:
  case 0x7efc: ROM_BANK8(0xA000,V);break;
  case 0x7efe:
  case 0x7eff: ROM_BANK8(0xC000,V);break;
 }
}

void Mapper80_init(void)
{
SetWriteHandler(0x4020,0x7fff,Mapper80_write);
}

