/*
  GP2X minimal library v0.A by rlyeh, 2005. emulnation.info@rlyeh (swap it!)

  + GP2X 920t/940t CPUs library with a FIFO message system.
  + GP2X video library with double buffering.
  + GP2X sound library with double buffering.
  + GP2X blitter library.
  + GP2X timer library.
  + GP2X joystick library.

  Thanks to Squidge, Robster, snaff, Reesy and NK, for the help & previous work! :-)


  License
  =======

  Free for non-commercial projects (it would be nice receiving a mail from you).
  Other cases, ask me first.

  GamePark Holdings is not allowed to use this library and/or use parts from it.


  Known projects using the library or parts from it
  =================================================

  REminiscence-0.1.8 (rlyeh)
  Atari800 GP2X pre-release 3 (foft)
  XUMP (kedo)
  MAME2X (Franxis)
  DrMD for GP2X (Reesy)


  What's new
  ==========

  0.A: 940t disabled all time unless required
       sound is paused by default now, use gp2x_sound_pause() to unlock it

       fixed functions:
        - gp2x_sound_play()                        ; increased the number of sampling buffers
        - gp2x_timer_read()                        ; it should work again. broken at some point before
        - gp2x_dualcore_launch_program_from_disk() ; it missed the code uploading call

       added functions:
        - gp2x_sound_pause()
        - gp2x_timer_delay()
        - gp2x_dualcore_pause()

  0.9: initial FIFO message system for dual cpu cores.
       initial 48 Mb support.
       initial quadruple buffering in 8bbp mode.

       added functions:
        - gp2x_dualcore_exec() ; initial FIFO message system for dual cpu cores.
        - gp2x_dualcore_sync() ; initial FIFO message system for dual cpu cores.

       improved functions:
        - gp2x_video_flip()    ; initial quadruple buffering in 8bbp mode.

  0.8: initial dual cores cpu support.
       very basic blit functions by popular demand ;-)
       vsync & hsync waiting code (thanks Reesy)

       added functions:
        - gp2x_launch_program()            ; initial dual cores cpu support.
        - gp2x_launch_program_from_disk()  ; initial dual cores cpu support.
        - gp2x_launch_subprogram()         ; initial dual cores cpu support.
        - gp2x_blitter_rect15()            ; very basic blit function by popular demand ;-)
        - gp2x_blitter_rect8()             ; very basic blit function by popular demand ;-)
        - gp2x_video_hsync()               ; hsync waiting code (thanks Reesy)
        - gp2x_video_vsync()               ; vsync waiting code (thanks Reesy)

       fixed functions:
        - gp2x_video_color8()              ; bugfixed a stupid typo (thanks Franxis for the bug report)

  0.7: added functions:
        - gp2x_sound_volume()

       fixed functions:
        - gp2x_deinit()           ; fixed menu launch code when exiting.

       improved functions:
        - gp2x_timer_read()       ; rewritten timer. it should be more accurate now.
        - gp2x_init()             ; faster init code.

  0.6: added functions:
        - gp2x_timer_read()
        - gp2x_sound_pause()

       fixed functions:
        - gp2x_video_setpalette() ; palette handling was incorrect. fixed.

  0.5: improved functions:
        - gp2x_init()             ; better and cleaner initalizing code.
        - gp2x_init()             ; sound patched for real stereo output (by using NK's solution)

  0.4: lots of cleanups.
       sound is threaded and double buffered now.
       8 bpp video support.

       fixed functions:
        - gp2x_deinit()           ; better and cleaner exiting code.

  0.3: shorter library.

       fixed functions:
        - gp2x_joystick_read()    ; improved joystick diagonal detection.

  0.2: better code layout.
       public release.

  0.1: beta release.
*/

#include "minimal.h"
#include "squidgehack.h"
#include "asmutils.h"

         unsigned char  *gp2x_screen8                 ,*gp2x_upperRAM;
         unsigned long   gp2x_dev[8]={0,0,0,0,0,0,0,0}, gp2x_physvram[8], *gp2x_memregl,       gp2x_volume,                      gp2x_ticks_per_second;
volatile unsigned long   gp2x_sound_pausei=1,           gp2x_ticks=0,      gp2x_sound=0,      *gp2x_dualcore_ram=0;
         unsigned short *gp2x_memregs,                 *gp2x_screen15,    *gp2x_logvram15[4],  gp2x_sound_buffer[4+((44100/25)*2)*8]; //25 Hz gives our biggest supported sampling buffer
