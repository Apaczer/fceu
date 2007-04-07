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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "types.h"

#include "general.h"
#include "state.h"
#include "version.h"
#include "svga.h"
#include "driver.h"

char *marray[1]={"Error allocating memory!"};

static char BaseDirectory[2048];
static char FileBase[2048];
static char FileExt[2048];
static char FileBaseDirectory[2048];

void FCEUI_SetBaseDirectory(char *dir)
{
 strncpy(BaseDirectory,dir,2047);
 BaseDirectory[2047]=0;
}

static char *odirs[FCEUIOD__COUNT]={0,0,0,0,0};     // odirs, odors. ^_^

void FCEUI_SetDirOverride(int which, char *n)
{
 odirs[which]=n;
 if(which==FCEUIOD_STATE)
  SaveStateRefresh();
}

/* We should probably use snprintf(), but many C libraries don't seem to
   have this function.
*/
char *FCEU_MakeFName(int type, int id1, char *cd1)
{
 static uint8 ret[2048];

 ret[0]=0;
 switch(type)
 {
  case FCEUMKF_STATE:if(odirs[FCEUIOD_STATE])
                      sprintf(ret,"%s"PSS"%s.fc%d",odirs[FCEUIOD_STATE],FileBase,id1);
                     else
                      sprintf(ret,"%s"PSS"fcs"PSS"%s.fc%d",BaseDirectory,FileBase,id1);
                     break;
  case FCEUMKF_SNAP:
		    if(FSettings.SnapName)
		    {
                     if(odirs[FCEUIOD_SNAPS])
                      sprintf(ret,"%s"PSS"%s-%d.%s",odirs[FCEUIOD_SNAPS],FileBase,id1,cd1);
                     else
                      sprintf(ret,"%s"PSS"snaps"PSS"%s-%d.%s",BaseDirectory,FileBase,id1,cd1);
		    }
		    else
		    {
		     if(odirs[FCEUIOD_SNAPS])
                      sprintf(ret,"%s"PSS"%d.%s",odirs[FCEUIOD_SNAPS],id1,cd1);
                     else
                      sprintf(ret,"%s"PSS"snaps"PSS"%d.%s",BaseDirectory,id1,cd1);
		    }
                    break;
  case FCEUMKF_SAV:if(odirs[FCEUIOD_NV])
                   {
                    sprintf(ret,"%s"PSS"%s.%s",odirs[FCEUIOD_NV],FileBase,cd1);
                   }
	           else
                   {
                    if(FSettings.SUnderBase)
                     sprintf(ret,"%s"PSS"sav"PSS"%s.%s",BaseDirectory,FileBase,cd1);
                    else
                     sprintf(ret,"%s"PSS"%s.%s",FileBaseDirectory,FileBase,cd1);
                   }
                   break;
  case FCEUMKF_CHEAT:
                     if(odirs[FCEUIOD_CHEATS])
                      sprintf(ret,"%s"PSS"%s.cht",odirs[FCEUIOD_CHEATS],FileBase);
                     else
                      sprintf(ret,"%s"PSS"cheats"PSS"%s.cht",BaseDirectory,FileBase);
                     break;
  case FCEUMKF_GGROM:sprintf(ret,"%s"PSS"gg.rom",BaseDirectory);break;
  case FCEUMKF_FDSROM:sprintf(ret,"%s"PSS"disksys.rom",BaseDirectory);break;
  case FCEUMKF_PALETTE:
                       if(odirs[FCEUIOD_MISC])
                        sprintf(ret,"%s"PSS"%s.pal",odirs[FCEUIOD_MISC],FileBase);
                       else
                        sprintf(ret,"%s"PSS"gameinfo"PSS"%s.pal",BaseDirectory,FileBase);
                       break;
 }
 return(ret);
}

void GetFileBase(char *f)
{
        char *tp1,*tp3;

 #if PSS_STYLE==4
     tp1=((char *)strrchr(f,':'));
 #elif PSS_STYLE==1
     tp1=((char *)strrchr(f,'/'));
 #else
     tp1=((char *)strrchr(f,'\\'));
  #if PSS_STYLE!=3
     tp3=((char *)strrchr(f,'/'));
     if(tp1<tp3) tp1=tp3;
  #endif
 #endif
     if(!tp1)
     {
      tp1=f;
      strcpy(FileBaseDirectory,".");
     }
     else
     {
      memcpy(FileBaseDirectory,f,tp1-f);
      FileBaseDirectory[tp1-f]=0;
      tp1++;
     }

     if((tp3=strrchr(f,'.'))!=NULL)
     {
      memcpy(FileBase,tp1,tp3-tp1);
      FileBase[tp3-tp1]=0;
      strcpy(FileExt,tp3+1);
     }
     else
     {
      strcpy(FileBase,tp1);
      FileExt[0]=0;
     }
}

uint32 uppow2(uint32 n)
{
 int x;

 for(x=31;x>=0;x--)
  if(n&(1<<x))
  {
   if((1<<x)!=n)
    return(1<<(x+1));
   break;
  }
 return n;
}

