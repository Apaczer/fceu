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

#ifndef UNIFPRIV
int UNIFLoad(char *name, int fp);
#endif

void TCU01_Init(void);
void S8259B_Init(void);
void S8259A_Init(void);
void S74LS374N_Init(void);
void SA0161M_Init(void);

void SA72007_Init(void);
void SA72008_Init(void);
void SA0036_Init(void);
void SA0037_Init(void);

void H2288_Init(void);

void HKROM_Init(void);

void ETROM_Init(void);
void EKROM_Init(void);
void ELROM_Init(void);
void EWROM_Init(void);

void SAROM_Init(void);
void SBROM_Init(void);
void SCROM_Init(void);
void SEROM_Init(void);
void SGROM_Init(void);
void SKROM_Init(void);
void SLROM_Init(void);
void SL1ROM_Init(void);
void SNROM_Init(void);
void SOROM_Init(void);

void NROM_Init(void);
void NROM256_Init(void);
void NROM128_Init(void);
void MHROM_Init(void);
void UNROM_Init(void);
void MALEE_Init(void);
void Supervision16_Init(void);
void Super24_Init(void);
void Novel_Init(void);
void CNROM_Init(void);
void CPROM_Init(void);

void TFROM_Init(void);
void TGROM_Init(void);
void TKROM_Init(void);
void TSROM_Init(void);
void TLROM_Init(void);
void TLSROM_Init(void);
void TKSROM_Init(void);
void TQROM_Init(void);
void TQROM_Init(void);


void UNIFOpenWRAM(int t, char *ext, int override);
void UNIFWriteWRAM(uint8 *p, int size);
void UNIFReadWRAM(uint8 *p, int size);
void UNIFCloseWRAM(void);
#define	UOW_RD	0
#define UOW_WR	1

extern void (*BoardClose)(void);
extern void (*BoardPower)(void);
extern void (*BoardReset)(void);

#define UNIFMemBlock (GameMemBlock+32768)

extern int UNIFbattery;
extern char *UNIFchrrama;	// Meh.  So I can't stop CHR RAM 
	 			// bank switcherooing with certain boards...