volatile unsigned short  gp2x_palette[512];

extern void gp2x_sound_frame(void *blah, void *buff, int samples);

// hack
static int scaling_enabled = 0;

void gp2x_video_flip(void)
{
  unsigned long address=gp2x_physvram[gp2x_physvram[7]];
  if (scaling_enabled) address += 32;

  // since we are using the mmu hack, we must flush the cache first
  // (the params are most likely wrong, but they seem to work somehow)
  flushcache(address, address + 320*240, 0);

  if(++gp2x_physvram[7]==4) gp2x_physvram[7]=0;
  gp2x_screen15=gp2x_logvram15[gp2x_physvram[7]];
  gp2x_screen8=(unsigned char *)gp2x_screen15;

  gp2x_memregs[0x290E>>1]=(unsigned short)(address & 0xFFFF);
  gp2x_memregs[0x2910>>1]=(unsigned short)(address >> 16);
  gp2x_memregs[0x2912>>1]=(unsigned short)(address & 0xFFFF);
  gp2x_memregs[0x2914>>1]=(unsigned short)(address >> 16);
}

void gp2x_video_flip_single(void)
{
  unsigned long address=gp2x_physvram[0];

  gp2x_screen15=gp2x_logvram15[0];
  gp2x_screen8=(unsigned char *)gp2x_screen15;

  gp2x_memregs[0x290E>>1]=(unsigned short)(address & 0xFFFF);
  gp2x_memregs[0x2910>>1]=(unsigned short)(address >> 16);
  gp2x_memregs[0x2912>>1]=(unsigned short)(address & 0xFFFF);
  gp2x_memregs[0x2914>>1]=(unsigned short)(address >> 16);
}


void gp2x_video_setgamma(unsigned short gamma) /*0..255*/
{
  int i=256*3;
  gp2x_memregs[0x295C>>1]=0;
  while(i--) gp2x_memregs[0x295E>>1]=gamma;
}

void gp2x_video_setpalette(void)
{
  unsigned short *g=(unsigned short *)gp2x_palette; int i=512;
  gp2x_memregs[0x2958>>1]=0;
  while(i--) gp2x_memregs[0x295A>>1]=*g++;
}

void gp2x_blitter_rect15(gp2x_rect *r)
{
 int x, y; unsigned short *data=r->data15, *offset=&gp2x_screen15[r->x+r->y*320];

 y=r->h; if(r->solid)
         while(y--) { x=r->w; while(x--) *offset++=*data++; offset+=320-x; }
         else
         while(y--) { x=r->w; while(x--) { if(*data) *offset=*data; offset++, data++; }
                      offset+=320-x; }
}

void gp2x_blitter_rect8(gp2x_rect *r)
{
 int x, y; unsigned char *data=r->data8,   *offset=&gp2x_screen8[r->x+r->y*320];

 y=r->h; if(r->solid)
         while(y--) { x=r->w; while(x--) *offset++=*data++; offset+=320-x; }
         else
         while(y--) { x=r->w; while(x--) { if(*data) *offset=*data; offset++, data++; }
                      offset+=320-x; }
}

unsigned long gp2x_joystick_read(void)
{
  unsigned long value=(gp2x_memregs[0x1198>>1] & 0x00FF);

  if(value==0xFD) value=0xFA;
  if(value==0xF7) value=0xEB;
  if(value==0xDF) value=0xAF;
  if(value==0x7F) value=0xBE;

  return ~((gp2x_memregs[0x1184>>1] & 0xFF00) | value | (gp2x_memregs[0x1186>>1] << 16));
}

void gp2x_sound_volume(int l, int r)
{
 l=(((l*0x50)/100)<<8)|((r*0x50)/100);          //0x5A, 0x60
 ioctl(gp2x_dev[4], SOUND_MIXER_WRITE_PCM, &l); //SOUND_MIXER_WRITE_VOLUME
}

void gp2x_timer_delay(unsigned long ticks)
{
 unsigned long target=gp2x_memregl[0x0A00>>2]+ticks*gp2x_ticks_per_second;
 while(gp2x_memregl[0x0A00>>2]<target);
}

unsigned long gp2x_timer_read(void)
{
 return gp2x_memregl[0x0A00>>2]/gp2x_ticks_per_second;
}

