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

static uint64 tmethod,tfreq;
static uint64 desiredfps;

static void RefreshThrottleFPS(void)
{
 desiredfps=FCEUI_GetDesiredFPS()>>8;
}

static uint64 GetCurTime(void)
{
 if(tmethod)
 {
  uint64 tmp;

  /* Practically, LARGE_INTEGER and uint64 differ only by signness and name. */
  QueryPerformanceCounter((LARGE_INTEGER*)&tmp);

  return(tmp);
 }
 else
  return((uint64)GetTickCount());

}

static void InitSpeedThrottle(void)
{
 tmethod=0;
 if(QueryPerformanceFrequency((LARGE_INTEGER*)&tfreq))
 {
  tmethod=1;
 }
 else
  tfreq=1000;
 tfreq<<=16;    /* Adjustment for fps returned from FCEUI_GetDesiredFPS(). */
}


static void SpeedThrottle(void)
{
 static uint64 ttime,ltime;

 waiter:

 ttime=GetCurTime();

                            
 if( (ttime-ltime) < (tfreq/desiredfps) )
  goto waiter;
 if( (ttime-ltime) >= (tfreq*4/desiredfps))
  ltime=ttime;
 else
  ltime+=tfreq/desiredfps;
}
