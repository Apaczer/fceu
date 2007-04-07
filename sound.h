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

#define OVERSAMPLESHIFT 4
#define OVERSAMPLE *16
#define SND_BUFSIZE 256


typedef struct {
	   void (*Fill)(int Count);
	   void (*RChange)(void);
	   void (*Kill)(void);
} EXPSOUND;

extern EXPSOUND GameExpSound;

extern int64 nesincsizeLL;
extern uint8 PSG[];
extern uint32 PSG_base;
extern int32 PCMIRQCount;

void SetSoundVariables(void);
void PowerSound(void);
void ResetSound(void);
extern uint8 decvolume[];

extern int vdis;
extern uint8 sqnon;
extern uint16 nreg;

extern uint8 trimode;
extern uint8 tricoop;
extern uint8 PCMBitIndex;
extern uint32 PCMAddressIndex;
extern int32 PCMSizeIndex;
extern uint8 PCMBuffer;

extern uint8 sweepon[2];
extern int32 curfreq[2];

extern uint8 SweepCount[2];
extern uint8 DecCountTo1[3];

extern uint8 fcnt;
extern int32 fhcnt;
extern int32 fhinc;

void GetSoundBuffer(int32 **W);
int FlushEmulateSound(void);
extern uint32 Wave[2048];
extern int32 WaveFinal[2048];
extern uint32 soundtsinc;

void SetNESSoundMap(void);
void FrameSoundUpdate(void);
void FixOldSaveStateSFreq(void);
