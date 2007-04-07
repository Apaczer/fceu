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

static int RecalcCustom(void);

#define VF_DDSTRETCHED     1

#define VEF_LOSTSURFACE 1
#define VEF____INTERNAL 2

#define VMDF_DXBLT 1
#define VMDF_STRFS 2

typedef struct {
        int x;
        int y;
        int bpp;
        int flags;
        int xscale;
        int yscale;
        RECT srect;
        RECT drect;        
} vmdef;

// left, top, right, bottom
static vmdef vmodes[11]={
                         {320,240,8,0,1,1}, //0
                         {320,240,8,0,1,1}, //1
                         {512,384,8,0,1,1}, //2
                         {640,480,8,0,1,1}, //3
                         {640,480,8,0,1,1}, //4
                         {640,480,8,0,1,1}, //5
                         {640,480,8,VMDF_DXBLT,2,2}, //6
                         {1024,768,8,VMDF_DXBLT,4,3}, //7
                         {1280,1024,8,VMDF_DXBLT,5,4}, //8
                         {1600,1200,8,VMDF_DXBLT,6,5}, //9
                         {800,600,8,VMDF_DXBLT|VMDF_STRFS,0,0}    //10
                       };
static DDCAPS caps;
static int mustrestore=0;
static DWORD CBM[3];

static int bpp;
static int vflags;
static int veflags;

int fssync=0;
int winsync=0;

static uint32 *palettetranslate=0;

PALETTEENTRY color_palette[256];
static int PaletteChanged=0;

LPDIRECTDRAWCLIPPER lpClipper=0;
LPDIRECTDRAW  lpDD=0;
LPDIRECTDRAW4 lpDD4=0;
LPDIRECTDRAWPALETTE lpddpal;

DDSURFACEDESC2 ddsd;

DDSURFACEDESC2        ddsdback;
LPDIRECTDRAWSURFACE4  lpDDSPrimary=0;
LPDIRECTDRAWSURFACE4  lpDDSDBack=0;
LPDIRECTDRAWSURFACE4  lpDDSBack=0;

static void ShowDDErr(char *s)
{
 char tempo[512];
 sprintf(tempo,"DirectDraw: %s",s);
 FCEUD_PrintError(tempo);
}

int RestoreDD(int w)
{
 if(w)
  {
   if(!lpDDSBack) return 0;
   if(IDirectDrawSurface4_Restore(lpDDSBack)!=DD_OK) return 0;
  }
 else
  {
   if(!lpDDSPrimary) return 0;
   if(IDirectDrawSurface4_Restore(lpDDSPrimary)!=DD_OK) return 0;
  }
 veflags|=1;
 return 1;
}

void FCEUD_SetPalette(unsigned char index, unsigned char r, unsigned char g, unsigned char b)
{
        color_palette[index].peRed=r;
        color_palette[index].peGreen=g;
        color_palette[index].peBlue=b;
        PaletteChanged=1;
}

void FCEUD_GetPalette(unsigned char i, unsigned char *r, unsigned char *g, unsigned char *b)
{
        *r=color_palette[i].peRed;
        *g=color_palette[i].peGreen;
        *b=color_palette[i].peBlue;
}

int InitializeDDraw(void)
{
        ddrval = DirectDrawCreate(NULL, &lpDD, NULL);
        if (ddrval != DD_OK)
        {
         ShowDDErr("Error creating DirectDraw object.");
         return 0;
        }

        ddrval = IDirectDraw_QueryInterface(lpDD,&IID_IDirectDraw4,(LPVOID *)&lpDD4);
        IDirectDraw_Release(lpDD);

        if (ddrval != DD_OK)
        {
         ShowDDErr("Error querying interface.");
         return 0;
        }

        caps.dwSize=sizeof(caps);
        if(IDirectDraw4_GetCaps(lpDD4,&caps,0)!=DD_OK)
        {
         ShowDDErr("Error getting capabilities.");
         return 0;
        }
        return 1;
}

static int GetBPP(void)
{
        DDPIXELFORMAT ddpix;

        memset(&ddpix,0,sizeof(ddpix));
        ddpix.dwSize=sizeof(ddpix);

        ddrval=IDirectDrawSurface4_GetPixelFormat(lpDDSPrimary,&ddpix);
        if (ddrval != DD_OK)
        {
         ShowDDErr("Error getting primary surface pixel format.");
         return 0;
        }

        if(ddpix.dwFlags&DDPF_RGB)
        {
         bpp=ddpix.DUMMYUNIONNAMEN(1).dwRGBBitCount;
         CBM[0]=ddpix.DUMMYUNIONNAMEN(2).dwRBitMask;
         CBM[1]=ddpix.DUMMYUNIONNAMEN(3).dwGBitMask;
         CBM[2]=ddpix.DUMMYUNIONNAMEN(4).dwBBitMask;
        }
        else
        {
         ShowDDErr("RGB data not valid.");
         return 0;
        }
        if(bpp==15) bpp=16;

        return 1;
}

