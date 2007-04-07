#include <sys/time.h>
#include "main.h"
#include "throttle.h"

static uint64 tfreq;
static uint64 desiredfps;

void RefreshThrottleFPS(void)
{
 uint64 f=FCEUI_GetDesiredFPS();
 // great, a bit faster than before
 //f = (f*65) >> 6;
 desiredfps=f>>8;
 tfreq=1000000;
 tfreq<<=16;    /* Adjustment for fps returned from FCEUI_GetDesiredFPS(). */
}

static uint64 GetCurTime(void)
{
 uint64 ret;
 struct timeval tv;

 gettimeofday(&tv,0);
 ret=(uint64)tv.tv_sec*1000000;
 ret+=tv.tv_usec;
 return(ret);
}

INLINE void SpeedThrottle(void)
{
 static uint64 ttime,ltime;

 waiter:

 ttime=GetCurTime();

 if( (ttime-ltime) < (tfreq/desiredfps) )
 {
  goto waiter;
 }
 if( (ttime-ltime) >= (tfreq*4/desiredfps))
  ltime=ttime;
 else
  ltime+=tfreq/desiredfps;
}

