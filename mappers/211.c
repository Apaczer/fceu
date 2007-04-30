#include "mapinc.h"

static uint8 mul[2];
static uint8 regie;
#define tkcom1 mapbyte1[1]
#define tkcom2 mapbyte1[2]
#define tkcom3 mapbyte1[3]

#define prgb        mapbyte2
#define chr     mapbyte3

static uint16 names[4];
static uint8  tekker;

static DECLFR(tekread)
{
// FCEU_printf("READ READ READ: $%04x, $%04x, $%04x\n",A,X.PC,tekker);
 switch(A)
 {
  case 0x5000:return(tekker);
  case 0x5800:return(mul[0]*mul[1]);
  case 0x5801:return((mul[0]*mul[1])>>8);
  case 0x5803:return(regie);
  default:break;
 }
 return(X.DB);
}

static void mira(void)
{
  int x;
  if(tkcom1&0x40)        // Name tables are ROM-only
  {
   for(x=0;x<4;x++)
    setntamem(CHRptr[0]+(((names[x])&CHRmask1[0])<<10), 0, x);
  }
  else                        // Name tables can be RAM or ROM.
  {
  for(x=0;x<4;x++)
  {
   if(((tkcom3&0x80)==(names[x]&0x80)))        // RAM selected.
    setntamem(NTARAM + ((names[x]&0x1)<<10),1,x);
   else
    setntamem(CHRptr[0]+(((names[x])&CHRmask1[0])<<10), 0, x);
  }
  }
}

static void tekprom(void)
{
 switch(tkcom1&3)
  {
   case 1:              // 16 KB
          ROM_BANK16(0x8000,prgb[0]);
          ROM_BANK16(0xC000,prgb[2]);
          break;

   case 2:              //2 = 8 KB ??
          ROM_BANK8(0x8000,prgb[0]);
          ROM_BANK8(0xa000,prgb[1]);
          ROM_BANK8(0xc000,prgb[2]);
          ROM_BANK8(0xe000,~0);
               break;
  }
}

static void tekvrom(void)
{
 int x;
 switch(tkcom1&0x18)
  {
   case 0x00:      // 8KB
           setchr8(chr[0]);
               break;
   case 0x08:      // 4KB
          for(x=0;x<8;x+=4)
           setchr4(x<<10,chr[x]);
              break;
   case 0x10:      // 2KB
              for(x=0;x<8;x+=2)
           setchr2(x<<10,chr[x]);
              break;
   case 0x18:      // 1KB
              for(x=0;x<8;x++)
               setchr1(x<<10,chr[x]);
              break;
 }
}

static DECLFW(Mapper211_write)
{
 if(A==0x5800) mul[0]=V;
 else if(A==0x5801) mul[1]=V;
 else if(A==0x5803) regie=V;

 A&=0xF007;
 if(A>=0x8000 && A<=0x8003)
 {
  prgb[A&3]=V;
  tekprom();
 }
 else if(A>=0x9000 && A<=0x9007)
 {
  chr[A&7]=V;
  tekvrom();
 }
 else if(A>=0xb000 && A<=0xb007)
 {
   names[A&3]=V&3;
   mira();
 }
 else switch(A)
 {
   case 0xc002:IRQa=0;X6502_IRQEnd(FCEU_IQEXT);break;
   case 0xc003:
   case 0xc004:if(!IRQa) {IRQa=1;IRQCount=IRQLatch;} break;
   case 0xc005:IRQCount=IRQLatch=V;
               X6502_IRQEnd(FCEU_IQEXT);break;
   case 0xd000: tkcom1=V; mira(); break;
   case 0xd001: tkcom3=V; mira(); break;
 }
}

static void Mapper211_hb(void)
{
 if(IRQCount) IRQCount--;
 if(!IRQCount)
 {
  if(IRQa) X6502_IRQBegin(FCEU_IQEXT);
  IRQa=0;
 }
}
static void togglie()
{
  tekker^=0xFF;
}

void Mapper211_init(void)
{
  tekker=0xFF;
  MapperReset=togglie;
  SetWriteHandler(0x5000,0xffff,Mapper211_write);
  SetReadHandler(0x5000,0x5fff,tekread);
  GameHBIRQHook=Mapper211_hb;
}