static int InitBPPStuff(void)
{
   if(bpp==16)
    palettetranslate=malloc(65536*4);
   else if(bpp>=24)
    palettetranslate=malloc(256*4);
   else if(bpp==8)
   {
    ddrval=IDirectDraw4_CreatePalette( lpDD4, DDPCAPS_8BIT|DDPCAPS_ALLOW256|DDPCAPS_INITIALIZE,color_palette,&lpddpal,NULL);
    if (ddrval != DD_OK)
    {
     ShowDDErr("Error creating palette object.");
     return 0;
    }
    ddrval=IDirectDrawSurface4_SetPalette(lpDDSPrimary, lpddpal);
    if (ddrval != DD_OK)
    {
     ShowDDErr("Error setting palette object.");
     return 0;
    }
   }
   return 1;
}

int SetVideoMode(int fs)
{
        if(!lpDD4)      // DirectDraw not initialized
         return(1);

        if(fs)
         if(!vmod)
          if(!RecalcCustom())
           return(0);

        vflags=0;
        veflags=1;
        PaletteChanged=1;

        ResetVideo();

        if(!fs)
        { 
         ShowCursorAbs(1);
         windowedfailed=1;
         HideFWindow(0);

         ddrval = IDirectDraw4_SetCooperativeLevel ( lpDD4, hAppWnd, DDSCL_NORMAL);
         if (ddrval != DD_OK)
         {
          ShowDDErr("Error setting cooperative level.");
          return 1;
         }

         /* Beginning */
         memset(&ddsd,0,sizeof(ddsd));
         ddsd.dwSize = sizeof(ddsd);
         ddsd.dwFlags = DDSD_CAPS;
         ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;

         ddrval = IDirectDraw4_CreateSurface ( lpDD4, &ddsd, &lpDDSPrimary,(IUnknown FAR*)NULL);
         if (ddrval != DD_OK)
         {
          ShowDDErr("Error creating primary surface.");
          return 1;
         }

         memset(&ddsdback,0,sizeof(ddsdback));
         ddsdback.dwSize=sizeof(ddsdback);
         ddsdback.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
         ddsdback.ddsCaps.dwCaps= DDSCAPS_OFFSCREENPLAIN;

         ddsdback.dwWidth=256;
         ddsdback.dwHeight=240;

         /* If no blit hardware is present, make sure buffer is created
            in system memory.
         */
         if(!(caps.dwCaps&DDCAPS_BLT))
          ddsdback.ddsCaps.dwCaps|=DDSCAPS_SYSTEMMEMORY;
        
         ddrval = IDirectDraw4_CreateSurface ( lpDD4, &ddsdback, &lpDDSBack, (IUnknown FAR*)NULL);
         if (ddrval != DD_OK)
         {
          ShowDDErr("Error creating secondary surface.");
          return 0;
         }

         if(!GetBPP())
          return 0;

         if(bpp!=16 && bpp!=24 && bpp!=32)
         {
          ShowDDErr("Current bit depth not supported!");
          return 0;
         }

         if(!InitBPPStuff())
          return 0;

         ddrval=IDirectDraw4_CreateClipper(lpDD4,0,&lpClipper,0);
         if (ddrval != DD_OK)
         {
          ShowDDErr("Error creating clipper.");
          return 0;
         }

         ddrval=IDirectDrawClipper_SetHWnd(lpClipper,0,hAppWnd);
         if (ddrval != DD_OK)
         {
          ShowDDErr("Error setting clipper window.");
          return 0;
         }
         ddrval=IDirectDrawSurface4_SetClipper(lpDDSPrimary,lpClipper);
         if (ddrval != DD_OK)
         {
          ShowDDErr("Error attaching clipper to primary surface.");
          return 0;
         }

         windowedfailed=0;
         SetMainWindowStuff();
        }
        else
        {
         HideFWindow(1);

         ddrval = IDirectDraw4_SetCooperativeLevel ( lpDD4, hAppWnd,DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN | DDSCL_ALLOWREBOOT);
         if (ddrval != DD_OK)
         {
          ShowDDErr("Error setting cooperative level.");
          return 0;
         }

         ddrval = IDirectDraw4_SetDisplayMode(lpDD4, vmodes[vmod].x, vmodes[vmod].y,vmodes[vmod].bpp,0,0);
         if (ddrval != DD_OK)
         {
          ShowDDErr("Error setting display mode.");
          return 0;
         }
         if(vmodes[vmod].flags&VMDF_DXBLT)
         {
          memset(&ddsdback,0,sizeof(ddsdback));
          ddsdback.dwSize=sizeof(ddsdback);
          ddsdback.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
          ddsdback.ddsCaps.dwCaps= DDSCAPS_OFFSCREENPLAIN;

          ddsdback.dwWidth=256; //vmodes[vmod].srect.right;
          ddsdback.dwHeight=240; //vmodes[vmod].srect.bottom;

          if(!(caps.dwCaps&DDCAPS_BLT))
           ddsdback.ddsCaps.dwCaps|=DDSCAPS_SYSTEMMEMORY;

          ddrval = IDirectDraw4_CreateSurface ( lpDD4, &ddsdback, &lpDDSBack, (IUnknown FAR*)NULL);
          if(ddrval!=DD_OK)
          {
           ShowDDErr("Error creating secondary surface.");
           return 0;
          }
         }

         // create foreground surface
        
         memset(&ddsd,0,sizeof(ddsd));
         ddsd.dwSize = sizeof(ddsd);
 
         ddsd.dwFlags = DDSD_CAPS;
         ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;

         if(fssync==2) // Double buffering.
         {
          ddsd.dwFlags |= DDSD_BACKBUFFERCOUNT;
          ddsd.dwBackBufferCount = 1;
          ddsd.ddsCaps.dwCaps |= DDSCAPS_COMPLEX | DDSCAPS_FLIP;
         }

         ddrval = IDirectDraw4_CreateSurface ( lpDD4, &ddsd, &lpDDSPrimary,(IUnknown FAR*)NULL);
         if (ddrval != DD_OK)
         {
          ShowDDErr("Error creating primary surface.");
          return 0;
         } 

         if(fssync==2)
         {
          DDSCAPS2 tmp;

          memset(&tmp,0,sizeof(tmp));
          tmp.dwCaps=DDSCAPS_BACKBUFFER;

          if(IDirectDrawSurface4_GetAttachedSurface(lpDDSPrimary,&tmp,&lpDDSDBack)!=DD_OK)
          {
           ShowDDErr("Error getting attached surface.");
           return 0;
          }
         }

         if(!GetBPP())
          return 0;
         if(!InitBPPStuff())
          return 0;

         mustrestore=1;
         ShowCursorAbs(0);
        }

        InputScreenChanged(fs);
        fullscreen=fs;
        return 1;
}

