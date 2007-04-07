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

static int16 MoBuffer[2048];
static int wsize;
void WriteWaveData(int32 *Buffer, int Count)
{
  int P;

  for(P=0;P<Count;P++)
   MoBuffer[P]=Buffer[P];
  wsize+=fwrite(MoBuffer,1,Count<<1,soundlog);
}

static int CloseWave(void)
{
 int s;

 if(!soundlog) return 0;
 s=ftell(soundlog)-8;
 fseek(soundlog,4,SEEK_SET);
 fputc(s&0xFF,soundlog);
 fputc((s>>8)&0xFF,soundlog);
 fputc((s>>16)&0xFF,soundlog);
 fputc((s>>24)&0xFF,soundlog);

 fseek(soundlog,0x28,SEEK_SET);
 s=wsize;
 fputc(s&0xFF,soundlog);
 fputc((s>>8)&0xFF,soundlog);
 fputc((s>>16)&0xFF,soundlog);
 fputc((s>>24)&0xFF,soundlog);

 fclose(soundlog);
 soundlog=0;
 return 1;
}
int WriteWaveHeader(FILE *fp)
{
 int r;
 fputs("RIFF",fp);
 fseek(fp,4,SEEK_CUR);  // Skip size
 fputs("WAVEfmt ",fp);
 fputc(0x10,fp);
 fputc(0,fp);
 fputc(0,fp);
 fputc(0,fp);

 fputc(1,fp);           // PCM
 fputc(0,fp);
 fputc(1,fp);           // Monophonic
 fputc(0,fp);

 r=44100;
 fputc(r&0xFF,fp);
 fputc((r>>8)&0xFF,fp);
 fputc((r>>16)&0xFF,fp);
 fputc((r>>24)&0xFF,fp);
 r<<=1;
 fputc(r&0xFF,fp);
 fputc((r>>8)&0xFF,fp);
 fputc((r>>16)&0xFF,fp);
 fputc((r>>24)&0xFF,fp);
 fputc(2,fp);
 fputc(0,fp);
 fputc(16,fp);
 fputc(0,fp);

 fputs("data",fp);
 fseek(fp,4,SEEK_CUR);

 return 1;
}

int StartSoundLog(char *str)
{
  wsize=0;
  soundlog=fopen(str,"wb");
  if(soundlog)
   return WriteWaveHeader(soundlog);
  else
   return 0;
}
int CreateSoundSave(void)
{
 const char filter[]="MS WAVE(*.wav)\0*.wav\0";
 char nameo[2048];
 OPENFILENAME ofn;

 if(soundlog)
  {
   CloseWave();
   return 0;
  }

 memset(&ofn,0,sizeof(ofn));
 ofn.lStructSize=sizeof(ofn);
 ofn.hInstance=fceu_hInstance;
 ofn.lpstrTitle="Log Sound As...";
 ofn.lpstrFilter=filter;
 nameo[0]=0;
 ofn.lpstrFile=nameo;
 ofn.nMaxFile=256;
 ofn.Flags=OFN_EXPLORER|OFN_FILEMUSTEXIST|OFN_HIDEREADONLY;
 if(GetSaveFileName(&ofn))
  return StartSoundLog(nameo);
 return 0;
}
