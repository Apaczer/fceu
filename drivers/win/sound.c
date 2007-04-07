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

FILE *soundlog=0;
void WriteWaveData(int32 *Buffer, int Count);
DWORD WINAPI DSThread(LPVOID lpParam);
LPDIRECTSOUND ppDS=0;
LPDIRECTSOUNDBUFFER ppbuf=0;
LPDIRECTSOUNDBUFFER ppbufsec=0;
LPDIRECTSOUNDBUFFER ppbufw;

DSBUFFERDESC DSBufferDesc;
WAVEFORMATEX wfa;
WAVEFORMATEX wf;

static int DSBufferSize=0;
static int bittage;

void TrashSound(void)
{
 FCEUI_Sound(0);
 if(ppbufsec)
 {
  IDirectSoundBuffer_Stop(ppbufsec);
  IDirectSoundBuffer_Release(ppbufsec);
  ppbufsec=0;
 }
 if(ppbuf)
 {
  IDirectSoundBuffer_Stop(ppbuf);
  IDirectSoundBuffer_Release(ppbuf);
  ppbuf=0;
 }
 if(ppDS)
 {
  IDirectSound_Release(ppDS);
  ppDS=0;
 }
}


 static VOID *feegle[2];
 static DWORD dook[2];
 static DWORD writepos=0,playpos=0,lplaypos=0;
void CheckDStatus(void)
{
  DWORD status;
  status=0;
  IDirectSoundBuffer_GetStatus(ppbufw, &status);

  if(status&DSBSTATUS_BUFFERLOST)
  {
   IDirectSoundBuffer_Restore(ppbufw);
  }

  if(!(status&DSBSTATUS_PLAYING))
  {
   lplaypos=0;
   writepos=((soundbufsize)<<bittage);
   IDirectSoundBuffer_SetFormat(ppbufw,&wf);
   IDirectSoundBuffer_Play(ppbufw,0,0,DSBPLAY_LOOPING);
  }
}

static int16 MBuffer[2048];
void FCEUD_WriteSoundData(int32 *Buffer, int Count)
{
 int P;
 int k=0;

 if(soundlog)
  WriteWaveData(Buffer, Count);

 if(!bittage)
 {
  for(P=0;P<Count;P++)
   *(((uint8*)MBuffer)+P)=((int8)(Buffer[P]>>8))^128;
 }
 else
 {
  for(P=0;P<Count;P++)
   MBuffer[P]=Buffer[P];
 }
  ilicpo:
  CheckDStatus();
  IDirectSoundBuffer_GetCurrentPosition(ppbufw,&playpos,0);

  if(writepos>=DSBufferSize) 
   if(playpos<lplaypos)
    writepos-=DSBufferSize;
  lplaypos=playpos;

  /* If the write position is beyond the fill buffer, block. */
  if(writepos>=(playpos+(soundbufsize<<bittage)))
  //if(!(writepos<playpos+((soundbufsize)<<bittage)))
  {
   if(!NoWaiting)
   {
    if(soundsleep==1)
    {
     if(!k)
     {
      int stime;      

      stime=writepos-(playpos+(soundbufsize<<bittage));
      stime*=1000;
      stime/=soundrate;
      stime>>=1;
      if(stime>=5)
       Sleep(stime);
      k=1;
     }     
    }
    else if(soundsleep==2)
    {
     int stime;      
     stime=writepos-(playpos+(soundbufsize<<bittage));
     stime*=1000;
     stime/=soundrate;
     stime>>=1;
     if(stime>=2)
      Sleep(stime);
    }
   }
   BlockingCheck();
   if(!soundo || NoWaiting) return;
   goto ilicpo;
  }

  if(netplaytype && netplayon)
  {
   if(writepos<=playpos+128)
   writepos=playpos+(soundbufsize<<bittage);
  }

  {
   feegle[0]=feegle[1]=0;
   dook[0]=dook[1]=0;

   
   ddrval=IDirectSoundBuffer_Lock(ppbufw,(writepos%DSBufferSize),Count<<bittage,&feegle[0],&dook[0],&feegle[1],&dook[1],0);
   if(ddrval!=DS_OK)
    goto nolock;

   if(feegle[1]!=0 && feegle[1]!=feegle[0])
   {
    if(soundflush)
    {
     memset(feegle[0],0x80,dook[0]);
     memset(feegle[1],0x80,dook[1]);
    }
    else
    {
     memcpy(feegle[0],(uint8 *)MBuffer,dook[0]);
     memcpy(feegle[1],((uint8 *)MBuffer)+dook[0],dook[1]);
    }
   }
   else
   {
    if(soundflush)
     memset(feegle[0],0x80,dook[0]);
    else
     memcpy(feegle[0],(uint8 *)MBuffer,dook[0]);
   }

   IDirectSoundBuffer_Unlock(ppbufw,feegle[0],dook[0],feegle[1],dook[1]);
   writepos+=Count<<bittage;
  }
 nolock:
 ///////// Ending
}

