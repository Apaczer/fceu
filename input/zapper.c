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

static ZAPPER ZD[2];

static void FP_FASTAPASS(3) ZapperThingy(int w, uint8 *buf, int line)
{
	int mzx=ZD[w].mzx;

        if(line==0) ZD[w].colok=1<<16;     /* Disable it. */

	ZD[w].coloklast=ZD[w].colok;

	if(ZD[w].mzb&2) return;
        if((line>=ZD[w].mzy-3 && line<=ZD[w].mzy+3) && mzx<256)
        {
         int a,sum,x;

         for(x=-4;x<4;x++)
         {
          if((mzx+x)<0 || (mzx+x)>255) continue;
          a=buf[mzx+x]&63;
          sum=palo[a].r+palo[a].g+palo[a].b;

          if(sum>=100*3)
          {
           ZD[w].colok=timestamp+mzx/3;
           break;
          }
         }
        }

}

static INLINE int CheckColor(int w)
{
  if( (timestamp>=ZD[w].coloklast && timestamp<=(ZD[w].coloklast+100)) ||
   (timestamp>=ZD[w].colok && timestamp<=(ZD[w].colok+100)) )
   return 0;
  return 1;
}

static uint8 FP_FASTAPASS(1) ReadZapperVS(int w)
{
                uint8 ret=0;

                if(ZD[w].zap_readbit==4) ret=1;

                if(ZD[w].zap_readbit==7)
                {
                 if(ZD[w].bogo)
                  ret|=0x1;
                }
                if(ZD[w].zap_readbit==6)
                {
                 if(!CheckColor(w))
                  ret|=0x1;
                }
                ZD[w].zap_readbit++; 
                return ret;
}

static void FP_FASTAPASS(1) StrobeZapperVS(int w)
{                        
			ZD[w].zap_readbit=0;
}

static uint8 FP_FASTAPASS(1) ReadZapper(int w)
{
                uint8 ret=0;
                if(ZD[w].bogo)
                 ret|=0x10;
                if(CheckColor(w))
                 ret|=0x8;
                return ret;
}

static void FASTAPASS(3) DrawZapper(int w, uint8 *buf, int arg)
{
 if(arg)
  FCEU_DrawCursor(buf, ZD[w].mzx,ZD[w].mzy);
}

static void FP_FASTAPASS(3) UpdateZapper(int w, void *data, int arg)
{
  uint32 *ptr=data;

  if(ZD[w].bogo)
   ZD[w].bogo--;
  if(ptr[2]&3 && (!(ZD[w].mzb&3)))
   ZD[w].bogo=5;

  ZD[w].mzx=ptr[0];
  ZD[w].mzy=ptr[1];
  ZD[w].mzb=ptr[2];

  if(ZD[w].mzb&2 || ZD[w].mzx>=256 || ZD[w].mzy>=240)
   ZD[w].colok=0;
}

static INPUTC ZAPC={ReadZapper,0,0,UpdateZapper,ZapperThingy,DrawZapper};
static INPUTC ZAPVSC={ReadZapperVS,0,StrobeZapperVS,UpdateZapper,ZapperThingy,DrawZapper};

INPUTC *FCEU_InitZapper(int w)
{
  memset(&ZD[w],0,sizeof(ZAPPER));
  if(FCEUGameInfo.type==GIT_VSUNI)
   return(&ZAPVSC);
  else
   return(&ZAPC);
}