unsigned long gp2x_timer_read_ms(void)
{
 return gp2x_memregl[0x0A00>>2];
}


void gp2x_sound_pause(int yes) { gp2x_sound_pausei=yes; }

#if 0
static void *gp2x_sound_play(void *blah)
{
  int flip=0, flyp=gp2x_sound_buffer[1];
  struct timespec ts; ts.tv_sec=0, ts.tv_nsec=gp2x_sound_buffer[2];

  while(!gp2x_sound)
  {
    nanosleep(&ts, NULL);


   if(!gp2x_sound_pausei)
   {
     // [1] is sound buffer size     at 22050, 16, stereo, it is 1468 (367*4)
     // [0] number of bytes?,  at 22050,16,stereo, it is 367
     // first half of buffer

     // first one is 368


    gp2x_sound_frame(blah, (void *)(&gp2x_sound_buffer[4+flip]), gp2x_sound_buffer[0]);
    // write out to second half of buffer
    write(gp2x_dev[3],     (void *)(&gp2x_sound_buffer[4+flyp]), gp2x_sound_buffer[1]);

    flip+=gp2x_sound_buffer[1];
         if(flip==gp2x_sound_buffer[1]*8) flip=0;
   flyp+=gp2x_sound_buffer[1];
   if(flyp==gp2x_sound_buffer[1]*8) flyp=0;
   }
  }
  return NULL;
}
#endif

#if 0
static void gp2x_initqueue(gp2x_queue *q, unsigned long queue_items, unsigned long *position920t, unsigned long *position940t)
{
  q->head  = q->tail  = q->items = 0;
  q->max_items = queue_items;
  if(position920t) q->place920t=position920t; else q->place920t=(unsigned long *)malloc(sizeof(unsigned long) * queue_items);
  if(position940t) q->place940t=position940t;
  memset(q->place920t, 0, sizeof(unsigned long) * queue_items);
}

static void gp2x_enqueue(gp2x_queue *q, unsigned long data)
{
  while(q->items==q->max_items); /*waiting for tail to decrease...*/
  q->place920t[q->head = (q->head < q->max_items ? q->head+1 : 0)] = data;
  q->items++;
}

/* UNUSED
 static unsigned long gp2x_dequeue(gp2x_queue *q)
{
  while(!q->items); //waiting for head to increase...
  q->items--;
  return q->place920t[q->tail = (q->tail < q->max_items ? q->tail+1 : 0)];
} */
#endif

       void gp2x_dualcore_pause(int yes) { if(yes) gp2x_memregs[0x0904>>1] &= 0xFFFE; else gp2x_memregs[0x0904>>1] |= 1; }
static void gp2x_940t_reset(int yes)     { gp2x_memregs[0x3B48>>1] = ((yes&1) << 7) | (0x03); }
static void gp2x_940t_pause(int yes)     { gp2x_dualcore_pause(yes); }

static void gp2x_dualcore_registers(int save)
{
 static unsigned short regs[8];

 if(save)
 {
  regs[0]=gp2x_memregs[0x0904>>1];  regs[1]=gp2x_memregs[0x0912>>1];
  regs[2]=gp2x_memregs[0x091c>>1];  regs[3]=gp2x_memregs[0x3b40>>1];
  regs[4]=gp2x_memregs[0x3b42>>1];  regs[5]=gp2x_memregs[0x3b48>>1];
  regs[6]=gp2x_memregs[0x3b44>>1];  regs[7]=gp2x_memregs[0x3b46>>1];

  gp2x_940t_reset(1);
  gp2x_940t_pause(1);
 }
 else
 {
  gp2x_memregs[0x0904>>1]=regs[0];  gp2x_memregs[0x0912>>1]=regs[1];
  gp2x_memregs[0x091c>>1]=regs[2];  gp2x_memregs[0x3b40>>1]=regs[3];
  gp2x_memregs[0x3b42>>1]=regs[4];  gp2x_memregs[0x3b48>>1]=regs[5];
  gp2x_memregs[0x3b44>>1]=regs[6];  gp2x_memregs[0x3b46>>1]=regs[7];
 }
}

#if 0
void gp2x_dualcore_sync(void)
{
  gp2x_queue *q=(gp2x_queue *)gp2x_1stcore_data_ptr(GP2X_QUEUE_ARRAY_PTR);
  while(q->items);
}