static void BlitScreenWindow(uint8 *XBuf);
static void BlitScreenFull(uint8 *XBuf);

void FCEUD_BlitScreen(uint8 *XBuf)
{
 doagain:

 UpdateFCEUWindow();

 if(!(eoptions&EO_BGRUN))
  while(nofocus)
  {
   Sleep(50);
   BlockingCheck();
  }


 /* This complex statement deserves some explanation.
    Make sure this special speed throttling hasn't been disabled by the user
    first. Second, we don't want to throttle the speed if the fast-forward
    button is pressed down(or during certain network play conditions).
       
    Now, if we're at this point, we'll throttle speed if sound is disabled.
    Otherwise, it gets a bit more complicated.  We'll throttle speed if focus
    to FCE Ultra has been lost and we're writing to the primary sound buffer
    because our sound code won't block.  Blocking does seem to work when
    writing to a secondary buffer, so we won't throttle when a secondary
    buffer is used.
 */

 if(!(eoptions&EO_NOTHROTTLE))
  if(!NoWaiting)
   if(!soundo || (soundo && nofocus && !(soundoptions&SO_SECONDARY)) )
       SpeedThrottle();

 if(fullscreen)
 {
  if(fssync==1 && !NoWaiting)
   IDirectDraw4_WaitForVerticalBlank(lpDD4,DDWAITVB_BLOCKBEGIN,0);

  BlitScreenFull(XBuf);
 }
 else
 {
  if(winsync && !NoWaiting)
   IDirectDraw4_WaitForVerticalBlank(lpDD4,DDWAITVB_BLOCKBEGIN,0);

  if(!windowedfailed)
   BlitScreenWindow(XBuf);
 }
 if(userpause)
 {
  StopSound();
  Sleep(50);
  BlockingCheck();
  goto doagain;
 }
}

