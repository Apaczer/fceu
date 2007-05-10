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
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdio.h>
#include <string.h>
#include <sys/time.h>

#include "../../video.h"

#include "main.h"
#include "gp2x.h"
#include "minimal.h"
#include "fonts.h"

extern int showfps;

static char fps_str[32];
static int framesEmulated, framesRendered;

int gp2x_palette[256];

int scaled_display=0;
int paletterefresh;

#define FPS_COLOR 1


static void gp2x_text(unsigned char *screen, int x, int y, char *text, int color, int flip)
{
	int i,l,slen;
        slen=strlen(text);

	screen=screen+x+y*320;

	for (i=0;i<slen;i++)
        {
		for (l=0;l<8;l++)
                {

			screen[l*320+0]=(fontdata8x8[((text[i])*8)+l]&0x80)?color:screen[l*320+0];
			screen[l*320+1]=(fontdata8x8[((text[i])*8)+l]&0x40)?color:screen[l*320+1];
			screen[l*320+2]=(fontdata8x8[((text[i])*8)+l]&0x20)?color:screen[l*320+2];
			screen[l*320+3]=(fontdata8x8[((text[i])*8)+l]&0x10)?color:screen[l*320+3];
			screen[l*320+4]=(fontdata8x8[((text[i])*8)+l]&0x08)?color:screen[l*320+4];
			screen[l*320+5]=(fontdata8x8[((text[i])*8)+l]&0x04)?color:screen[l*320+5];
			screen[l*320+6]=(fontdata8x8[((text[i])*8)+l]&0x02)?color:screen[l*320+6];
			screen[l*320+7]=(fontdata8x8[((text[i])*8)+l]&0x01)?color:screen[l*320+7];

		}
		screen+=8;
	}
}


void CleanSurface(void)
{
	int c=4;
	while (c--)
	{
		memset(gp2x_screen, 0, 320*240);
		gp2x_video_flip();
	}
	XBuf = gp2x_screen;
}


void KillVideo(void)
{
}


int InitVideo(void)
{
	fps_str[0]=0;

	CleanSurface();

	puts("Initialized GP2X VIDEO via minimal");

	srendline=0;
	erendline=239;
	XBuf = gp2x_screen;
	return 1;
}


void ToggleFS(void)
{
}


void FCEUD_SetPalette(uint8 index, uint8 r, uint8 g, uint8 b)
{
	gp2x_palette[index] = (r << 16) | (g << 8) | b;
	gp2x_video_setpalette(gp2x_palette, index + 1);

	paletterefresh = 1;
}


void FCEUD_GetPalette(uint8 index, uint8 * r, uint8 * g, uint8 * b)
{
	int pix = gp2x_palette[index];
	*r = (uint8) (pix >> 16);
	*g = (uint8) (pix >> 8);
	*b = (uint8)  pix;
}


static INLINE void printFps(uint8 *screen)
{
	struct timeval tv_now;
	static int prevsec, needfpsflip = 0;

	gettimeofday(&tv_now, 0);
	if (prevsec != tv_now.tv_sec)
	{
		sprintf(fps_str, "%2i/%2i", framesRendered, framesEmulated);
		framesEmulated = framesRendered = 0;
		needfpsflip = 4;
		prevsec = tv_now.tv_sec;
	}

	if (!scaled_display)
	{
		if (needfpsflip)
		{
			int y, *destt = (int *) screen;
			for (y = 20/*240*/; y; y--)
			{
				*destt++ = 0; *destt++ = 0; *destt++ = 0; *destt++ = 0;
				*destt++ = 0; *destt++ = 0; *destt++ = 0; *destt++ = 0;
				destt += 64+8;

				//*destt++ = 0x3F3F3F3F; *destt++ = 0x3F3F3F3F; *destt++ = 0x3F3F3F3F; *destt++ = 0x3F3F3F3F;
				//*destt++ = 0x3F3F3F3F; *destt++ = 0x3F3F3F3F; *destt++ = 0x3F3F3F3F; *destt++ = 0x3F3F3F3F;
			}
			if (showfps)
			{
				int sep;
				for (sep=1; sep < 5; sep++)
					if (fps_str[sep] == '/' || fps_str[sep] == 0) break;
				fps_str[sep] = 0;
				gp2x_text(screen, 0,  0, fps_str,       FPS_COLOR, 0);
				gp2x_text(screen, 0, 10, fps_str+sep+1, FPS_COLOR, 0);
			}
			needfpsflip--;
		}
	}
	else
	{
		if (showfps)
		{
			gp2x_text(screen+32, 0, 0, fps_str, FPS_COLOR, 0);
		}
	}
}


void BlitScreen(uint8 *buf)
{
	framesEmulated++;

	if (!buf) return;

	framesRendered++;

	printFps(gp2x_screen);
	gp2x_video_flip();
	XBuf = gp2x_screen;
}


