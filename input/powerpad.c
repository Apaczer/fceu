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

#include        <string.h>
#include	<stdlib.h>
#include	"share.h"


static uint32 pprsb[2];
static uint32 pprdata[2];

static uint8 FP_FASTAPASS(1) ReadPP(int w)
{
                uint8 ret=0;
                ret|=((pprdata[w]>>pprsb[w])&1)<<3;
                ret|=((pprdata[w]>>(pprsb[w]+8))&1)<<4;
		if(pprsb[w]>=4) 
		{
		 ret|=0x10;
		 if(pprsb[w]>=8) 
		  ret|=0x08;
		}
                pprsb[w]++;
                return ret;
}

static void FP_FASTAPASS(1) StrobePP(int w)
{
		pprsb[w]=0;
}

void FP_FASTAPASS(3) UpdatePP(int w, void *data, int arg)
{
        pprdata[w]=*(uint32 *)data;
}

static INPUTC PPC={ReadPP,0,StrobePP,UpdatePP,0,0};

INPUTC *FCEU_InitPowerpad(int w)
{
 pprsb[w]=pprdata[w]=0;
 return(&PPC);
}
