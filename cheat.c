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
#include "x6502.h"
#include "cheat.h"
#include "fce.h"
#include "svga.h"
#include "general.h"
#include "cart.h"
#include "memory.h"

static uint8 *CheatRPtrs[64];

void FCEU_CheatResetRAM(void)
{
 int x;

 for(x=0;x<64;x++) 
  CheatRPtrs[x]=0;
}

void FCEU_CheatAddRAM(int s, uint32 A, uint8 *p)
{
 uint32 AB=A>>10;
 int x;

 for(x=s-1;x>=0;x--)
  CheatRPtrs[AB+x]=p-A;
}


struct CHEATF {
           struct CHEATF *next;
           char *name;
           uint16 addr;
           uint8 val;
	   int status;
};

struct CHEATF *cheats=0,*cheatsl=0;


#define CHEATC_NONE     0x8000
#define CHEATC_EXCLUDED 0x4000
#define CHEATC_NOSHOW   0xC000

static uint16 *CheatComp=0;
static int savecheats;

static int AddCheatEntry(char *name, uint32 addr, uint8 val, int status);

static void CheatMemErr(void)
{
 FCEUD_PrintError("Error allocating memory for cheat data.");
}

/* This function doesn't allocate any memory for "name" */
static int AddCheatEntry(char *name, uint32 addr, uint8 val, int status)
{
 struct CHEATF *temp;
 if(!(temp=malloc(sizeof(struct CHEATF))))
 {
  CheatMemErr();
  return(0);
 }
 temp->name=name;
 temp->addr=addr;
 temp->val=val;
 temp->status=status;

 temp->next=0;

 if(cheats)
 {
  cheatsl->next=temp;
  cheatsl=temp;
 }
 else
  cheats=cheatsl=temp;

 return(1);
}

void LoadGameCheats(void)
{
 FILE *fp;
 char *name=0;
 unsigned int addr;
 unsigned int val;
 unsigned int status;
 int x;

 char linebuf[256+4+2+2+1+1]; /* 256 for name, 4 for address, 2 for value, 2 for semicolons, 1 for status, 1 for null */
 char namebuf[257];
 int tc=0;

 savecheats=0;
 if(!(fp=fopen(FCEU_MakeFName(FCEUMKF_CHEAT,0,0),"rb")))
  return;
 
 while(fgets(linebuf,256+4+2+2+1+1,fp)>0)
 { 
  addr=val=status=0;
  namebuf[0]=0; // If the cheat doesn't have a name...
  if(linebuf[0]==':')  
  {
   strncpy(namebuf,&linebuf[1+4+2+2],257);
   if(sscanf(&linebuf[1],"%04x%*[:]%02x",&addr,&val)!=2)
    continue;
   status=0;
  }
  else
  {
   strncpy(namebuf,&linebuf[4+2+2],257);
   if(sscanf(linebuf,"%04x%*[:]%02x",&addr,&val)!=2) continue;
   status=1;
  }
  for(x=0;x<257;x++)
  {
   if(namebuf[x]==10 || namebuf[x]==13)
   {
    namebuf[x]=0;
    break;
   }
  }
  namebuf[256]=0;
  if(!(name=malloc(strlen(namebuf)+1)))
   CheatMemErr();
  else
  {
   strcpy(name,namebuf);
   AddCheatEntry(name,addr,val,status);
   tc++;
  }
 }
 fclose(fp);
}


void FlushGameCheats(void)
{
 if(CheatComp)
 {
  free(CheatComp);
  CheatComp=0;
 }

 if(!savecheats)
 {
  if(cheats)
  {
   struct CHEATF *next=cheats;
   for(;;)
   {  
    next=next->next;
    free(cheats->name);
    free(cheats);
    if(!next) break;
   }
   cheats=cheatsl=0;
  }
 }
 else
 {
  if(cheats)
  {
   struct CHEATF *next=cheats;
   FILE *fp;
   if((fp=fopen(FCEU_MakeFName(FCEUMKF_CHEAT,0,0),"wb")))
   {
    for(;;)
    {
     struct CHEATF *t;
     if(next->status)
      fprintf(fp,"%04x:%02x:%s\n",next->addr,next->val,next->name);
     else
      fprintf(fp,":%04x:%02x:%s\n",next->addr,next->val,next->name);
     free(next->name);
     t=next;
     next=next->next;
     free(t);
     if(!next) break;
    }
    fclose(fp);
   }
   else
    FCEUD_PrintError("Error saving cheats.");
   cheats=cheatsl=0;
  }
  else
   remove(FCEU_MakeFName(FCEUMKF_CHEAT,0,0));
 }
}


