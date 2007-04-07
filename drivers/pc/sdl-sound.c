/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2002 Xodnizel
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
#include <stdlib.h>
#include <sys/time.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/soundcard.h>

#include "sdl.h"
#include "minimal.h"

#define GP2X_SOUND 1

extern void pthread_yield(void);
extern void FCEUI_FrameSkip(int x);

//extern int dosound;
extern int soundvol;

// always have this call
INLINE void gp2x_sound_frame(void *blah, void *buff, int samples)
 {
}





//static int sexy_in_function=0;
#define NumSexyBufferBuffers 2
struct timespec gp2x_sleep_ts;
int firstentry=1;
//int firstprint=1;
struct timeval sleeptimer;



pthread_t       gp2x_sexy_sound_thread=0;
int**  SexyBufferBuffers=NULL;
int    SexyBufferBufferCounts[NumSexyBufferBuffers];
int  gp2x_sound_inited=0;
int gp2x_in_sound_thread=0;
extern unsigned long   gp2x_dev[8];

pthread_cond_t gp2x_sound_cond=PTHREAD_COND_INITIALIZER;
pthread_mutex_t gp2x_sound_mutex = PTHREAD_MUTEX_INITIALIZER;
int zzdebug01_entry=0;
int zzdebug01_wait=0;
int hasSound=0;
extern unsigned long fps;
extern unsigned long avg_fps;
extern unsigned long ticks;

int throttlecount=0;

void WriteSound(int32 *Buffer, int Count)
  {
  write(gp2x_dev[3], Buffer, Count<<1);
  pthread_yield();
  SpeedThrottle();
  }

void* gp2x_write_sound(void* blah)
  {
  gp2x_in_sound_thread=1;
  {
    if (hasSound)
  {
      hasSound=0;
  }
    else
  {
  }

 }
  gp2x_in_sound_thread=0;
  return NULL;
}

void SilenceSound(int n)
{
  soundvol=0;
}

int InitSound(FCEUGI *gi)
{
  Settings.sound=22050;
  FCEUI_Sound(Settings.sound);
  gp2x_sound_volume(soundvol, soundvol);
  printf("InitSound() sound_rate: %d\n", Settings.sound);
  if(firstentry)
  {
      firstentry=0;
      gp2x_sound_inited=1;
  }
  return 1 ;
}

uint32 GetMaxSound(void)
{
 return(4096);
}

uint32 GetWriteSound(void)
        {
 return 1024;
}

int KillSound(void)
{
  FCEUI_Sound(0);
  gp2x_sound_inited=0;

  return 1;
}




