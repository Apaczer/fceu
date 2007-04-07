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

#include <stdio.h>
#include "sdl.h"

#ifndef DSPSOUND

//#define BSIZE (32768-1024)

static int32 BSIZE;
static volatile int16 AudioBuf[32768];
static volatile uint32 readoffs,writeoffs;
void fillaudio(void *udata, uint8 *stream, int len)
{
 int16 *dest=(int16 *)stream;

 len>>=1;
 while(len)
 {
  *dest=AudioBuf[readoffs];
  dest++;
  readoffs=(readoffs+1)&32767;
  len--;
 }
}

void WriteSound(int32 *Buffer, int Count, int NoWaiting)
{
 while(Count)
 {
  while(writeoffs==((readoffs-BSIZE)&32767)) 
   if(NoWaiting)
    return;
  AudioBuf[writeoffs]=*Buffer;
  writeoffs=(writeoffs+1)&32767;
  Buffer++;
  Count--;
 }
}

int InitSound(void)
{
 if(_sound)
 {
  SDL_AudioSpec spec;

  if(_lbufsize<_ebufsize) 
  {
   puts("Ack, lbufsize must not be smaller than ebufsize!");
   return(0);
  }
  if(_lbufsize<6 || _lbufsize>13)
  {
   puts("lbufsize out of range");
   return(0);
  }
  if(_ebufsize<5)
  {
   puts("ebufsize out of range");
   return(0);
  }
  memset(&spec,0,sizeof(spec));
  if(SDL_InitSubSystem(SDL_INIT_AUDIO)<0)
  {
   puts(SDL_GetError());
   return(0);
  }
  if(_sound==1) _sound=44100;
  spec.freq=_sound;
  spec.format=AUDIO_S16;
  spec.channels=1;
  spec.samples=1<<_ebufsize;
  spec.callback=fillaudio;
  spec.userdata=0;

  if(SDL_OpenAudio(&spec,0)<0)
  {
   puts(SDL_GetError());
   SDL_QuitSubSystem(SDL_INIT_AUDIO);
   return(0);
  }
  FCEUI_Sound(_sound);
  BSIZE=32768-(1<<_lbufsize);
  SDL_PauseAudio(0);
  return(1);
 }
 return(0);
}

void SilenceSound(int n)
{
 SDL_PauseAudio(n);

}

void KillSound(void)
{
 SDL_CloseAudio();
 SDL_QuitSubSystem(SDL_INIT_AUDIO);
}

#else
#include "../common/unixdsp.h"

void WriteSound(int32 *Buffer, int Count, int NoWaiting)
{
  WriteUNIXDSPSound(Buffer, Count, NoWaiting);
}

int InitSound(void)
{
        if(_sound)
        {
         int rate;
         if(_sound==1)
          _sound=48000;
         rate=_sound;
         if(InitUNIXDSPSound(&rate,_f8bit?0:1,8,8))
         {
          FCEUI_Sound(rate);
          return(1);
         }
        }
        return(0);
}
void KillSound(void)
{
        KillUNIXDSPSound();
}
#endif
