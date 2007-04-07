/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 1998 \Firebug\
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
#include <string.h>
#include <vga.h>
#include <sys/io.h>

#define inportb inb
#define outportb(port, value) outb(value, port)
#define outportw(port, value) outw(value, port)

#include "main.h"
#include "svgalib.h"
#include "svga-video.h"


int vmode=1;

#ifdef FRAMESKIP
int FCEUDfskip=0;
#endif

static int vidready=0;
static int conlock=0;

void LockConsole(void)
{
 if(!conlock)
 {
  vga_lockvc();
  conlock=1;
  FCEUI_DispMessage("Console locked.");
 }
}

void UnlockConsole(void)
{
 if(conlock)
 {
  vga_unlockvc();
  conlock=0;
  FCEUI_DispMessage("Console unlocked.");
 }
}

void SetBorder(void)
{
 if(!conlock) 
  vga_lockvc();
 inportb(0x3da);
 outportb(0x3c0,(0x11|0x20));
 outportb(0x3c0,0x80);
 if(!conlock) 
  vga_unlockvc();
}

#include "vgatweak.c"

void TweakVGA(int VGAMode)
{
  int I;

  if(!conlock)
   vga_lockvc();

  outportb(0x3C8,0x00);
  for(I=0;I<768;I++) outportb(0x3C9,0x00);

  outportb(0x3D4,0x11);
  I=inportb(0x3D5)&0x7F;
  outportb(0x3D4,0x11);
  outportb(0x3D5,I);

  switch(VGAMode)
  {
    case 1:  for(I=0;I<25;I++) VGAPortSet(v256x240[I]);break;
    case 2:  for(I=0;I<25;I++) VGAPortSet(v256x256[I]);break;
    case 3:  for(I=0;I<25;I++) VGAPortSet(v256x256S[I]);break;
    case 6:  for(I=0;I<25;I++) VGAPortSet(v256x224S[I]);break;
    case 8:  for(I=0;I<25;I++) VGAPortSet(v256x224_103[I]);break;
    default: break;
  }

  outportb(0x3da,0);
  if(!conlock) 
   vga_unlockvc();
}


static uint8 palettedbr[256],palettedbg[256],palettedbb[256];

static void FlushPalette(void)
{
 int x;
 for(x=0;x<256;x++)
 {
  int z=x;
  if(vmode==4 || vmode==5 || vmode==7) z^=0x80;
  vga_setpalette(z,palettedbr[x]>>2,palettedbg[x]>>2,palettedbb[x]>>2);
 }
}

void FCEUD_SetPalette(uint8 index, uint8 r, uint8 g, uint8 b)
{
  palettedbr[index]=r;
  palettedbg[index]=g;
  palettedbb[index]=b;

  if(vidready)
  {
   if(vmode==4 || vmode==5 || vmode==7) index^=0x80;
   vga_setpalette(index,r>>2,g>>2,b>>2);
  }
}


void FCEUD_GetPalette(uint8 i, uint8 *r, uint8 *g, uint8 *b)
{
 *r=palettedbr[i];
 *g=palettedbg[i];
 *b=palettedbb[i];
}

static void vcfix(void)
{
 int z;

 if(!conlock)
  vga_lockvc();
 z=inportb(0x3cc);
 if(!conlock) 
  vga_unlockvc();
 if(z!=0xe3 && z!=0xe7)		// Common value in all tweaked video modes(and not in 320x200 mode).
 {
  TweakVGA(vmode);
  SetBorder();
  FlushPalette();
 }
}

static uint8 *ScreenLoc;

int InitVideo(void)
{
 #ifdef DUMMY
 return(1);
 #endif
 vidready=0;

 if(vmode<=3 || vmode==6 || vmode==8)
 {
  if(vga_getcurrentchipset()==FBDEV)
  {
   puts("Tweaked VGA video modes will not work.  Using a 320x240 video mode instead...");
   vmode=7;
  }
 }

 switch(vmode)
 {
  default:
  case 1:
  case 2:
  case 3:
  case 6:
  case 8:
	 vga_setmode(G320x200x256);
	 vidready|=1;
         ScreenLoc=vga_getgraphmem();
         TweakVGA(vmode);
         SetBorder();
         memset(ScreenLoc,128,256*256);
         break;
  case 4:
  case 5:
         if(!(vga_getmodeinfo(G640x480x256)->flags & CAPABLE_LINEAR))
         {
          puts("Video:  No linear addressing mode available!");
          return 0;
         }
         if(vga_setmode(G640x480x256)==-1)
         {
          puts("Video:  Could not set 640x480x8bpp video mode!");
          return 0;
         }
	 vidready|=1;

         vga_setpage(0);
         if(vga_setlinearaddressing()!=-1)
          ScreenLoc=vga_getgraphmem();
         else
         {
          puts("Video:  Could not set linear addressing!");
          return 0;
         }
         memset(ScreenLoc,0,640*480);
         break;
  case 7:
         if(!(vga_getmodeinfo(G320x240x256V)->flags & CAPABLE_LINEAR))
         {
          puts("Video:  No linear addressing mode available!");
          return 0;
         }
         if(vga_setmode(G320x240x256V)==-1)
         {
          puts("Video:  Could not set 320x240x8bpp video mode!");
          return 0;
         }
         vidready|=1;

         vga_setpage(0);
         if(vga_setlinearaddressing()!=-1)
          ScreenLoc=vga_getgraphmem();
         else
         {
          puts("Video:  Could not set linear addressing!");
          return 0;
         }
         memset(ScreenLoc,0,320*240);
         break;
 }
 vidready|=2;
 FlushPalette(); // Needed for cheat console code(and it isn't a bad thing to do anyway...).
 return 1;
}

