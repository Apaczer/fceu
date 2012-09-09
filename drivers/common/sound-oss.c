/* FCE Ultra - NES/Famicom Emulator
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
 */

#include <stdio.h>

#include "../../driver.h"
#include "../common/settings.h"
#include "../common/throttle.h"
#include "../libpicofe/linux/sndout_oss.h"


extern int soundvol;

void WriteSound(int16 *Buffer, int Count)
{
	sndout_oss_write_nb(Buffer, Count * 2);
}

void SilenceSound(int n)
{
	soundvol = 0;
}

int InitSound(void)
{
	FCEUI_Sound(Settings.sound_rate);
	sndout_oss_init();
	return 1;
}

uint32 GetMaxSound(void)
{
	return(4096);
}

uint32 GetWriteSound(void)
{
	return 1024;
}

void StartSound(void)
{
	 sndout_oss_start(Settings.sound_rate, 0, 2);
}

int KillSound(void)
{
	//FCEUI_Sound(0);

	return 1;
}