static INLINE void BlitVidHi(uint8 *src, uint8 *dest, /*int xr,*/ int yr, int pitch)
{
 int x,y;
 int pinc;

 if(!(eoptions&EO_CLIPSIDES))
  switch(bpp)
  {
   case 32:

   pinc=pitch-(256<<2);
   for(y=yr;y;y--)
   {
    for(x=256;x;x--)
    {
     *(uint32 *)dest=palettetranslate[(uint32)*src];     
     dest+=4;
     src++;
    }
    dest+=pinc;
    src+=16;
   }
   break;

  case 24:
   pinc=pitch-(256*3);
   for(y=yr;y;y--)
   {
    for(x=256;x;x--)
    {
     uint32 tmp;
     tmp=palettetranslate[(uint32)*src];
     *(uint16*)dest=(uint16)tmp;
     *&dest[2]=(uint8)(tmp>>16);
     dest+=3;
     src++;
    }
    dest+=pinc;
    src+=16;
   }
   break;

  case 16:
   pinc=pitch-(256<<1);
   for(y=yr;y;y--)
   {
    for(x=256>>1;x;x--)
    {
     *(unsigned long *)dest=palettetranslate[*(unsigned short *)src];
     dest+=4;
     src+=2;
    }
    dest+=pinc;
    src+=16;
   }
   break;
 }
 else
  switch(bpp)
  {
   case 32:

   pinc=pitch-(240<<2);
   for(y=yr;y;y--)
   {
    for(x=240;x;x--)
    {
     *(uint32 *)dest=palettetranslate[(uint32)*src];
     dest+=4;
     src++;
    }
    dest+=pinc;
    src+=32;
   }
   break;

  case 24:
   pinc=pitch-(240*3);
   for(y=yr;y;y--)
   {
    for(x=240;x;x--)
    {
     uint32 tmp;
     tmp=palettetranslate[(uint32)*src];
     *(uint16*)dest=(uint16)tmp;
     *&dest[2]=(uint8)(tmp>>16);
     dest+=3;
     src++;
    }
    dest+=pinc;
    src+=32;
   }
   break;
  case 16:
   pinc=pitch-(240<<1);
   for(y=yr;y;y--)
   {
    for(x=240>>1;x;x--)
    {
     *(unsigned long *)dest=palettetranslate[*(unsigned short *)src];
     dest+=4;
     src+=2;
    }
    dest+=pinc;
    src+=32;
   }
   break;
 }
}

static INLINE void FixPaletteHi(void)
{
   int x;

   switch(bpp)
   {
    case 16:{
             int cshiftr[3];
             int cshiftl[3];
             int a,x,z,y;
        
             cshiftl[0]=cshiftl[1]=cshiftl[2]=-1;
             for(a=0;a<3;a++)
             {
              for(x=0,y=-1,z=0;x<16;x++)
              {
               if(CBM[a]&(1<<x))
               {
                if(cshiftl[a]==-1) cshiftl[a]=x;
                z++;
               }
              }
              cshiftr[a]=(8-z);
             }

             for(x=0;x<65536;x++)
             { 
              uint16 lower,upper;   
              lower=(color_palette[x&255].peRed>>cshiftr[0])<<cshiftl[0];
              lower|=(color_palette[x&255].peGreen>>cshiftr[1])<<cshiftl[1];
              lower|=(color_palette[x&255].peBlue>>cshiftr[2])<<cshiftl[2];
              upper=(color_palette[x>>8].peRed>>cshiftr[0])<<cshiftl[0];
              upper|=(color_palette[x>>8].peGreen>>cshiftr[1])<<cshiftl[1];
              upper|=(color_palette[x>>8].peBlue>>cshiftr[2])<<cshiftl[2];      
              palettetranslate[x]=lower|(upper<<16);
             }
            }
            break;
    case 24:
    case 32:
            for(x=0;x<256;x++)
            {
             uint32 t;
             t=0;
             t=color_palette[x].peBlue;
             t|=color_palette[x].peGreen<<8;
             t|=color_palette[x].peRed<<16;
             palettetranslate[x]=t;
            }
            break;
   }
}

static void BlitScreenWindow(unsigned char *XBuf)
{
 int pitch;
 unsigned char *ScreenLoc;
 static RECT srect;
 RECT drect;

 srect.top=srect.left=0;
 srect.right=VNSWID;
 srect.bottom=totallines;

 if(PaletteChanged==1)
 {
  FixPaletteHi();
  PaletteChanged=0;
 }
 if(!GetClientAbsRect(&drect)) return;

 ddrval=IDirectDrawSurface4_Lock(lpDDSBack,NULL,&ddsdback, 0, NULL);
 if(ddrval!=DD_OK)
 {
  if(ddrval==DDERR_SURFACELOST) RestoreDD(1);
  return;
 }
 pitch=ddsdback.DUMMYUNIONNAMEN(1).lPitch;
 ScreenLoc=ddsdback.lpSurface;

 if(veflags&1)
 {
  memset(ScreenLoc,0,pitch*240);
  veflags&=~1;
 }

 BlitVidHi(XBuf+srendline*272+VNSCLIP, ScreenLoc, /*VNSWID,*/ totallines, pitch);

 IDirectDrawSurface4_Unlock(lpDDSBack, NULL);

 if(IDirectDrawSurface4_Blt(lpDDSPrimary, &drect,lpDDSBack,&srect,DDBLT_ASYNC,0)!=DD_OK)
 {
  ddrval=IDirectDrawSurface4_Blt(lpDDSPrimary, &drect,lpDDSBack,&srect,DDBLT_WAIT,0);
  if(ddrval!=DD_OK)
  {
   if(ddrval==DDERR_SURFACELOST) {RestoreDD(1);RestoreDD(0);}
   return;
  }
 }
}