int FCEUI_AddCheat(char *name, uint32 addr, uint8 val)
{
 char *t;

 if(!(t=malloc(strlen(name)+1)))
 {
  CheatMemErr();
  return(0);
 }
 strcpy(t,name);
 if(!AddCheatEntry(t,addr,val,1))
 {
  free(t);
  return(0);
 }
 savecheats=1;
 return(1);
}

int FCEUI_DelCheat(uint32 which)
{
 struct CHEATF *prev;
 struct CHEATF *cur;
 uint32 x=0;

 for(prev=0,cur=cheats;;)
 {
  if(x==which)          // Remove this cheat.
  {
   if(prev)             // Update pointer to this cheat.
   {
    if(cur->next)       // More cheats.
     prev->next=cur->next;
    else                // No more.
    {
     prev->next=0;
     cheatsl=prev;      // Set the previous cheat as the last cheat.
    }
   }
   else                 // This is the first cheat.
   {
    if(cur->next)       // More cheats
     cheats=cur->next;
    else
     cheats=cheatsl=0;  // No (more) cheats.
   }
   free(cur->name);     // Now that all references to this cheat are removed,
   free(cur);           // free the memory.   
   break;
  }                     // *END REMOVE THIS CHEAT*


  if(!cur->next)        // No more cheats to go through(this shouldn't ever happen...)
   return(0);
  prev=cur;
  cur=prev->next;
  x++;
 }

 savecheats=1;
 return(1);
}

void ApplyPeriodicCheats(void)
{
 struct CHEATF *cur=cheats;
 if(!cur) return;

 for(;;)
 {
  if(cur->status)
   if(CheatRPtrs[cur->addr>>10])
    CheatRPtrs[cur->addr>>10][cur->addr]=cur->val;
  if(cur->next)
   cur=cur->next;
  else
   break;
 }
}


void FCEUI_ListCheats(int (*callb)(char *name, uint32 a, uint8 v, int s))
{
  struct CHEATF *next=cheats;

  while(next)
  {
   if(!callb(next->name,next->addr,next->val,next->status)) break;
   next=next->next;
  }
}

int FCEUI_GetCheat(uint32 which, char **name, uint32 *a, uint8 *v, int *s)
{
 struct CHEATF *next=cheats;
 uint32 x=0;

 while(next)
 {
  if(x==which)
  {
   if(name)
    *name=next->name;
   if(a)
    *a=next->addr; 
   if(v)
    *v=next->val;
   if(s)
    *s=next->status;

   return(1);
  }
  next=next->next;
  x++;
 }
 return(0);
}

/* name can be NULL if the name isn't going to be changed. */
/* same goes for a, v, and s(except the values of each one must be <0) */

int FCEUI_SetCheat(uint32 which, char *name, int32 a, int32 v, int s)
{
 struct CHEATF *next=cheats;
 uint32 x=0;

 while(next)
 {
  if(x==which)
  {
   if(name)
   {
    char *t;

    if((t=realloc(next->name,strlen(name+1))))
    {
     next->name=t;
     strcpy(next->name,name);
    }
    else
     return(0);
   }
   if(a>=0)
    next->addr=a;
   if(v>=0)
    next->val=v;
   if(s>=0)
    next->status=s;
   savecheats=1;
   return(1);
  }
  next=next->next;
  x++;
 }
 return(0);
}


static int InitCheatComp(void)
{
 uint32 x;

 CheatComp=malloc(65536*sizeof(uint16));
 if(!CheatComp)
 {
  CheatMemErr();
  return(0);
 }
 for(x=0;x<65536;x++)
  CheatComp[x]=CHEATC_NONE;
 
 return(1);
}

