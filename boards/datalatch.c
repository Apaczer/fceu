/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2002 Xodnizel
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

static uint8 latche;

//------------------ CPROM ---------------------------

static DECLFW(CPROMWrite)
{
  latche=V&3;
  setchr4(0x1000,latche);
}

static void CPROMReset(void)
{
  setprg32(0x8000,0);
  setchr8(0);
  SetReadHandler(0x8000,0xFFFF,CartBR);
  SetWriteHandler(0x8000,0xffff,CPROMWrite);
}

static void CPROMRestore(int version)
{
  setchr4(0x1000,latche);
}

void CPROM_Init(CartInfo *info)
{
  info->Power=CPROMReset;
  GameStateRestore=CPROMRestore;
  AddExState(&latche, 1, 0, "LATC");
}

//------------------ CNROM ---------------------------

DECLFW(CNROMWrite)
{
  latche=V&3;
  setchr8(latche);
}

static void CNROMReset(void)
{
  setprg16(0x8000,0);
  setprg16(0xC000,1);
  SetReadHandler(0x8000,0xFFFF,CartBR);
  SetWriteHandler(0x8000,0xffff,CNROMWrite);
}

static void CNROMRestore(int version)
{
  setchr8(latche);
}

void CNROM_Init(CartInfo *info)
{
  info->Power=CNROMReset;
  GameStateRestore=CNROMRestore;
  AddExState(&latche, 1, 0, "LATC");
}

//------------------ NROM128 ---------------------------

static void NROM128Reset(void)
{
  setprg16(0x8000,0);
  setprg16(0xC000,0);
  setchr8(0);
  SetReadHandler(0x8000,0xFFFF,CartBR);
}

void NROM128_Init(CartInfo *info)
{
  info->Power=NROM128Reset;
}

//------------------ NROM256 ---------------------------

static void NROM256Reset(void)
{
  setprg16(0x8000,0);
  setprg16(0xC000,1);
  setchr8(0);
  SetReadHandler(0x8000,0xFFFF,CartBR);
}

void NROM256_Init(CartInfo *info)
{
  info->Power=NROM256Reset;
}

//------------------ MHROM ---------------------------

static DECLFW(MHROMWrite)
{
  setprg32(0x8000,V>>4);
  setchr8(V);
  latche=V;
}

static void MHROMReset(void)
{
  setprg32(0x8000,0);
  setchr8(0);
  latche=0;
  SetReadHandler(0x8000,0xFFFF,CartBR);
}

static void MHROMRestore(int version)
{
  setprg32(0x8000,latche);
  setchr8(latche);
  SetWriteHandler(0x8000,0xffff,MHROMWrite);
}

void MHROM_Init(CartInfo *info)
{ 
  info->Power=MHROMReset;
  AddExState(&latche, 1, 0,"LATC");
  PRGmask32[0]&=1;
  CHRmask8[0]&=1;
  GameStateRestore=MHROMRestore;
}

//------------------ UNROM ---------------------------

static void UNROMRestore(int version)
{
  setprg16(0x8000,latche);
}

static DECLFW(UNROMWrite)
{
  setprg16(0x8000,V);
  latche=V;
}

static void UNROMReset(void)
{
  setprg16(0x8000,0);
  setprg16(0xc000,~0);
  setchr8(0);
  SetWriteHandler(0x8000,0xffff,UNROMWrite);
  SetReadHandler(0x8000,0xFFFF,CartBR);
  latche=0;
}

void UNROM_Init(CartInfo *info)
{
  info->Power=UNROMReset;
  PRGmask16[0]&=7;
  AddExState(&latche, 1, 0, "LATC");
  GameStateRestore=UNROMRestore;
}

//------------------ GNROM ---------------------------

static void GNROMSync()
{
  setchr8(latche&3);
  setprg32(0x8000,(latche>>4)&3);
}

static DECLFW(GNROMWrite)
{
  latche=V&0x33;
  GNROMSync();
}

static void GNROMStateRestore(int version)
{
  GNROMSync();
}

static void GNROMReset(void)
{
  latche=0;
  GNROMSync();
  SetWriteHandler(0x8000,0xffff,GNROMWrite);
  SetReadHandler(0x8000,0xFFFF,CartBR);
}

void GNROM_Init(CartInfo *info)
{
  info->Power=GNROMReset;
  AddExState(&latche, 1, 0, "LATC");
  GameStateRestore=GNROMStateRestore;
}