static void BlitScreenFull(uint8 *XBuf)
{
  static int pitch;
  char *ScreenLoc;
  unsigned long x;
  uint8 y;
  RECT srect,drect;
  LPDIRECTDRAWSURFACE4 lpDDSVPrimary;


  if(fssync==2)
   lpDDSVPrimary=lpDDSDBack;
  else
   lpDDSVPrimary=lpDDSPrimary;

  if(PaletteChanged==1)
  {
   if(bpp>=16)
    FixPaletteHi();
   else
    for(x=0;x<=0x80;x+=0x80)
    {
     ddrval=IDirectDrawPalette_SetEntries(lpddpal,0,0x80^x,128,&color_palette[x]);
     if(ddrval!=DD_OK)
     {
      if(ddrval==DDERR_SURFACELOST) RestoreDD(0);
      return;
     }
    }
   PaletteChanged=0;
  }

 if(vmodes[vmod].flags&VMDF_DXBLT)
 {
  ddrval=IDirectDrawSurface4_Lock(lpDDSBack,NULL,&ddsdback, 0, NULL);
  if(ddrval!=DD_OK)
  {
   if(ddrval==DDERR_SURFACELOST) RestoreDD(1);
   return;
  }
  ScreenLoc=ddsdback.lpSurface;
  pitch=ddsdback.DUMMYUNIONNAMEN(1).lPitch;

  srect.top=0;
  srect.left=0;
  srect.right=VNSWID;
  srect.bottom=totallines;
  if(vmodes[vmod].flags&VMDF_STRFS)
  {
   drect.top=0;
   drect.left=0;
   drect.right=vmodes[vmod].x;
   drect.bottom=vmodes[vmod].y;
  }
  else
  {
   drect.top=(vmodes[vmod].y-(totallines*vmodes[vmod].yscale))>>1;
   drect.bottom=drect.top+(totallines*vmodes[vmod].yscale);
   drect.left=(vmodes[vmod].x-VNSWID*vmodes[vmod].xscale)>>1;
   drect.right=drect.left+VNSWID*vmodes[vmod].xscale;
  }
 }
 else
 {
  ddrval=IDirectDrawSurface4_Lock(lpDDSVPrimary,NULL,&ddsd, 0, NULL);
  if(ddrval!=DD_OK)
  {
   if(ddrval==DDERR_SURFACELOST) RestoreDD(0);
   return;
  }

  ScreenLoc=ddsd.lpSurface;
  pitch=ddsd.DUMMYUNIONNAMEN(1).lPitch;
 }

 if(veflags&1)
 {
  if(vmodes[vmod].flags&VMDF_DXBLT)
  {
   veflags|=2;
   memset((char *)ScreenLoc,0,pitch*srect.bottom);
  }
  else
  {
   memset((char *)ScreenLoc,0,pitch*vmodes[vmod].y);
  }
  PaletteChanged=1;
  veflags&=~1;
 }

 if(vmod==5)
 {
  if(eoptions&EO_CLIPSIDES)
  {
   asm volatile(
      "xorl %%edx, %%edx\n\t"
      "akoop1:\n\t"
      "movb $120,%%al     \n\t"
      "akoop2:\n\t"
      "movb 1(%%esi),%%dl\n\t"
      "shl  $16,%%edx\n\t"
      "movb (%%esi),%%dl\n\t"
      "xorl $0x00800080,%%edx\n\t"
      "movl %%edx,(%%edi)\n\t"
      "addl $2,%%esi\n\t"
      "addl $4,%%edi\n\t"
      "decb %%al\n\t"
      "jne akoop2\n\t"
      "addl $32,%%esi\n\t"
      "addl %%ecx,%%edi\n\t"
      "decb %%bl\n\t"
      "jne akoop1\n\t"
      :
      : "S" (XBuf+srendline*272+VNSCLIP), "D" (ScreenLoc+((240-totallines)/2)*pitch+(640-(VNSWID<<1))/2),"b" (totallines), "c" ((pitch-VNSWID)<<1)
      : "%al", "%edx", "%cc" );
  }
  else
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
     "addl %%ecx,%%edi\n\t"
     "decb %%bl\n\t"
     "jne koop1\n\t"
      :
      : "S" (XBuf+srendline*272), "D" (ScreenLoc+((240-totallines)/2)*pitch+(640-512)/2),"b" (totallines), "c" (pitch-512+pitch)
      : "%al", "%edx", "%cc" );
  }
 }
 else if(vmod==4)
 {
  if(eoptions&EO_CLIPSIDES)
  {
   asm volatile(
      "ayoop1:\n\t"
      "movb $120,%%al     \n\t"
      "ayoop2:\n\t"
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
      "jne ayoop2\n\t"
      "addl $32,%%esi\n\t"
      "addl %%ecx,%%edi\n\t"
      "decb %%bl\n\t"
      "jne ayoop1\n\t"
      :
      : "S" (XBuf+srendline*272+VNSCLIP), "D" (ScreenLoc+((240-totallines)/2)*pitch+(640-(VNSWID<<1))/2),"b" (totallines), "c" ((pitch-VNSWID)<<1)
      : "%al", "%edx", "%cc" );
  }
  else
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
      "addl %%ecx,%%edi\n\t"
      "decb %%bl\n\t"
      "jne yoop1\n\t"
      :
      : "S" (XBuf+srendline*272), "D" (ScreenLoc+((240-totallines)/2)*pitch+(640-512)/2),"b" (totallines), "c" (pitch-512+pitch)
      : "%al", "%edx", "%cc" );
  }
 }
 else
 {
  if(!(vmodes[vmod].flags&VMDF_DXBLT))
  {  
   ScreenLoc+=((vmodes[vmod].x-VNSWID)>>1)*(bpp>>3)+(((vmodes[vmod].y-totallines)>>1))*pitch;
  }
  if(bpp>=16)
  {
   BlitVidHi(XBuf+srendline*272+VNSCLIP, ScreenLoc, /*VNSWID,*/ totallines, pitch);
  }
  else
  {
   XBuf+=srendline*272+VNSCLIP;
   if(eoptions&EO_CLIPSIDES)
   {
    for(y=totallines;y;y--)
    {
     for(x=60;x;x--)
     {
      *(long *)ScreenLoc=(*(long *)XBuf)^0x80808080;
      ScreenLoc+=4;
      XBuf+=4;
     }
     ScreenLoc+=pitch-240;
     XBuf+=32;
    }
   }
   else
   {
    for(y=totallines;y;y--)
    {
     for(x=64;x;x--)
     {
      *(long *)ScreenLoc=(*(long *)XBuf)^0x80808080;
      ScreenLoc+=4;
      XBuf+=4;
     }
     ScreenLoc+=pitch-256;
     XBuf+=16;
    }
   }
  }
 }

 if(vmodes[vmod].flags&VMDF_DXBLT)
 { 
  IDirectDrawSurface4_Unlock(lpDDSBack, NULL);

  if(veflags&2)
  {
   if(IDirectDrawSurface4_Lock(lpDDSVPrimary,NULL,&ddsd, 0, NULL)==DD_OK)
   {
    memset(ddsd.lpSurface,0,ddsd.DUMMYUNIONNAMEN(1).lPitch*vmodes[vmod].y);
    IDirectDrawSurface4_Unlock(lpDDSVPrimary, NULL);
    veflags&=~2;
   }
  }

 
  if(IDirectDrawSurface4_Blt(lpDDSVPrimary, &drect,lpDDSBack,&srect,DDBLT_ASYNC,0)!=DD_OK)
  {
   ddrval=IDirectDrawSurface4_Blt(lpDDSVPrimary, &drect,lpDDSBack,&srect,DDBLT_WAIT,0);
   if(ddrval!=DD_OK)
   {
    if(ddrval==DDERR_SURFACELOST)
    {
     RestoreDD(0);
     RestoreDD(1);
    }
    return;
   }

  }
 }
 else
  IDirectDrawSurface4_Unlock(lpDDSVPrimary, NULL);
 if(fssync==2)
 {
  IDirectDrawSurface4_Flip(lpDDSPrimary,0,0);

 }
}