void FCEUI_CheatSearchSetCurrentAsOriginal(void)
{
 uint32 x;
 for(x=0x000;x<0x10000;x++)
  if(!(CheatComp[x]&CHEATC_NOSHOW))
  {
   if(CheatRPtrs[x>>10])
    CheatComp[x]=CheatRPtrs[x>>10][x];
   else
    CheatComp[x]|=CHEATC_NONE;
  }
}

void FCEUI_CheatSearchShowExcluded(void)
{
 uint32 x;

 for(x=0x000;x<0x10000;x++)
  CheatComp[x]&=~CHEATC_EXCLUDED;
}


int32 FCEUI_CheatSearchGetCount(void)
{
 uint32 x,c=0;

 if(CheatComp)
 {
  for(x=0x0000;x<0x10000;x++)
   if(!(CheatComp[x]&CHEATC_NOSHOW) && CheatRPtrs[x>>10])
    c++;
 }

 return c;
}
/* This function will give the initial value of the search and the current value at a location. */

void FCEUI_CheatSearchGet(int (*callb)(uint32 a, uint8 last, uint8 current))
{
  uint32 x;

  if(!CheatComp)
  {
   if(!InitCheatComp())
    CheatMemErr();
   return;
  }

  for(x=0;x<0x10000;x++)
   if(!(CheatComp[x]&CHEATC_NOSHOW) && CheatRPtrs[x>>10])
    if(!callb(x,CheatComp[x],CheatRPtrs[x>>10][x]))
     break;
}

void FCEUI_CheatSearchGetRange(uint32 first, uint32 last, int (*callb)(uint32 a, uint8 last, uint8 current))
{
  uint32 x;
  uint32 in=0;

  if(!CheatComp)
  {
   if(!InitCheatComp())
    CheatMemErr();
   return;
  }

  for(x=0;x<0x10000;x++)
   if(!(CheatComp[x]&CHEATC_NOSHOW) && CheatRPtrs[x>>10])
   {
    if(in>=first)
     if(!callb(x,CheatComp[x],CheatRPtrs[x>>10][x]))
      break;
    in++;
    if(in>last) return;
   }
}

void FCEUI_CheatSearchBegin(void)
{
 uint32 x;

 if(!CheatComp)
 {
  if(!InitCheatComp())
  {
   CheatMemErr();
   return;
  }
 }
 for(x=0;x<0x10000;x++)
 {
  if(CheatRPtrs[x>>10])
   CheatComp[x]=CheatRPtrs[x>>10][x];
  else
   CheatComp[x]=CHEATC_NONE;
 }
}


static int INLINE CAbs(int x)
{
 if(x<0)
  return(0-x);
 return x;
}

void FCEUI_CheatSearchEnd(int type, uint8 v1, uint8 v2)
{
 uint32 x;

 if(!CheatComp)
 {
  if(!InitCheatComp())
  {
   CheatMemErr();
   return;
  }
 }


 if(!type)      // Change to a specific value.
 {
  for(x=0;x<0x10000;x++)
   if(!(CheatComp[x]&CHEATC_NOSHOW))
   {
    if(CheatComp[x]==v1 && CheatRPtrs[x>>10][x]==v2)
    {

    }
    else
     CheatComp[x]|=CHEATC_EXCLUDED;
   }
 }
 else if(type==1)           // Search for relative change(between values).
 {
  for(x=0;x<0x10000;x++)
   if(!(CheatComp[x]&CHEATC_NOSHOW))
   {
    if(CheatComp[x]==v1 && CAbs(CheatComp[x]-CheatRPtrs[x>>10][x])==v2)
    {

    }
    else
     CheatComp[x]|=CHEATC_EXCLUDED;
   }
 }
 else if(type==2)                          // Purely relative change.
 {
  for(x=0x000;x<0x10000;x++)
   if(!(CheatComp[x]&CHEATC_NOSHOW))
   {
    if(CAbs(CheatComp[x]-CheatRPtrs[x>>10][x])==v2)
    {

    }
    else
     CheatComp[x]|=CHEATC_EXCLUDED;
   }
 }
 else if(type==3)                          // Any change.
 {
  for(x=0x000;x<0x10000;x++)
   if(!(CheatComp[x]&CHEATC_NOSHOW))
   {
    if(CheatComp[x]!=CheatRPtrs[x>>10][x])
    {

    }
    else
     CheatComp[x]|=CHEATC_EXCLUDED;
   }

 }
}
