/* Is this an MMC3 workalike piece of hardware, with the addition of
   a register at $4120 or does it have only partial MMC3 functionality?
   A good test would be to see if commands 6 and 7 can change PRG banks
   and of course test the regs >=$c000, on the real cart.
*/
#include "mapinc.h"

#define cmd mapbyte1[0]
static DECLFW(Mapper189_write)
{
 if(A==0x4120) ROM_BANK32(V>>4);
 else switch(A&0xE001)
 {
  case 0xa000:MIRROR_SET(V&1);break;
  case 0x8000:cmd=V;break; 
  case 0x8001:switch(cmd&7)
	      {
	       case 0:VROM_BANK2(0x0000,V>>1);break;
	       case 1:VROM_BANK2(0x0800,V>>1);break;
	       case 2:VROM_BANK1(0x1000,V);break;
               case 3:VROM_BANK1(0x1400,V);break;
               case 4:VROM_BANK1(0x1800,V);break;
               case 5:VROM_BANK1(0x1C00,V);break;
	      }
	      break;

 }
}

void Mapper189_init(void)
{
  SetWriteHandler(0x4120,0xFFFF,Mapper189_write);
  SetReadHandler(0x6000,0x7FFF,0);
  ROM_BANK32(0);
}


