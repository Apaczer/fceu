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

#include        "share.h"

typedef struct {
        uint32 mzx,mzy,mzb;
        int zap_readbit;
        int bogo;
        uint32 colok;
        uint32 coloklast;
} ZAPPER;

static ZAPPER ZD;

static void FP_FASTAPASS(2) ZapperThingy(uint8 *buf, int line)
{
	int mzx=ZD.mzx;

	if(line==0) ZD.colok=1<<16;	/* Disable it. */
	ZD.coloklast=ZD.colok;

	if((line>=ZD.mzy-3 && line<=ZD.mzy+3) && mzx<256)
	{
	 int a,sum,x;

	 for(x=-4;x<4;x++)
	 {
	  if((mzx+x)<0 || (mzx+x)>255) continue;
	  a=buf[mzx+x]&63;
	  sum=palo[a].r+palo[a].g+palo[a].b;
	 
	  if(sum>=100*3)
	  {
	   ZD.colok=timestamp+mzx/3;
	   break;
	  }
	 }
	}
}

static INLINE int CheckColor(void)
{
  if( (timestamp>=ZD.coloklast && timestamp<=(ZD.coloklast+10)) ||
   (timestamp>=ZD.colok && timestamp<=(ZD.colok+10)) )
   return 0;
  return 1;
}

static uint8 FP_FASTAPASS(2) ReadZapper(int w, uint8 ret)
{
		if(w)
		{
		 ret&=~0x18;
                 if(ZD.bogo)
                  ret|=0x10;
                 if(CheckColor())
                  ret|=0x8;
		}
		else
		{
		 //printf("Kayo: %d\n",ZD.zap_readbit);
		 ret&=~2;
		 //if(ZD.zap_readbit==4) ret|=ZD.mzb&2;
		 ret|=(ret&1)<<1;
		 //ZD.zap_readbit++;
		}
                return ret;
}

static void FP_FASTAPASS(2) DrawZapper(uint8 *buf, int arg)
{
 if(arg)
  FCEU_DrawCursor(buf, ZD.mzx, ZD.mzy);
}

static void FP_FASTAPASS(2) UpdateZapper(void *data, int arg)
{
  uint32 *ptr=data;

  if(ZD.bogo)
   ZD.bogo--;
  if(ptr[2]&1 && (!(ZD.mzb&1)))
   ZD.bogo=5;

  ZD.mzx=ptr[0];
  ZD.mzy=ptr[1];
  ZD.mzb=ptr[2];

  if(ZD.mzx>=256 || ZD.mzy>=240)
   ZD.colok=0;
}

static void StrobeShadow(void)
{
 ZD.zap_readbit=0;
}

static INPUTCFC SHADOWC={ReadZapper,0,StrobeShadow,UpdateZapper,ZapperThingy,DrawZapper};

INPUTCFC *FCEU_InitSpaceShadow(void)
{
  memset(&ZD,0,sizeof(ZAPPER));
  return(&SHADOWC);
}


