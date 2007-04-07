#include <sys/time.h>
#include "main.h"
#include "throttle.h"

#if 0
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

#else

extern uint8 PAL;
extern int FSkip, FSkip_setting;
static int usec_aim = 0, usec_done = 0;
static int skip_count = 0;

INLINE void SpeedThrottle(void)
{
	static struct timeval tv_prev;
	struct timeval tv_now;
	int delta_nom = PAL ? 19997 : 16639; // ~50.007, 19.997 ms/frame : ~60.1, 16.639 ms/frame


	if (usec_done == 0) { // first time
		usec_done = 1;
		gettimeofday(&tv_prev, 0);
		return;
	}

	gettimeofday(&tv_now, 0);

	usec_aim += delta_nom;
	if (tv_now.tv_sec != tv_prev.tv_sec)
		usec_done += 1000000;
	usec_done += tv_now.tv_usec - tv_prev.tv_usec;

#ifdef FRAMESKIP
	if (FSkip_setting >= 0)
	{
		if (skip_count >= FSkip_setting)
			skip_count = 0;
		else {
			skip_count++;
			FSkip = 1;
		}
	}
	else if (usec_done > usec_aim)
	{
		/* auto frameskip */
		if (usec_done - usec_aim > 150000)
			usec_done = usec_aim = 0; // too much behind, try to recover..
		FSkip = 1;
		tv_prev = tv_now;
		return;
	}
#endif

	tv_prev = tv_now;
	while (usec_done < usec_aim)
	{
		gettimeofday(&tv_now, 0);

		if (tv_now.tv_sec != tv_prev.tv_sec)
			usec_done += 1000000;
		usec_done += tv_now.tv_usec - tv_prev.tv_usec;
		tv_prev = tv_now;
	}
	usec_done = usec_done - usec_aim + 1; // reset to prevent overflows
	usec_aim = 0;
}
#endif