void ResetVideo(void)
{
 ShowCursorAbs(1);
 if(palettetranslate) {free(palettetranslate);palettetranslate=0;}
 if(lpDD4)
  if(mustrestore)
   {IDirectDraw4_RestoreDisplayMode(lpDD4);mustrestore=0;}
 if(lpDDSBack) {IDirectDrawSurface4_Release(lpDDSBack);lpDDSBack=0;}
 if(lpDDSPrimary) {IDirectDrawSurface4_Release(lpDDSPrimary);lpDDSPrimary=0;} 
 if(lpClipper) {IDirectDrawClipper_Release(lpClipper);lpClipper=0;}
}

static int RecalcCustom(void)
{
 vmodes[0].flags&=~VMDF_DXBLT;

 if(vmodes[0].flags&VMDF_STRFS)
 {
  vmodes[0].flags|=VMDF_DXBLT;

  vmodes[0].srect.top=srendline;
  vmodes[0].srect.left=VNSCLIP;
  vmodes[0].srect.right=256-VNSCLIP;
  vmodes[0].srect.bottom=erendline+1;

  vmodes[0].drect.top=vmodes[0].drect.left=0;
  vmodes[0].drect.right=vmodes[0].x;
  vmodes[0].drect.bottom=vmodes[0].y;
 }
 else if(vmodes[0].xscale!=1 || vmodes[0].yscale!=1)
 {
  vmodes[0].flags|=VMDF_DXBLT;
  if(VNSWID*vmodes[0].xscale>vmodes[0].x)
  {
   FCEUD_PrintError("Scaled width is out of range.  Reverting to no horizontal scaling.");
   vmodes[0].xscale=1;
  }
  if(totallines*vmodes[0].yscale>vmodes[0].y)
  {
   FCEUD_PrintError("Scaled height is out of range.  Reverting to no vertical scaling.");
   vmodes[0].yscale=1;
  }

  vmodes[0].srect.left=VNSCLIP;
  vmodes[0].srect.top=srendline;
  vmodes[0].srect.right=256-VNSCLIP;
  vmodes[0].srect.bottom=erendline+1;

  vmodes[0].drect.top=(vmodes[0].y-(totallines*vmodes[0].yscale))>>1;
  vmodes[0].drect.bottom=vmodes[0].drect.top+totallines*vmodes[0].yscale;
  
  vmodes[0].drect.left=(vmodes[0].x-(VNSWID*vmodes[0].xscale))>>1;
  vmodes[0].drect.right=vmodes[0].drect.left+VNSWID*vmodes[0].xscale;
 }

 if(vmodes[0].x<VNSWID)
 {
  FCEUD_PrintError("Horizontal resolution is too low.");
  return(0);
 }
 if(vmodes[0].y<totallines && !(vmodes[0].flags&VMDF_STRFS))
 {
  FCEUD_PrintError("Vertical resolution must not be less than the total number of drawn scanlines.");
  return(0);
 }

 return(1);
}

