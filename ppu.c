/**
 *  For whatever reason, breaking this out of fce.c made sprites not corrupt
 */


#include        <string.h>
#include	<stdio.h>
#include	<stdlib.h>

#include	"types.h"
#include	"x6502.h"
#include	"fce.h"
#include	"sound.h"
#include        "svga.h"
#include	"netplay.h"
#include	"general.h"
#include	"endian.h"
#include	"version.h"
#include        "memory.h"

#include	"cart.h"
#include	"nsf.h"
#include	"fds.h"
#include	"ines.h"
#include	"unif.h"
#include        "cheat.h"

#define MMC5SPRVRAMADR(V)      &MMC5SPRVPage[(V)>>10][(V)]
//#define MMC5BGVRAMADR(V)      &MMC5BGVPage[(V)>>10][(V)]
#define	VRAMADR(V)	&VPage[(V)>>10][(V)]

#define	V_FLIP	0x80
#define	H_FLIP	0x40
#define	SP_BACK	0x20

uint8 SPRAM[0x100];
static uint8 SPRBUF[0x100];

static uint8 sprlinebuf[256+8];
extern void BGRender(uint8 *target);
extern int tosprite;


static int maxsprites=8;


void FCEUI_DisableSpriteLimitation(int a)
{
 maxsprites=a?64:8;
}


//int printed=1;
typedef struct {
        uint8 y,no,atr,x;
} SPR __attribute__((aligned(1)));

typedef struct {
  //	uint8 ca[2],atr,x;
  	uint8 ca[2],atr,x;
  //  union {  int z; }


} SPRB __attribute__((aligned(1)));



static uint8 nosprites,SpriteBlurp;

void FetchSpriteData(void)
{
	SPR *spr;
	uint8 H;
	int n,vofs;

	spr=(SPR *)SPRAM;
	H=8;

	nosprites=SpriteBlurp=0;

        vofs=(unsigned int)(PPU[0]&0x8&(((PPU[0]&0x20)^0x20)>>2))<<9;
	H+=(PPU[0]&0x20)>>2;

        if(!PPU_hook)
         for(n=63;n>=0;n--,spr++)
         {
                if((unsigned int)(scanline-spr->y)>=H) continue;

                if(nosprites<maxsprites)
                {
                 if(n==63) SpriteBlurp=1;

		 {
		  SPRB dst;
		  uint8 *C;
                  int t;
                  unsigned int vadr;

                  t = (int)scanline-(spr->y);

                  if (Sprite16)
                   vadr = ((spr->no&1)<<12) + ((spr->no&0xFE)<<4);
                  else
                   vadr = (spr->no<<4)+vofs;

                  if (spr->atr&V_FLIP)
                  {
                        vadr+=7;
                        vadr-=t;
                        vadr+=(PPU[0]&0x20)>>1;
                        vadr-=t&8;
                  }
                  else
                  {
                        vadr+=t;
                        vadr+=t&8;
                  }

		  /* Fix this geniestage hack */
      	          if(MMC5Hack && geniestage!=1) C = MMC5SPRVRAMADR(vadr);
                  else C = VRAMADR(vadr);


		  dst.ca[0]=C[0];
		  dst.ca[1]=C[8];
		  dst.x=spr->x;
		  dst.atr=spr->atr;


		  *(uint32 *)&SPRBUF[nosprites<<2]=*(uint32 *)&dst;
		 }

                 nosprites++;
                }
                else
                {
                  PPU_status|=0x20;
                  break;
                }
         }
	else
         for(n=63;n>=0;n--,spr++)
         {
                if((unsigned int)(scanline-spr->y)>=H) continue;

                if(nosprites<maxsprites)
                {
                 if(n==63) SpriteBlurp=1;

                 {
                  SPRB dst;
                  uint8 *C;
                  int t;
                  unsigned int vadr;

                  t = (int)scanline-(spr->y);

                  if (Sprite16)
                   vadr = ((spr->no&1)<<12) + ((spr->no&0xFE)<<4);
                  else
                   vadr = (spr->no<<4)+vofs;

                  if (spr->atr&V_FLIP)
                  {
                        vadr+=7;
                        vadr-=t;
                        vadr+=(PPU[0]&0x20)>>1;
                        vadr-=t&8;
                  }
                  else
                  {
                        vadr+=t;
                        vadr+=t&8;
                  }

                  if(MMC5Hack) C = MMC5SPRVRAMADR(vadr);
                  else C = VRAMADR(vadr);
                  dst.ca[0]=C[0];
		  if(nosprites<8)
		  {
		   PPU_hook(0x2000);
                   PPU_hook(vadr);
		  }
                  dst.ca[1]=C[8];
                  dst.x=spr->x;
                  dst.atr=spr->atr;


                  *(uint32 *)&SPRBUF[nosprites<<2]=*(uint32 *)&dst;
                 }

                 nosprites++;
                }
                else
                {
                  PPU_status|=0x20;
                  break;
                }
         }

        if(nosprites>8) PPU_status|=0x20;  /* Handle case when >8 sprites per
					   scanline option is enabled. */
	else if(PPU_hook)
	{
	 for(n=0;n<(8-nosprites);n++)
	 {
                 PPU_hook(0x2000);
                 PPU_hook(vofs);
	 }
	}

}