int InitSound()
{
 DSCAPS dscaps;
 DSBCAPS dsbcaps;

 memset(&wf,0x00,sizeof(wf));
 wf.wFormatTag = WAVE_FORMAT_PCM;
 wf.nChannels = 1;
 wf.nSamplesPerSec = soundrate;

 ddrval=DirectSoundCreate(0,&ppDS,0);
 if (ddrval != DS_OK)
 {
  FCEUD_PrintError("DirectSound: Error creating DirectSound object.");
  return 0;
 }

 if(soundoptions&SO_SECONDARY)
 {
  trysecondary:
  ddrval=IDirectSound_SetCooperativeLevel(ppDS,hAppWnd,DSSCL_PRIORITY);
  if (ddrval != DS_OK)
  {
   FCEUD_PrintError("DirectSound: Error setting cooperative level to DDSCL_PRIORITY.");
   TrashSound();
   return 0;
  }
 }
 else
 {
  ddrval=IDirectSound_SetCooperativeLevel(ppDS,hAppWnd,DSSCL_WRITEPRIMARY);
  if (ddrval != DS_OK)
  {
   FCEUD_PrintError("DirectSound: Error setting cooperative level to DDSCL_WRITEPRIMARY.  Forcing use of secondary sound buffer and trying again...");
   soundoptions|=SO_SECONDARY;
   goto trysecondary;
  }
 }
 memset(&dscaps,0x00,sizeof(dscaps));
 dscaps.dwSize=sizeof(dscaps);
 ddrval=IDirectSound_GetCaps(ppDS,&dscaps);
 if(ddrval!=DS_OK)
 {
  FCEUD_PrintError("DirectSound: Error getting capabilities.");
  return 0;
 }

 if(dscaps.dwFlags&DSCAPS_EMULDRIVER)
  FCEUD_PrintError("DirectSound: Sound device is being emulated through waveform-audio functions.  Sound quality will most likely be awful.  Try to update your sound device's sound drivers.");

 IDirectSound_Compact(ppDS);

 memset(&DSBufferDesc,0x00,sizeof(DSBUFFERDESC));
 DSBufferDesc.dwSize=sizeof(DSBufferDesc);
 if(soundoptions&SO_SECONDARY)
  DSBufferDesc.dwFlags=DSBCAPS_PRIMARYBUFFER;
 else
  DSBufferDesc.dwFlags=DSBCAPS_PRIMARYBUFFER|DSBCAPS_GETCURRENTPOSITION2;

 ddrval=IDirectSound_CreateSoundBuffer(ppDS,&DSBufferDesc,&ppbuf,0);
 if (ddrval != DS_OK)
 {
  FCEUD_PrintError("DirectSound: Error creating primary buffer.");
  TrashSound();
  return 0;
 } 

 memset(&wfa,0x00,sizeof(wfa));

 if(soundoptions&SO_FORCE8BIT)
  bittage=0;
 else
 {
  bittage=1;
  if( (!(dscaps.dwFlags&DSCAPS_PRIMARY16BIT)) ||
      (!(dscaps.dwFlags&DSCAPS_SECONDARY16BIT) && (soundoptions&SO_SECONDARY)))
  {
   FCEUD_PrintError("DirectSound: 16-bit sound is not supported.  Forcing 8-bit sound.");
   bittage=0;
   soundoptions|=SO_FORCE8BIT;
  }
 }

 wf.wBitsPerSample=8<<bittage;
 wf.nBlockAlign = bittage+1;
 wf.nAvgBytesPerSec = wf.nSamplesPerSec * wf.nBlockAlign;
 
 ddrval=IDirectSoundBuffer_SetFormat(ppbuf,&wf);
 if (ddrval != DS_OK)
 {
  FCEUD_PrintError("DirectSound: Error setting primary buffer format.");
  TrashSound();
  return 0;
 }

 IDirectSoundBuffer_GetFormat(ppbuf,&wfa,sizeof(wfa),0);

 if(soundoptions&SO_SECONDARY)
 {
  memset(&DSBufferDesc,0x00,sizeof(DSBUFFERDESC));  
  DSBufferDesc.dwSize=sizeof(DSBufferDesc);
  DSBufferDesc.dwFlags=DSBCAPS_GETCURRENTPOSITION2;
  if(soundoptions&SO_GFOCUS)
   DSBufferDesc.dwFlags|=DSBCAPS_GLOBALFOCUS;
  DSBufferDesc.dwBufferBytes=32768;
  DSBufferDesc.lpwfxFormat=&wfa;  
  ddrval=IDirectSound_CreateSoundBuffer(ppDS, &DSBufferDesc, &ppbufsec, 0);
  if (ddrval != DS_OK)
  {
   FCEUD_PrintError("DirectSound: Error creating secondary buffer.");
   TrashSound();
   return 0;
  }
 }

 //sprintf(TempArray,"%d\n",wfa.nSamplesPerSec);
 //FCEUD_PrintError(TempArray);

 if(soundoptions&SO_SECONDARY)
 {
  DSBufferSize=32768;
  IDirectSoundBuffer_SetCurrentPosition(ppbufsec,0);
  ppbufw=ppbufsec;
 }
 else
 {
  memset(&dsbcaps,0,sizeof(dsbcaps));
  dsbcaps.dwSize=sizeof(dsbcaps);
  ddrval=IDirectSoundBuffer_GetCaps(ppbuf,&dsbcaps);
  if (ddrval != DS_OK)
  {
   FCEUD_PrintError("DirectSound: Error getting buffer capabilities.");
   TrashSound();
   return 0;
  }

  DSBufferSize=dsbcaps.dwBufferBytes;

  if(DSBufferSize<8192)
  {
   FCEUD_PrintError("DirectSound: Primary buffer size is too small!");
   TrashSound();
   return 0;
  }
  ppbufw=ppbuf;
 }

 soundbufsize=(soundbuftime*soundrate/1000);
 FCEUI_Sound(soundrate);
 return 1;
}

