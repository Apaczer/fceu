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
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/joystick.h>
#include "main.h"
#include "lnx-joystick.h"

int joy[4]={0,0,0,0};
int joyBMap[4][4];

static int32 joybuttons[4]={0,0,0,0};
static int32 joyx[4]={0,0,0,0};
static int32 joyy[4]={0,0,0,0};

static void ConfigJoystick(int z);
static int fd[4];

#define JOY_A   1
#define JOY_B   2
#define JOY_SELECT      4
#define JOY_START       8
#define JOY_UP  0x10
#define JOY_DOWN        0x20
#define JOY_LEFT        0x40
#define JOY_RIGHT       0x80

static void UpdateJoyData(int x)
{
	struct js_event e;

	while(read(fd[x],&e,sizeof(struct js_event))==sizeof(struct js_event))
	{
	 e.type&=~JS_EVENT_INIT;
         if(e.type==JS_EVENT_BUTTON) 
 	 {
          if(e.value)
           joybuttons[x]|=(1<<e.number);
          else
           joybuttons[x]&=~(1<<e.number);
         }
	 else if(e.type==JS_EVENT_AXIS)
	 {
	  if(e.number==0)
	   joyx[x]=e.value;
	  else if(e.number==1)
	   joyy[x]=e.value;
	 }
	}
}

uint32 GetJSOr(void)
{
	int x,y;
	unsigned long ret;
	ret=0;

	for(x=0;x<4;x++)
	{
	 int *joym=joyBMap[x];

	 if(!joy[x]) continue;
	 UpdateJoyData(x);
	 for(y=0;y<4;y++)
	  if(joybuttons[x]&joym[y]) ret|=(1<<y)<<(x<<3);
	 if(joyx[x]<=-16383) ret|=JOY_LEFT<<(x<<3);
	 else if(joyx[x]>=16383) ret|=JOY_RIGHT<<(x<<3);
	 if(joyy[x]<=-16383) ret|=JOY_UP<<(x<<3);
	 else if(joyy[x]>=16383) ret|=JOY_DOWN<<(x<<3);
        }
	return ret;
}

void KillJoysticks(void)
{
	int x;
	for(x=0;x<4;x++)
	 if(joy[x]) 
	  close(fd[x]);
}

int InitJoysticks(void)
{
	char dbuf[64];
	int version;
	int z;

	for(z=0;z<4;z++)
	{
	 if(!joy[z]) continue;
	 sprintf(dbuf,"/dev/js%d",joy[z]-1);
 	 if((fd[z]=open(dbuf,O_RDONLY|O_NONBLOCK))<0) 
	 {
          printf("Could not open %s.\n",dbuf);
          joy[z]=0;
          continue;
         }

         if(ioctl(fd[z], JSIOCGVERSION, &version)==-1) 
	 {
	  printf("Error using ioctl JSIOCGVERSION on %s.\n",dbuf);
	  joy[z]=0;
	  close(fd[z]);
	  continue;
	 }

         if(!(joyBMap[z][0]|joyBMap[z][1]|joyBMap[z][2]|joyBMap[z][3]))
          ConfigJoystick(z);
        }

	return(joy[0]|joy[1]|joy[2]|joy[3]);
}

#define WNOINPUT(); for(;;){uint8 t; if(read(fileno(stdin),&t,1)==-1) \
                          {break;}}

static void BConfig(int z,int b)
{
  WNOINPUT();
  for(;;)
  {
   uint8 t;
   if(read(fileno(stdin),&t,1)==-1)
   {
    if(errno!=EAGAIN) break;
   }
   else
    break;

   {
    struct js_event e;

    while(read(fd[z],&e,sizeof(struct js_event))==sizeof(struct js_event))
    {
      if(e.type==JS_EVENT_BUTTON)
      {
       if(!e.value)
       {
	joyBMap[z][b]=1<<e.number;
	goto endsa;
       }
      }
    }
   }

  }
  endsa:
  WNOINPUT();
}

void ConfigJoystick(int z)
{
 int sa;
 static char *genb="** Press button for ";

 printf("\n\n Joystick button configuration:\n\n");
 printf("   Push the button to map to the virtual joystick.\n");
 printf("   If you do not wish to assign a button, press Enter to skip\n");
 printf("   that button.\n\n");
 printf("   Press enter to continue...\n");
 getchar();
 printf("****  Configuring joystick %d ****\n\n",z+1);

 sa=fcntl(fileno(stdin),F_GETFL);
 fcntl(fileno(stdin),F_SETFL,O_NONBLOCK);

  printf("%s\"Select\".\n",genb);
  BConfig(z,2);

  printf("%s\"Start\".\n",genb);
  BConfig(z,3);

  printf("%s\"B\".\n",genb);
  BConfig(z,1);

  printf("%s\"A\".\n",genb);
  BConfig(z,0);

  fcntl(fileno(stdin),F_SETFL,sa);
}