#ifdef FRAMESKIP
extern int FSkip;
#endif

void RefreshSprite(uint8 *target)
{
	int n, minx=256;
        SPRB *spr;

        if(!nosprites) return;
	#ifdef FRAMESKIP
	if(FSkip)
	{
	 if(!SpriteBlurp)
	 {
	  nosprites=0;
	  return;
	 }
	 else
	  nosprites=1;
	}
	#endif

        nosprites--;
        spr = (SPRB*)SPRBUF+nosprites;

       for(n=nosprites;n>=0;n--,spr--)
       {
        register uint32 J;

        J=spr->ca[0]|spr->ca[1];

        if (J)
        {
          register uint8 atr,c1,c2;
          uint8 *C;
          uint8 *VB;
	  int x=spr->x;
	  atr=spr->atr;

          if (x < minx)
	  {
           if (minx == 256) FCEU_dwmemset(sprlinebuf,0x80808080,256); // only clear sprite buff when we encounter first sprite
           minx = x;
	  }
                        if(n==0 && SpriteBlurp && !(PPU_status&0x40))
                        {
			 int z,ze=x+8;
			 if(ze>256) {ze=256;}
			 if(ScreenON && (scanline<FSettings.FirstSLine || scanline>FSettings.LastSLine
			 #ifdef FRAMESKIP
			 || FSkip
			 #endif
			 ))
			  BGRender(target);

			 if(!(atr&H_FLIP))
			 {
			  for(z=x;z<ze;z++)
			  {
			   if(J&(0x80>>(z-x)))
			   {
			    if(!(target[z]&64))
			     tosprite=z;
			   }
			  }
			 }
			 else
			 {
                          for(z=x;z<ze;z++)
                          {
                           if(J&(1<<(z-x)))
                           {
                            if(!(target[z]&64))
                             tosprite=z;
                           }
                          }
			 }
			 //FCEU_DispMessage("%d, %d:%d",scanline,x,tosprite);
                        }

         c1=((spr->ca[0]>>1)&0x55)|(spr->ca[1]&0xAA);
	 c2=(spr->ca[0]&0x55)|((spr->ca[1]<<1)&0xAA);

	 C = sprlinebuf+x;
         VB = (PALRAM+0x10)+((atr&3)<<2);

         {
	  J &= 0xff;
	  if(atr&SP_BACK) J |= 0x4000;
          if (atr&H_FLIP)
          {
           if (J&0x02)  C[1]=VB[c1&3]|(J>>8);
           if (J&0x01)  *C=VB[c2&3]|(J>>8);
           c1>>=2;c2>>=2;
           if (J&0x08)  C[3]=VB[c1&3]|(J>>8);
           if (J&0x04)  C[2]=VB[c2&3]|(J>>8);
           c1>>=2;c2>>=2;
           if (J&0x20)  C[5]=VB[c1&3]|(J>>8);
           if (J&0x10)  C[4]=VB[c2&3]|(J>>8);
           c1>>=2;c2>>=2;
           if (J&0x80)  C[7]=VB[c1]|(J>>8);
           if (J&0x40)  C[6]=VB[c2]|(J>>8);
	  } else  {
           if (J&0x02)  C[6]=VB[c1&3]|(J>>8);
           if (J&0x01)  C[7]=VB[c2&3]|(J>>8);
	   c1>>=2;c2>>=2;
           if (J&0x08)  C[4]=VB[c1&3]|(J>>8);
           if (J&0x04)  C[5]=VB[c2&3]|(J>>8);
           c1>>=2;c2>>=2;
           if (J&0x20)  C[2]=VB[c1&3]|(J>>8);
           if (J&0x10)  C[3]=VB[c2&3]|(J>>8);
           c1>>=2;c2>>=2;
           if (J&0x80)  *C=VB[c1]|(J>>8);
           if (J&0x40)  C[1]=VB[c2]|(J>>8);
	  }
         }
      }
     }

     nosprites=0;
     #ifdef FRAMESKIP
     if(FSkip) return;
     #endif
     if (minx == 256) return; // no visible sprites

     {
      uint8 n=((PPU[1]&4)^4)<<1;
      if ((int)n < minx) n = minx & 0xfc;
      loopskie:
      {
       uint32 t=*(uint32 *)(sprlinebuf+n);
       if(t!=0x80808080)
       {
	#ifdef LSB_FIRST
        uint32 tb=*(uint32 *)(target+n);
        if(!(t&0x00000080) && (!(t&0x00000040) || (tb&0x00000040))) { // have sprite pixel AND (normal sprite OR behind bg with no bg)
          tb &= ~0x000000ff; tb |= t & 0x000000ff;
        }

        if(!(t&0x00008000) && (!(t&0x00004000) || (tb&0x00004000))) {
          tb &= ~0x0000ff00; tb |= t & 0x0000ff00;
        }

        if(!(t&0x00800000) && (!(t&0x00400000) || (tb&0x00400000))) {
          tb &= ~0x00ff0000; tb |= t & 0x00ff0000;
        }

        if(!(t&0x80000000) && (!(t&0x40000000) || (tb&0x40000000))) {
          tb &= ~0xff000000; tb |= t & 0xff000000;
        }
	*(uint32 *)(target+n)=tb;
	#else
        if(!(t&0x80000000))
        {
         if(!(t&0x40))       // Normal sprite
          P[n]=sprlinebuf[n];
         else if(P[n]&64)        // behind bg sprite
          P[n]=sprlinebuf[n];
        }

        if(!(t&0x800000))
        {
         if(!(t&0x4000))       // Normal sprite
          P[n+1]=(sprlinebuf+1)[n];
         else if(P[n+1]&64)        // behind bg sprite
          P[n+1]=(sprlinebuf+1)[n];
        }

        if(!(t&0x8000))
        {
         if(!(t&0x400000))       // Normal sprite
          P[n+2]=(sprlinebuf+2)[n];
         else if(P[n+2]&64)        // behind bg sprite
          P[n+2]=(sprlinebuf+2)[n];
        }

        if(!(t&0x80))
        {
         if(!(t&0x40000000))       // Normal sprite
          P[n+3]=(sprlinebuf+3)[n];
         else if(P[n+3]&64)        // behind bg sprite
          P[n+3]=(sprlinebuf+3)[n];
        }
	#endif
       }
      }
      n+=4;
      if(n) goto loopskie;
     }
}





