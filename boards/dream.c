/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2005 CaH4e3
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

static uint8 latch;

static void DoPRG(void)
{
  setprg16(0x8000,latch);
  setprg16(0xC000,8);
}

static DECLFW(DREAMWrite)
{
  latch=V&7;
  DoPRG();
}

static void DREAMPower(void)
{
  latch=0;
  SetReadHandler(0x8000,0xFFFF,CartBR);
  SetWriteHandler(0x5020,0x5020,DREAMWrite);
  setchr8(0);
  DoPRG();
}

static void Restore(int version)
{
  DoPRG();
}

void DreamTech01_Init(CartInfo *info)
{
  GameStateRestore=Restore;
  info->Power=DREAMPower;
  AddExState(&latch, 1, 0, "LATCH");
}