void gp2x_dualcore_exec(unsigned long command) { gp2x_enqueue((gp2x_queue *)gp2x_1stcore_data_ptr(GP2X_QUEUE_ARRAY_PTR),command); }

void gp2x_dualcore_launch_program(unsigned long *area, unsigned long size)
{
  unsigned long i=0, *arm940t_ram=(unsigned long *)gp2x_dualcore_ram;

  gp2x_940t_reset(1);

  gp2x_memregs[0x3B40>>1] = 0;                               //disable interrupts
  gp2x_memregs[0x3B42>>1] = 0;
  gp2x_memregs[0x3B44>>1] = 0xffff;
  gp2x_memregs[0x3B46>>1] = 0xffff;

  gp2x_940t_pause(0);

  while(i < size) *arm940t_ram++=area[i++];

  gp2x_initqueue((gp2x_queue *)gp2x_1stcore_data_ptr(GP2X_QUEUE_ARRAY_PTR), GP2X_QUEUE_MAX_ITEMS, (unsigned long *)gp2x_1stcore_data_ptr(GP2X_QUEUE_DATA_PTR), (unsigned long *)gp2x_2ndcore_data_ptr(GP2X_QUEUE_DATA_PTR));

  gp2x_940t_reset(0);
}

void gp2x_dualcore_launch_program_from_disk(const char *file, unsigned long offset, unsigned long size)
{
 FILE *in; void *data;

 if((in=fopen(file, "rb"))==NULL) return;
 if((data=malloc(size))==NULL) { fclose(in); return; }
 fseek(in, 0L, offset);
 fread(data, 1, size, in);
 gp2x_dualcore_launch_program((unsigned long *)data, size);
 free(data);
 fclose(in);
}
#endif

void gp2x_init(int ticks_per_second, int bpp, int rate, int bits, int stereo, int Hz)
{
  struct fb_fix_screeninfo fixed_info;
  static int first=1;

  // GP2X_DO_SOUND
  int gp2x_do_sound=1;
  int frag=0;
  int channels=1;
  int stereoLocal=0;
  int ret;

  gp2x_ticks_per_second=7372800/ticks_per_second;

  if(!gp2x_dev[0])   gp2x_dev[0] = open("/dev/fb0",   O_RDWR);
  if(!gp2x_dev[1])   gp2x_dev[1] = open("/dev/fb1",   O_RDWR);
  if(!gp2x_dev[2])   gp2x_dev[2] = open("/dev/mem",   O_RDWR);

  ioctl(gp2x_dev[gp2x_physvram[7]=0], FBIOGET_FSCREENINFO, &fixed_info);
   gp2x_physvram[2]=gp2x_physvram[0]=fixed_info.smem_start;
   gp2x_screen15=gp2x_logvram15[2]=gp2x_logvram15[0]=
    (unsigned short *)mmap(0, 0x20000*2, PROT_READ|PROT_WRITE, MAP_SHARED, gp2x_dev[2], gp2x_physvram[0]);
                                        gp2x_screen8=(unsigned char *)gp2x_screen15;
  printf("/dev/fb0 is @ %08lx / %p\n", gp2x_physvram[0], gp2x_screen15);

  ioctl(gp2x_dev[1], FBIOGET_FSCREENINFO, &fixed_info);
   gp2x_physvram[3]=gp2x_physvram[1]=fixed_info.smem_start;
   gp2x_logvram15[3]=gp2x_logvram15[1]=
    (unsigned short *)mmap(0, 0x20000*2, PROT_READ|PROT_WRITE, MAP_SHARED, gp2x_dev[2], gp2x_physvram[1]);
  printf("/dev/fb1 is @ %08lx / %p\n", gp2x_physvram[1], gp2x_logvram15[1]);

  // apply the MMU hack
  ret = mmuhack();
  printf("squidge hack code finished and returned %s\n", ret ? "ok" : "fail");

  // don't run sound right now
  if ( gp2x_do_sound)
  {
    if(!gp2x_dev[3])   gp2x_dev[3] = open("/dev/dsp",   O_WRONLY|O_ASYNC);
    if(!gp2x_dev[4])   gp2x_dev[4] = open("/dev/mixer", O_RDWR);
  }

//  gp2x_dualcore_ram=(unsigned long  *)mmap(0, 0x1000000, PROT_READ|PROT_WRITE, MAP_SHARED, gp2x_dev[2], 0x03000000);
/*gp2x_dualcore_ram=(unsigned long  *)mmap(0, 0x2000000, PROT_READ|PROT_WRITE, MAP_SHARED, gp2x_dev[2], 0x02000000);*/
       gp2x_memregl=(unsigned long  *)mmap(0, 0x10000,   PROT_READ|PROT_WRITE, MAP_SHARED, gp2x_dev[2], 0xc0000000);
        gp2x_memregs=(unsigned short *)gp2x_memregl;

  if(first) { printf(MINILIB_VERSION "\n");
              gp2x_dualcore_registers(1);
              gp2x_sound_volume(100,100);
              gp2x_memregs[0x0F16>>1] = 0x830a; usleep(100000);
              gp2x_memregs[0x0F58>>1] = 0x100c; usleep(100000); }

  gp2x_memregs[0x28DA>>1]=(((bpp+1)/8)<<9)|0xAB; /*8/15/16/24bpp...*/
  gp2x_memregs[0x290C>>1]=320*((bpp+1)/8);       /*line width in bytes*/

  memset(gp2x_screen15, 0, 320*240*2); gp2x_video_flip();
  memset(gp2x_screen15, 0, 320*240*2); gp2x_video_flip();

  //if(bpp==8)  gp2x_physvram[2]+=320*240,    gp2x_physvram[3]+=320*240,
  //           gp2x_logvram15[2]+=320*240/2, gp2x_logvram15[3]+=320*240/2;
  if(bpp==8)  gp2x_physvram[2]+=0x20000,   gp2x_physvram[3]+=0x20000,
             gp2x_logvram15[2]+=0x20000/2, gp2x_logvram15[3]+=0x20000/2;


 if ( gp2x_do_sound)
 {
    ioctl(gp2x_dev[3], SNDCTL_DSP_SPEED,  &rate);
    ioctl(gp2x_dev[3], SNDCTL_DSP_SETFMT, &bits);

    ioctl(gp2x_dev[3], SNDCTL_DSP_CHANNELS, &channels);
    ioctl(gp2x_dev[3], SNDCTL_DSP_STEREO, &stereoLocal);

    frag = 0x40000|13;
    ioctl(gp2x_dev[3], SNDCTL_DSP_SETFRAGMENT, &frag);


    printf("minimal() do sound, rate %d, bits %d, stereo %d, frag %d\n", rate, bits, stereo, frag);

    if(first)
    {
      first=0;
    }
  }
}