/*
void FetchSpriteData(void)
{
        uint8 ns,sb;
        SPR *spr;
        uint8 H;
	int n;
	int vofs;
        uint8 P0=PPU[0];


        spr=(SPR *)SPRAM;
        H=8;

        ns=sb=0;

        vofs=(unsigned int)(P0&0x8&(((P0&0x20)^0x20)>>2))<<9;
        H+=(P0&0x20)>>2;

        if(!PPU_hook)
         for(n=63;n>=0;n--,spr++)
         {
                if((unsigned int)(scanline-spr->y)>=H) continue;
                //printf("%d, %u\n",scanline,(unsigned int)(scanline-spr->y));
                if(ns<maxsprites)
                {
                 if(n==63) sb=1;

                 {
                  SPRB dst;
                  uint8 *C;
                  int t;
                  unsigned int vadr;

                  t = (int)scanline-(spr->y);

                  if (Sprite16)
                   vadr = ((spr->no&1)<<12) + ((spr->no&0xFE)<<4);
                  else
                   vadr = (spr->no<<4)+vofs;

                  if (spr->atr&V_FLIP)
                  {
                        vadr+=7;
                        vadr-=t;
                        vadr+=(P0&0x20)>>1;
                        vadr-=t&8;
                  }
                  else
                  {
                        vadr+=t;
                        vadr+=t&8;
                  }

                  // Fix this geniestage hack
                  if(MMC5Hack && geniestage!=1) C = MMC5SPRVRAMADR(vadr);
                  else C = VRAMADR(vadr);


                  dst.ca[0]=C[0];
                  dst.ca[1]=C[8];
                  dst.x=spr->x;
                  dst.atr=spr->atr;

                  *(uint32 *)&SPRBUF[ns<<2]=*(uint32 *)&dst;
                 }

                 ns++;
                }
                else
                {
                  PPU_status|=0x20;
                  break;
                }
         }
        else
         for(n=63;n>=0;n--,spr++)
         {
                if((unsigned int)(scanline-spr->y)>=H) continue;

                if(ns<maxsprites)
                {
                 if(n==63) sb=1;

                 {
                  SPRB dst;
                  uint8 *C;
                  int t;
                  unsigned int vadr;

                  t = (int)scanline-(spr->y);

                  if (Sprite16)
                   vadr = ((spr->no&1)<<12) + ((spr->no&0xFE)<<4);
                  else
                   vadr = (spr->no<<4)+vofs;

                  if (spr->atr&V_FLIP)
                  {
                        vadr+=7;
                        vadr-=t;
                        vadr+=(P0&0x20)>>1;
                        vadr-=t&8;
                  }
                  else
                  {
                        vadr+=t;
                        vadr+=t&8;
                  }

                  if(MMC5Hack) C = MMC5SPRVRAMADR(vadr);
                  else C = VRAMADR(vadr);
                  dst.ca[0]=C[0];
		  if(ns<8)
		  {
		   PPU_hook(0x2000);
                   PPU_hook(vadr);
		  }
                  dst.ca[1]=C[8];
                  dst.x=spr->x;
                  dst.atr=spr->atr;


                  *(uint32 *)&SPRBUF[ns<<2]=*(uint32 *)&dst;
                 }

                 ns++;
                }
                else
                {
                  PPU_status|=0x20;
                  break;
                }
         }
        //if(ns>=7)
        //printf("%d %d\n",scanline,ns);
        if(ns>8) PPU_status|=0x20;	// Handle case when >8 sprites per
//				   scanline option is enabled.
	else if(PPU_hook)
	{
	 for(n=0;n<(8-ns);n++)
	 {
                 PPU_hook(0x2000);
                 PPU_hook(vofs);
	 }
	}
        numsprites=ns;
        SpriteBlurp=sb;
}



void RefreshSprite(uint8 *target)
{

	int n,sprindex;
	SPRB *spr;
        uint8 *P=target;

        //if (printed) {  printf("SPRB: %d  SPR: %d\n", sizeof(SPRB), sizeof(SPR)); printed=0; }
        if(!numsprites) return;

        FCEU_dwmemset(sprlinebuf,0x80808080,256);

        numsprites--;
        sprindex=numsprites;
        spr = (SPRB*)SPRBUF;

	//       for(n=nosprites;n>=0;n--,spr--)
       for(n=numsprites;n>=0;n--,sprindex--)
       {
        uint8 J,atr,c1,c2;
	int x=spr[sprindex].x;
        uint8 *C;
        uint8 *VB;

        P+=x;

        c1=((spr[sprindex].ca[0]>>1)&0x55)|(spr[sprindex].ca[1]&0xAA);
	c2=(spr[sprindex].ca[0]&0x55)|((spr[sprindex].ca[1]<<1)&0xAA);

        J=spr[sprindex].ca[0]|spr[sprindex].ca[1];
	atr=spr[sprindex].atr;

                       if(J)
                       {
                        if(n==0 && SpriteBlurp && !(PPU_status&0x40))
                        {
			 int z,ze=x+8;
			 if(ze>256) {ze=256;}
			 if(ScreenON && (scanline<FSettings.FirstSLine || scanline>FSettings.LastSLine
			 #ifdef FRAMESKIP
			 || FSkip
			 #endif
			 ))
			   // nothing wrong with this
			 BGRender(target);

			 if(!(atr&H_FLIP))
			 {
			  for(z=x;z<ze;z++)
			  {
			   if(J&(0x80>>(z-x)))
			   {
			    if(!(target[z]&64))
			     tosprite=z;
			   }
			  }
			 }
			 else
			 {
                          for(z=x;z<ze;z++)
                          {
                           if(J&(1<<(z-x)))
                           {
                            if(!(target[z]&64))
                             tosprite=z;
                           }
                          }
			 }
			 //FCEU_DispMessage("%d, %d:%d",scanline,x,tosprite);
                        }

	 //C = sprlinebuf+(uint8)x;
	 C = &(sprlinebuf[(uint8)x]);
         VB = (PALRAM+0x10)+((atr&3)<<2);

         if(atr&SP_BACK)
         {
          if (atr&H_FLIP)
          {
           if (J&0x02)  C[1]=VB[c1&3]|0x40;
           if (J&0x01)  *C=VB[c2&3]|0x40;
           c1>>=2;c2>>=2;
           if (J&0x08)  C[3]=VB[c1&3]|0x40;
           if (J&0x04)  C[2]=VB[c2&3]|0x40;
           c1>>=2;c2>>=2;
           if (J&0x20)  C[5]=VB[c1&3]|0x40;
           if (J&0x10)  C[4]=VB[c2&3]|0x40;
           c1>>=2;c2>>=2;
           if (J&0x80)  C[7]=VB[c1]|0x40;
           if (J&0x40)  C[6]=VB[c2]|0x40;
	  } else  {
           if (J&0x02)  C[6]=VB[c1&3]|0x40;
           if (J&0x01)  C[7]=VB[c2&3]|0x40;
	   c1>>=2;c2>>=2;
           if (J&0x08)  C[4]=VB[c1&3]|0x40;
           if (J&0x04)  C[5]=VB[c2&3]|0x40;
           c1>>=2;c2>>=2;
           if (J&0x20)  C[2]=VB[c1&3]|0x40;
           if (J&0x10)  C[3]=VB[c2&3]|0x40;
           c1>>=2;c2>>=2;
           if (J&0x80)  *C=VB[c1]|0x40;
           if (J&0x40)  C[1]=VB[c2]|0x40;
	  }
         } else {
          if (atr&H_FLIP)
	  {
           if (J&0x02)  C[1]=VB[(c1&3)];
           if (J&0x01)  *C=VB[(c2&3)];
           c1>>=2;c2>>=2;
           if (J&0x08)  C[3]=VB[(c1&3)];
           if (J&0x04)  C[2]=VB[(c2&3)];
           c1>>=2;c2>>=2;
           if (J&0x20)  C[5]=VB[(c1&3)];
           if (J&0x10)  C[4]=VB[(c2&3)];
           c1>>=2;c2>>=2;
           if (J&0x80)  C[7]=VB[c1];
           if (J&0x40)  C[6]=VB[c2];
          }else{
           if (J&0x02)  C[6]=VB[(c1&3)];
           if (J&0x01)  C[7]=VB[(c2&3)];
           c1>>=2;c2>>=2;
           if (J&0x08)  C[4]=VB[(c1&3)];
           if (J&0x04)  C[5]=VB[(c2&3)];
           c1>>=2;c2>>=2;
           if (J&0x20)  C[2]=VB[(c1&3)];
           if (J&0x10)  C[3]=VB[(c2&3)];
           c1>>=2;c2>>=2;
           if (J&0x80)  *C=VB[c1];
           if (J&0x40)  C[1]=VB[c2];
          }
         }
        }
       P-=x;
      }

     numsprites=0;
     #ifdef FRAMESKIP
     if(FSkip) return;
     #endif

     {
      uint8 n=((PPU[1]&4)^4)<<1;
      loopskie:
      {
       uint32 t=*((uint32 *)(&(sprlinebuf[n])));
       if(t!=0x80808080)
       {
	#ifdef LSB_FIRST
        if(!(t&0x80))
        {
         if(!(t&0x40))       // Normal sprite
          P[n]=sprlinebuf[n];
         else if(P[n]&64)        // behind bg sprite
          P[n]=sprlinebuf[n];
        }

        if(!(t&0x8000))
        {
         if(!(t&0x4000))       // Normal sprite
          P[n+1]=(sprlinebuf+1)[n];
         else if(P[n+1]&64)        // behind bg sprite
          P[n+1]=(sprlinebuf+1)[n];
        }

        if(!(t&0x800000))
        {
         if(!(t&0x400000))       // Normal sprite
          P[n+2]=(sprlinebuf+2)[n];
         else if(P[n+2]&64)        // behind bg sprite
          P[n+2]=(sprlinebuf+2)[n];
        }

        if(!(t&0x80000000))
        {
         if(!(t&0x40000000))       // Normal sprite
          P[n+3]=(sprlinebuf+3)[n];
         else if(P[n+3]&64)        // behind bg sprite
          P[n+3]=(sprlinebuf+3)[n];
        }
	#else
        if(!(t&0x80000000))
        {
         if(!(t&0x40))       // Normal sprite
          P[n]=sprlinebuf[n];
         else if(P[n]&64)        // behind bg sprite
          P[n]=sprlinebuf[n];
        }

        if(!(t&0x800000))
        {
         if(!(t&0x4000))       // Normal sprite
          P[n+1]=(sprlinebuf+1)[n];
         else if(P[n+1]&64)        // behind bg sprite
          P[n+1]=(sprlinebuf+1)[n];
        }

        if(!(t&0x8000))
        {
         if(!(t&0x400000))       // Normal sprite
          P[n+2]=(sprlinebuf+2)[n];
         else if(P[n+2]&64)        // behind bg sprite
          P[n+2]=(sprlinebuf+2)[n];
        }

        if(!(t&0x80))
        {
         if(!(t&0x40000000))       // Normal sprite
          P[n+3]=(sprlinebuf+3)[n];
         else if(P[n+3]&64)        // behind bg sprite
          P[n+3]=(sprlinebuf+3)[n];
        }
	#endif
       }
      }
      n+=4;
      if(n) goto loopskie;
     }
}


*/
