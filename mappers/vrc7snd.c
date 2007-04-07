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

#include <stdio.h>
#include <stdlib.h>
#include "mapinc.h"
#include "fmopl.h"

static void VRC7_LoadInstrument(uint8 Chan);
void vrc7translate(uint8 Reg,uint8 V);

FM_OPL *fmob=0;
uint8 VRC7Chan[3][6];
static void InitOPL(void);

void OPL2_setreg(uint8 A, uint8 V)
{
 if(fmob) 
  OPLWrite(fmob,A,V);
}


void LoadOPL(void)
{
 int x;
 int y;

 for(x=y=0;x<0x40;x++)
  y|=MapperExRAM[x];
 if(y)
 { 
  InitOPL(); 
  for(x=0;x<6;x++)
  {
   VRC7_LoadInstrument(x); 
   vrc7translate(0x10+x,VRC7Chan[0][x]);
  }
 }
}

static int dwave=0;

void VRC7Update(void)
{
 int32 z,a;

 z=((timestamp<<16)/soundtsinc)>>4;  
 a=z-dwave;

 if(a && fmob)
  YM3812UpdateOne(fmob, &Wave[dwave], a);
 dwave+=a;
}

void UpdateOPL(int Count)
{
 int32 z,a;

 z=((timestamp<<16)/soundtsinc)>>4;
 a=z-dwave;

 if(fmob && a)
  YM3812UpdateOne(fmob, &Wave[dwave], a);

 dwave=0;
}

void KillOPL(void)
{
 if(fmob) OPLDestroy(fmob);
 fmob=0;
}

static void InitOPL(void)
{
        int x;

	if(!fmob)
	{	
         if(!( fmob=OPLCreate(OPL_TYPE_WAVESEL,1789772*2,FSettings.SndRate)))
	  return;
	}
	GameExpSound.Kill=KillOPL;
        OPLResetChip(fmob);

        for(x=0x1;x<0xF6;x++)
         OPL2_setreg(x,0);
        OPL2_setreg(0xBD,0xC0);
        OPL2_setreg(1,0x20);      /* Enable waveform type manipulation */
}

/*	This following code is in the public domain, but the author, Quietust(see	*/
/*	the "AUTHORS" file, would appreciate credit to go to him if this code */
/*	is used.		*/

uint8 VRC7Instrument[16][8] = {
	{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},	/* Custom instrument. */
	{0x03,0x01,0x14,0x80,0xC2,0x90,0x43,0x14},	/* Currently working on this one */
	{0x13,0x41,0x10,0x0B,0xFF,0xF2,0x32,0xD6},
	{0x01,0x01,0x10,0x08,0xF0,0xF4,0x00,0x04},	/* 90% perfect */
	{0x21,0x41,0x1B,0x08,0x66,0x80,0x30,0x85},
	{0x22,0x21,0x20,0x03,0x75,0x70,0x24,0x14},
	{0x02,0x01,0x06,0x00,0xF0,0xF2,0x03,0x95},	/* Do not touch! 98% perfect! */
	{0x21,0x41,0x18,0x10,0x93,0xE0,0x21,0x15},
	{0x01,0x22,0x13,0x00,0xF0,0x82,0x00,0x15},
	{0x05,0x01,0x22,0x00,0x60,0xE3,0xA0,0xF5},	/* 90% perfect */
	{0x85,0x01,0x20,0x00,0xD7,0xA2,0x22,0xF5},	/* 90% perfect */
	{0x07,0x81,0x2B,0x05,0xF4,0xF2,0x14,0xF4},	/* 95% perfect */
	{0x21,0x41,0x20,0x18,0xF3,0x80,0x13,0x95},
	{0x01,0x02,0x20,0x00,0xF9,0x92,0x41,0x75},	/* Do not touch! 98% perfect! */
	{0x21,0x62,0x0E,0x00,0x84,0x85,0x45,0x15},	/* 90% perfect */
	{0x21,0x62,0x0E,0x00,0xA1,0xA0,0x34,0x16}	/* Do not touch! 98% perfect! */
};

static uint8 InstTrans[6] = {0x00,0x01,0x02,0x08,0x09,0x0A};

static void VRC7_LoadInstrument(uint8 Chan)
{
	uint8 *i;
	uint8 x = InstTrans[Chan];
	uint8 y = (VRC7Chan[2][Chan] >> 4) & 0xF;
	
	i=VRC7Instrument[y];

	OPL2_setreg((0x20+x),i[0]);
	OPL2_setreg((0x23+x),i[1]);
	OPL2_setreg((0x40+x),i[2]);
	OPL2_setreg((0x43+x),((i[3] & 0xC0) 
		| ((VRC7Chan[2][Chan] << 2) & 0x3C)));	// quiet
	OPL2_setreg(0xe0+x,(i[3] >> 3) & 0x01);
	OPL2_setreg(0xe3+x,(i[3] >> 4) & 0x01);
	OPL2_setreg(0xC0+Chan,(i[3] << 1) & 0x0E);
	OPL2_setreg(0x60+x,i[4]);
	OPL2_setreg(0x63+x,i[5]);
	OPL2_setreg(0x80+x,i[6]);
	OPL2_setreg(0x83+x,i[7]);
}

void vrc7translate(uint8 Reg,uint8 V)
{
	uint8 x = Reg & 0x0F, y;
        if(!fmob) InitOPL();

	MapperExRAM[Reg]=V;

	VRC7Update();
	switch ((Reg & 0xF0) >> 4)
	{
	 case 0:
		if (x & 0x08) break;
		VRC7Instrument[0][x] = V;
		for (y = 0; y < 6; y++)
		 if (!(VRC7Chan[2][y]&0xF0))
		  VRC7_LoadInstrument(y);
		break;
	 case 1:
		if(x>5) break;
		VRC7Chan[0][x] = V;
		OPL2_setreg(0xA0 + x,(VRC7Chan[0][x] << 1) & 0xFE);
		OPL2_setreg(0xB0 + x,((VRC7Chan[0][x] >> 7) & 0x01) | ((VRC7Chan[1][x] << 1) & 0x3E));
		break;
	 case 2:
		if(x>5) break;
		VRC7Chan[1][x] = V;
		OPL2_setreg(0xB0 + x,(((VRC7Chan[0][x] >> 7) & 0x01) | ((VRC7Chan[1][x] << 1) & 0x3E)));
		break;
	 case 3:
		if(x>5) break;
		VRC7Chan[2][x] = V;
		VRC7_LoadInstrument(x);
		break;
	}
}