extern int fcloseall(void);
void gp2x_deinit(void)
{
  while((gp2x_sound++)<1000000);                               //wait arm920t threads to finish

  gp2x_dualcore_registers(0);
  mmuunhack();

  gp2x_memregs[0x28DA>>1]=0x4AB;                               //set video mode
  gp2x_memregs[0x290C>>1]=640;

  { int i; for(i=0;i<8;i++) if(gp2x_dev[i]) close(gp2x_dev[i]); }    //close all devices

  fcloseall();                                                   //close all files

}


void SetVideoScaling(int pixels,int width,int height)
{
        float x, y;
        float xscale,yscale=0;

        int bpp=(gp2x_memregs[0x28DA>>1]>>9)&0x3;  /* bytes per pixel */

	scaling_enabled = (width == 320) ? 0 : 1;

        if(gp2x_memregs[0x2800>>1]&0x100) /* TV-Out ON? */
        {
                xscale=489.0; /* X-Scale with TV-Out (PAL or NTSC) */
                if (gp2x_memregs[0x2818>>1]  == 287) /* PAL? */
                        yscale=(pixels*274.0)/320.0; /* Y-Scale with PAL */
                else if (gp2x_memregs[0x2818>>1]  == 239) /* NTSC? */
                        yscale=(pixels*331.0)/320.0; /* Y-Scale with NTSC */
        }
        else /* TV-Out OFF? */
        {
                xscale=1024.0; /* X-Scale for LCD */
                yscale=pixels; /* Y-Scale for LCD */
        }

        x=(xscale*width)/320.0;
        y=(yscale*bpp*height)/240.0;
        gp2x_memregs[0x2906>>1]=(unsigned short)x; /* scale horizontal */
        gp2x_memregl[0x2908>>2]=(unsigned  long)y; /* scale vertical */
        gp2x_memregs[0x290C>>1]=pixels*bpp; /* Set Video Width */
}