void KillVideo(void)
{
 if(vidready)
 {
  vga_setmode(TEXT);
  vidready=0;
 }
}


void BlitScreen(uint8 *XBuf)
{
 static int conto=0;
 uint8 *dest;
 int tlines;
 #ifdef DUMMY
 return;
 #endif
 #ifdef FRAMESKIP
 FCEUI_FrameSkip(FCEUDfskip);
 #endif

 if(doptions&DO_VSYNC && !NoWaiting)
 {
  vga_waitretrace();
 }

 tlines=erendline-srendline+1;

 dest=ScreenLoc;

 if(vmode!=4 && vmode!=5 && vmode!=7)
 {
  conto=(conto+1)&0x3F;
  if(!conto) vcfix();
 }
 switch(vmode)
 {
  case 1:dest+=(((240-tlines)>>1)<<8);break;
  case 2:
  case 3:dest+=(((256-tlines)>>1)<<8);break;
  case 4:
  case 5:dest+=(((240-tlines)>>1)*640+((640-512)>>1));break;
  case 8:
  case 6:if(tlines>224) tlines=224;dest+=(((224-tlines)>>1)<<8);break;
  case 7:dest+=(((240-tlines)>>1)*320)+32;break;
 }

 XBuf+=(srendline<<8)+(srendline<<4);

 if(eoptions&EO_CLIPSIDES)
 {
  if(vmode==5)
  {
   asm volatile(
     "xorl %%edx, %%edx\n\t"
     "ckoop1:\n\t"
     "movb $120,%%al     \n\t"
     "ckoop2:\n\t"
     "movb 1(%%esi),%%dl\n\t"
     "shl  $16,%%edx\n\t"
     "movb (%%esi),%%dl\n\t"
     "xorl $0x00800080,%%edx\n\t"
     "movl %%edx,(%%edi)\n\t"
     "addl $2,%%esi\n\t"
     "addl $4,%%edi\n\t"
     "decb %%al\n\t"
     "jne ckoop2\n\t"

     "addl $32,%%esi\n\t"
     "addl $800,%%edi\n\t"
     "decb %%bl\n\t"
     "jne ckoop1\n\t"
     :
     : "S" (XBuf+8), "D" (dest+8), "b" (tlines)
     : "%al", "%edx", "%cc" );
  }
  else if(vmode==4)
  {
   asm volatile(
     "cyoop1:\n\t"
     "movb $120,%%al     \n\t"
     "cyoop2:\n\t"
     "movb 1(%%esi),%%dh\n\t"
     "movb %%dh,%%dl\n\t"
     "shl  $16,%%edx\n\t"
     "movb (%%esi),%%dl\n\t"
     "movb %%dl,%%dh\n\t"               // Ugh
     "xorl $0x80808080,%%edx\n\t"
     "movl %%edx,(%%edi)\n\t"
     "addl $2,%%esi\n\t"
     "addl $4,%%edi\n\t"
     "decb %%al\n\t"
     "jne cyoop2\n\t"

     "addl $32,%%esi\n\t"
     "addl $800,%%edi\n\t"
     "decb %%bl\n\t"
     "jne cyoop1\n\t"
     :
     : "S" (XBuf+8), "D" (dest+8), "b" (tlines)
     : "%al", "%edx", "%cc" );
  }
  else if(vmode==7)
  {
   asm volatile(
     "cgoop81:\n\t"
     "movl $30,%%eax\n\t"
     "cgoop82:\n\t"
     "movl (%%esi),%%edx\n\t"
     "movl 4(%%esi),%%ecx\n\t"
     "xorl $0x80808080,%%edx\n\t"
     "xorl $0x80808080,%%ecx\n\t"
     "movl %%edx,(%%edi)\n\t"
     "movl %%ecx,4(%%edi)\n\t"
     "addl $8,%%esi\n\t"
     "addl $8,%%edi\n\t"
     "decl %%eax\n\t"
     "jne cgoop82\n\t"
     "addl $80,%%edi\n\t"
     "addl $32,%%esi\n\t"
     "decb %%bl\n\t"
     "jne cgoop81\n\t"
     :
     : "S" (XBuf+8), "D" (dest+8), "b" (tlines)
     : "%eax","%cc","%edx","%ecx" );
  }
  else
  {
   asm volatile(
     "cgoop1:\n\t"
     "movl $30,%%eax\n\t"
     "cgoop2:\n\t"
     "movl (%%esi),%%edx\n\t"
     "movl 4(%%esi),%%ecx\n\t"
     "movl %%edx,(%%edi)\n\t"
     "movl %%ecx,4(%%edi)\n\t"
     "addl $8,%%esi\n\t"
     "addl $8,%%edi\n\t"
     "decl %%eax\n\t"
     "jne cgoop2\n\t"
     "addl $32,%%esi\n\t"
     "addl $16,%%edi\n\t"
     "decb %%bl\n\t"
     "jne cgoop1\n\t"
     :
     : "S" (XBuf+8), "D" (dest+8), "b" (tlines)
     : "%eax","%cc","%edx","%ecx" );
  }
 }
 else
 {
  if(vmode==5)
  {
   asm volatile(
     "xorl %%edx, %%edx\n\t"
     "koop1:\n\t"
     "movb $128,%%al     \n\t"
     "koop2:\n\t"
     "movb 1(%%esi),%%dl\n\t"
     "shl  $16,%%edx\n\t"
     "movb (%%esi),%%dl\n\t"
     "xorl $0x00800080,%%edx\n\t"
     "movl %%edx,(%%edi)\n\t"
     "addl $2,%%esi\n\t"
     "addl $4,%%edi\n\t"
     "decb %%al\n\t"
     "jne koop2\n\t"

     "addl $16,%%esi\n\t"
     "addl $768,%%edi\n\t"
     "decb %%bl\n\t"
     "jne koop1\n\t"
     :
     : "S" (XBuf), "D" (dest), "b" (tlines)
     : "%al", "%edx", "%cc" );
  }
  else if(vmode==4)
  {
   asm volatile(
     "yoop1:\n\t"
     "movb $128,%%al     \n\t"
     "yoop2:\n\t"
     "movb 1(%%esi),%%dh\n\t"
     "movb %%dh,%%dl\n\t"
     "shl  $16,%%edx\n\t"
     "movb (%%esi),%%dl\n\t"
     "movb %%dl,%%dh\n\t"               // Ugh
     "xorl $0x80808080,%%edx\n\t"
     "movl %%edx,(%%edi)\n\t"
     "addl $2,%%esi\n\t"
     "addl $4,%%edi\n\t"
     "decb %%al\n\t"
     "jne yoop2\n\t"

     "addl $16,%%esi\n\t"
     "addl $768,%%edi\n\t"
     "decb %%bl\n\t"
     "jne yoop1\n\t"
     :
     : "S" (XBuf), "D" (dest), "b" (tlines)
     : "%al", "%edx", "%cc" );
  }
  else if(vmode==7)
  {
   asm volatile(
     "goop81:\n\t"
     "movl $32,%%eax\n\t"
     "goop82:\n\t"
     "movl (%%esi),%%edx\n\t"
     "movl 4(%%esi),%%ecx\n\t"
     "xorl $0x80808080,%%edx\n\t"
     "xorl $0x80808080,%%ecx\n\t"
     "movl %%edx,(%%edi)\n\t"
     "movl %%ecx,4(%%edi)\n\t"
     "addl $8,%%esi\n\t"
     "addl $8,%%edi\n\t"
     "decl %%eax\n\t"
     "jne goop82\n\t"
     "addl $64,%%edi\n\t"
     "addl $16,%%esi\n\t"
     "decb %%bl\n\t"
     "jne goop81\n\t"
     :
     : "S" (XBuf), "D" (dest), "b" (tlines)
     : "%eax","%cc","%edx","%ecx" );
  }
  else
  {
   asm volatile(
     "goop1:\n\t"
     "movl $32,%%eax\n\t"
     "goop2:\n\t"
     "movl (%%esi),%%edx\n\t"
     "movl 4(%%esi),%%ecx\n\t"
     "movl %%edx,(%%edi)\n\t"
     "movl %%ecx,4(%%edi)\n\t"
     "addl $8,%%esi\n\t"
     "addl $8,%%edi\n\t"
     "decl %%eax\n\t"
     "jne goop2\n\t"
     "addl $16,%%esi\n\t"
     "decb %%bl\n\t"
     "jne goop1\n\t"
     :
     : "S" (XBuf), "D" (dest), "b" (tlines)
     : "%eax","%cc","%edx","%ecx" );
  }
 }
}