BOOL CALLBACK SoundConCallB(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  int x;

  switch(uMsg) {
   case WM_INITDIALOG:
                if(soundo)
                 CheckDlgButton(hwndDlg,126,BST_CHECKED);
                if(soundoptions&SO_FORCE8BIT)
                 CheckDlgButton(hwndDlg,122,BST_CHECKED);
                if(soundoptions&SO_SECONDARY)
                 CheckDlgButton(hwndDlg,123,BST_CHECKED);
                if(soundoptions&SO_GFOCUS)
                 CheckDlgButton(hwndDlg,124,BST_CHECKED);
                SetDlgItemInt(hwndDlg,200,soundrate,0);

                /* Volume Trackbar */
                SendDlgItemMessage(hwndDlg,500,TBM_SETRANGE,1,MAKELONG(0,200));
                SendDlgItemMessage(hwndDlg,500,TBM_SETTICFREQ,25,0);
                SendDlgItemMessage(hwndDlg,500,TBM_SETPOS,1,200-soundvolume);

                /* buffer size time trackbar */
                SendDlgItemMessage(hwndDlg,128,TBM_SETRANGE,1,MAKELONG(15,200));
                SendDlgItemMessage(hwndDlg,128,TBM_SETTICFREQ,1,0);
                SendDlgItemMessage(hwndDlg,128,TBM_SETPOS,1,soundbuftime);

                {
                 char tbuf[8];
                 sprintf(tbuf,"%d",soundbuftime);
                 SetDlgItemText(hwndDlg,666,(LPTSTR)tbuf);
                }
                
                SendDlgItemMessage(hwndDlg,129,CB_ADDSTRING,0,(LPARAM)(LPSTR)"Mean");
                SendDlgItemMessage(hwndDlg,129,CB_ADDSTRING,0,(LPARAM)(LPSTR)"Nice");
                SendDlgItemMessage(hwndDlg,129,CB_ADDSTRING,0,(LPARAM)(LPSTR)"Nicest");
                SendDlgItemMessage(hwndDlg,129,CB_SETCURSEL,soundsleep,(LPARAM)(LPSTR)0);
                break;
   case WM_HSCROLL:
                // This doesn't seem to work.  Hmm...
                //if((HWND)lParam==(HWND)128)
                {
                 char tbuf[8];
                 soundbuftime=SendDlgItemMessage(hwndDlg,128,TBM_GETPOS,0,0);
                 sprintf(tbuf,"%d",soundbuftime);
                 SetDlgItemText(hwndDlg,666,(LPTSTR)tbuf);
                }
                break;
   case WM_CLOSE:
   case WM_QUIT: goto gornk;
   case WM_COMMAND:
                if(!(wParam>>16))
                switch(wParam&0xFFFF)
                {
                 case 1:
                        gornk:
                        soundoptions=0;
                        if(IsDlgButtonChecked(hwndDlg,122)==BST_CHECKED)
                         soundoptions|=SO_FORCE8BIT;
                        if(IsDlgButtonChecked(hwndDlg,123)==BST_CHECKED)
                         soundoptions|=SO_SECONDARY;
                        if(IsDlgButtonChecked(hwndDlg,124)==BST_CHECKED)
                         soundoptions|=SO_GFOCUS;
                        if(IsDlgButtonChecked(hwndDlg,126)==BST_CHECKED)
                         soundo=1;
                        else
                         soundo=0;
                        x=GetDlgItemInt(hwndDlg,200,0,0);
                        if(x<8192 || x>65535)
                        {
                         FCEUD_PrintError("Sample rate is out of range(8192-65535).");
                         break;
                        }
                        else
                         soundrate=x;

                        soundvolume=200-SendDlgItemMessage(hwndDlg,500,TBM_GETPOS,0,0);
                        FCEUI_SetSoundVolume(soundvolume);
                        soundsleep=SendDlgItemMessage(hwndDlg,129,CB_GETCURSEL,0,(LPARAM)(LPSTR)0);
                        EndDialog(hwndDlg,0);
                        break;
               }
              }
  return 0;
}


void ConfigSound(void)
{
 int backo=soundo,sr=soundrate;
 int so=soundoptions;

 DialogBox(fceu_hInstance,"SOUNDCONFIG",hAppWnd,SoundConCallB);

 if(((backo?1:0)!=(soundo?1:0)))
 {
  if(!soundo)
   TrashSound();
  else
   soundo=InitSound();
 }
 else if(( soundoptions!=so || (sr!=soundrate)) && soundo)
 {
    TrashSound();
    soundo=InitSound();
 }
 soundbufsize=(soundbuftime*soundrate/1000);
}


void StopSound(void)
{
 if(soundo)
  IDirectSoundBuffer_Stop(ppbufw);
}

#include "wave.c"