BOOL CALLBACK VideoConCallB(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
        static char *vmstr[11]={
                                "Custom",
                                "320x240 Full Screen",
                                "512x384 Centered",
                                "640x480 Centered",
                                "640x480 Scanlines",
                                "640x480 \"4 per 1\"",
                                "640x480 2x,2y",
                                "1024x768 4x,3y",
                                "1280x1024 5x,4y",
                                "1600x1200 6x,5y",
                                "800x600 Stretched"
                               };
        int x;

        switch(uMsg)
        {
         case WM_INITDIALOG:                
                for(x=0;x<11;x++)
                 SendDlgItemMessage(hwndDlg,100,CB_ADDSTRING,0,(LPARAM)(LPSTR)vmstr[x]);
                SendDlgItemMessage(hwndDlg,100,CB_SETCURSEL,vmod,(LPARAM)(LPSTR)0);

                SendDlgItemMessage(hwndDlg,202,CB_ADDSTRING,0,(LPARAM)(LPSTR)"8");
                SendDlgItemMessage(hwndDlg,202,CB_ADDSTRING,0,(LPARAM)(LPSTR)"16");
                SendDlgItemMessage(hwndDlg,202,CB_ADDSTRING,0,(LPARAM)(LPSTR)"24");
                SendDlgItemMessage(hwndDlg,202,CB_ADDSTRING,0,(LPARAM)(LPSTR)"32");
                SendDlgItemMessage(hwndDlg,202,CB_SETCURSEL,(vmodes[0].bpp>>3)-1,(LPARAM)(LPSTR)0);

                SetDlgItemInt(hwndDlg,200,vmodes[0].x,0);
                SetDlgItemInt(hwndDlg,201,vmodes[0].y,0);

                SetDlgItemInt(hwndDlg,302,vmodes[0].xscale,0);
                SetDlgItemInt(hwndDlg,303,vmodes[0].yscale,0);
                CheckRadioButton(hwndDlg,300,301,(vmodes[0].flags&VMDF_STRFS)?301:300);
                if(eoptions&EO_FSAFTERLOAD)
                 CheckDlgButton(hwndDlg,102,BST_CHECKED);

                if(eoptions&EO_CLIPSIDES)
                 CheckDlgButton(hwndDlg,106,BST_CHECKED);

                SetDlgItemInt(hwndDlg,500,srendlinen,0);
                SetDlgItemInt(hwndDlg,501,erendlinen,0);

                SetDlgItemInt(hwndDlg,502,srendlinep,0);
                SetDlgItemInt(hwndDlg,503,erendlinep,0);


                SetDlgItemInt(hwndDlg,103,winsizemul,0);

                SendDlgItemMessage(hwndDlg,104,CB_ADDSTRING,0,(LPARAM)(LPSTR)"<none>");
                SendDlgItemMessage(hwndDlg,105,CB_ADDSTRING,0,(LPARAM)(LPSTR)"<none>");

                SendDlgItemMessage(hwndDlg,104,CB_ADDSTRING,0,(LPARAM)(LPSTR)"Wait for VBlank");
                SendDlgItemMessage(hwndDlg,105,CB_ADDSTRING,0,(LPARAM)(LPSTR)"Wait for VBlank");

                SendDlgItemMessage(hwndDlg,105,CB_ADDSTRING,0,(LPARAM)(LPSTR)"Double Buffering");

                SendDlgItemMessage(hwndDlg,104,CB_SETCURSEL,winsync,(LPARAM)(LPSTR)0);
                SendDlgItemMessage(hwndDlg,105,CB_SETCURSEL,fssync,(LPARAM)(LPSTR)0);
                break;
         case WM_CLOSE:
         case WM_QUIT: goto gornk;
         case WM_COMMAND:
                        if(!(wParam>>16))
                        switch(wParam&0xFFFF)
                        {
                         case 1:
                         gornk:
         
                         if(IsDlgButtonChecked(hwndDlg,106)==BST_CHECKED)
                          eoptions|=EO_CLIPSIDES;
                         else
                          eoptions&=~EO_CLIPSIDES;

                         srendlinen=GetDlgItemInt(hwndDlg,500,0,0);
                         erendlinen=GetDlgItemInt(hwndDlg,501,0,0);
                         srendlinep=GetDlgItemInt(hwndDlg,502,0,0);
                         erendlinep=GetDlgItemInt(hwndDlg,503,0,0);


                         if(erendlinen>239) erendlinen=239;
                         if(srendlinen>erendlinen) srendlinen=erendlinen;

                         if(erendlinep>239) erendlinep=239;
                         if(srendlinep>erendlinen) srendlinep=erendlinep;

                         UpdateRendBounds();

                         if(IsDlgButtonChecked(hwndDlg,301)==BST_CHECKED)
                          vmodes[0].flags|=VMDF_STRFS;
                         else
                          vmodes[0].flags&=~VMDF_STRFS;

                         vmod=SendDlgItemMessage(hwndDlg,100,CB_GETCURSEL,0,(LPARAM)(LPSTR)0);
                         vmodes[0].x=GetDlgItemInt(hwndDlg,200,0,0);
                         vmodes[0].y=GetDlgItemInt(hwndDlg,201,0,0);
                         vmodes[0].bpp=(SendDlgItemMessage(hwndDlg,202,CB_GETCURSEL,0,(LPARAM)(LPSTR)0)+1)<<3;

                         vmodes[0].xscale=GetDlgItemInt(hwndDlg,302,0,0);
                         vmodes[0].yscale=GetDlgItemInt(hwndDlg,303,0,0);
        
                         if(IsDlgButtonChecked(hwndDlg,101)==BST_CHECKED)
                          fullscreen=1;
                         else
                          fullscreen=0;
                         if(IsDlgButtonChecked(hwndDlg,102)==BST_CHECKED)
                          eoptions|=EO_FSAFTERLOAD;
                         else
                          eoptions&=~EO_FSAFTERLOAD;

                         {
                          int t=GetDlgItemInt(hwndDlg,103,0,0);
                          if(t>0 && t<60)
                           winsizemul=t;
                         }
                         winsync=SendDlgItemMessage(hwndDlg,104,CB_GETCURSEL,0,(LPARAM)(LPSTR)0);
                         fssync=SendDlgItemMessage(hwndDlg,105,CB_GETCURSEL,0,(LPARAM)(LPSTR)0);
                         EndDialog(hwndDlg,0);
                         break;
               }
              }
  return 0;
}

static void SetFSVideoMode(void)
{
 changerecursive=1;
 if(!SetVideoMode(1))
  SetVideoMode(0);
 changerecursive=0;
}



void ConfigVideo(void)
{
        DialogBox(fceu_hInstance,"VIDEOCONFIG",hAppWnd,VideoConCallB); 
        UpdateRendBounds();
        if(fullscreen)
         SetFSVideoMode();
        else
         SetMainWindowStuff();
}

void DoVideoConfigFix(void)
{
        UpdateRendBounds();
}


#ifdef moo
                         if(!vmod)
                         {
                          if(vmodes[0].x<VNSWID)
                          {
                           FCEUD_PrintError("Horizontal resolution is too low.");
                           return 0;
                          }
                          if(vmodes[0].y<totallines && !(vmodes[0].flags&VMDF_STRFS))
                          {
                           FCEUD_PrintError("Vertical resolution must not be less than the total number of drawn scanlines.");
                           return 0;
                          }
                         }


#endif
