/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 1998 BERO
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
#ifdef C80x86
{
 int dummy,dummy1,dummy2;
        __asm__ __volatile__(
	"xorl %%edx,%%edx\n\t"
        "movb (%%esi),%%cl\n\t"
        "movb 8(%%esi),%%dl\n\t"
        "movl %%ebx,%%esi\n\t"
        "addl %%eax,%%esi\n\t"
	"xorl %%ebx,%%ebx\n\t"
        "movb %%cl,%%bl\n\t"
        "movb %%dl,%%al\n\t"
        "shrb $1,%%bl\n\t"
        "andb $0xaa,%%al\n\t"
        "andb $0x55,%%bl\n\t"

        "andb $0x55,%%cl\n\t"
        "shlb $1,%%dl\n\t"
        "andb $0xaa,%%dl\n\t"
        "orb  %%al, %%bl\n\t"            // Stick c1 into bl
        "orb  %%cl, %%dl\n\t"           // Stick c2 into dl
        "xorl %%eax, %%eax\n\t"
        "xorl %%ecx, %%ecx\n\t"
        /*      At this point, bl contains c1, and dl contains c2 */
        /*      and edi contains P, esi contains VRAM[] */
        /*      al will be used for zz, cl will be used for zz2  */
        "movb %%bl,%%al\n\t"
        "movb %%dl,%%cl\n\t"
        "andb $3,%%al\n\t"
        "andb $3,%%cl\n\t"
        "movb (%%esi,%%eax),%%bh\n\t"
        "movb (%%esi,%%ecx),%%dh\n\t"
        "movb %%bh,6(%%edi)\n\t"
        "movb %%dh,7(%%edi)\n\t"

        "movb %%bl,%%al\n\t"
        "movb %%dl,%%cl\n\t"
        "shrb $2,%%al\n\t"
        "shrb $2,%%cl\n\t"
        "andb $3,%%al\n\t"
        "andb $3,%%cl\n\t"
        "movb (%%esi,%%eax),%%bh\n\t"
        "movb (%%esi,%%ecx),%%dh\n\t"
        "movb %%bh,4(%%edi)\n\t"
        "movb %%dh,5(%%edi)\n\t"

        "movb %%bl,%%al\n\t"
        "movb %%dl,%%cl\n\t"
        "shrb $4,%%al\n\t"
        "shrb $4,%%cl\n\t"
        "andb $3,%%al\n\t"
        "andb $3,%%cl\n\t"
        "movb (%%esi,%%eax),%%bh\n\t"
        "movb (%%esi,%%ecx),%%dh\n\t"
        "movb %%bh,2(%%edi)\n\t"
        "movb %%dh,3(%%edi)\n\t"

//        "movb %%bl,%%al\n\t"
//        "movb %%dl,%%cl\n\t"
	"xorb %%bh,%%bh\n\t"
	"xorb %%dh,%%dh\n\t"
        "shrb $6,%%bl\n\t"
        "shrb $6,%%dl\n\t"
        "movb (%%esi,%%ebx),%%al\n\t"
        "movb (%%esi,%%edx),%%cl\n\t"
        "movb %%al,0(%%edi)\n\t"
        "movb %%cl,1(%%edi)\n\t"
	: "=S" (dummy), "=a" (dummy1), "=b" (dummy2)
        : "D" (P), "S" (C), "a" (cc), "b" (PALRAM)
        : "%ecx", "%edx"
        );

}
#else
        {
	 uint8 *S=PALRAM+cc;
	 register uint8 c1,c2;
         
	 c1=((C[0]>>1)&0x55)|(C[8]&0xAA);
         c2=(C[0]&0x55)|((C[8]<<1)&0xAA);

         P[6]=S[c1&3];
         P[7]=S[c2&3];
         P[4]=S[(c1>>2)&3];
         P[5]=S[(c2>>2)&3];
         P[2]=S[(c1>>4)&3];
         P[3]=S[(c2>>4)&3];

         P[0]=S[c1>>6];
         P[1]=S[c2>>6];
        }
#endif
