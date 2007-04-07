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
#include <ctype.h>
#include "../../driver.h"

static void GetString(char *s)
{
 int x;
 fgets(s,256,stdin);

 for(x=0;x<256;x++)
  if(s[x]=='\n')
  {
   s[x]=0;
   break;
  }
}

/* Get unsigned 16-bit integer from stdin in hex. */
static uint32 GetH16(unsigned int def)
{
 char buf[32];

 fgets(buf,32,stdin);
 if(buf[0]=='\n')
  return(def);
 if(buf[0]=='$')
  sscanf(buf+1,"%04x",&def);
 else
  sscanf(buf,"%04x",&def);
 return def;
}

/* Get unsigned 8-bit integer from stdin in decimal. */
static uint8 Get8(unsigned int def)
{
 char buf[32];

 fgets(buf,32,stdin);
 if(buf[0]=='\n')
  return(def);
 sscanf(buf,"%d",&def);
 return def;
}

static int GetYN(int def)
{
 char buf[32];
 printf("(Y/N)[%s]: ",def?"Y":"N");
 fgets(buf,32,stdin);
 if(buf[0]=='y' || buf[0]=='Y')
  return(1);
 if(buf[0]=='n' || buf[0]=='N')
  return(0);
 return(def);
}

/*
**	Begin list code.
**
*/
static int listcount;
static int listids[16];
static int listsel;
static int mordoe;

void BeginListShow(void)
{
 listcount=0;
 listsel=-1;
 mordoe=0;
}

/* Hmm =0 for in list choices, hmm=1 for end of list choices. */
/* Return equals 0 to continue, -1 to stop, otherwise a number. */
int ListChoice(int hmm)
{
  char buf[32];

  if(!hmm)
  {
   int num=0;

   tryagain:
   printf(" <'Enter' to continue, (S)top, or enter a number.> ");
   fgets(buf,32,stdin);
   if(buf[0]=='s' || buf[0]=='S') return(-1);
   if(buf[0]=='\n') return(0);
   if(!sscanf(buf,"%d",&num))
    return(0);  
   if(num<1) goto tryagain;
   return(num);
  }
  else
  {
   int num=0;

   tryagain2:
   printf(" <'Enter' to make no selection or enter a number.> ");
   fgets(buf,32,stdin);
   if(buf[0]=='\n') return(0);
   if(!sscanf(buf,"%d",&num))
    return(0);
   if(num<1) goto tryagain2;
   return(num);
  }
}

int EndListShow(void)
{
  if(mordoe)
  {
   int r=ListChoice(1);
   if(r>0 && r<=listcount)
    listsel=listids[r-1];
  }
  return(listsel);
}

/* Returns 0 to stop listing, 1 to continue. */
int AddToList(char *text, uint32 id)
{
 if(listcount==16)
 {
  int t=ListChoice(0);
  mordoe=0;
  if(t==-1) return(0);  // Stop listing.
  else if(t>0 && t<17)
  {
   listsel=listids[t-1];
   return(0);
  }
  listcount=0;
 }
 mordoe=1;
 listids[listcount]=id;
 printf("%2d) %s\n",listcount+1,text);
 listcount++; 
 return(1);
}

/*
**	
**	End list code.
**/

typedef struct MENU {
	char *text;
	void *action;
	int type;	// 0 for menu, 1 for function.
} MENU;

static void SetOC(void)
{
 FCEUI_CheatSearchSetCurrentAsOriginal();
}

static void UnhideEx(void)
{
 FCEUI_CheatSearchShowExcluded();
}

static void ToggleCheat(int num)
{
 uint32 A;
 int s;

 FCEUI_GetCheat(num,0,&A,0,&s);
 s^=1;
 FCEUI_SetCheat(num,0,-1,-1,s);
 printf("Cheat for address $%04x %sabled.\n",(unsigned int)A,s?"en":"dis");
}

static void ModifyCheat(int num)
{
 char *name;
 char buf[256];
 uint32 A;
 uint8 V;
 int s;
 int t;

 FCEUI_GetCheat(num, &name, &A, &V, &s);

 printf("Name [%s]: ",name);
 GetString(buf);

 /* This obviously doesn't allow for cheats with no names.  Bah.  Who wants
    nameless cheats anyway... 
 */

 if(buf[0])
  name=buf;	// Change name when FCEUI_SetCheat() is called.
 else
  name=0;	// Don't change name when FCEUI_SetCheat() is called.

 printf("Address [$%04x]: ",(unsigned int)A);
 A=GetH16(A);

 printf("Value [%03d]: ",(unsigned int)V);
 V=Get8(V);

 printf("Enable [%s]: ",s?"Y":"N");
 t=getchar();
 if(t=='Y' || t=='y') s=1;
 else if(t=='N' || t=='n') s=0;

 FCEUI_SetCheat(num,name,A,V,s);
}

static void AddCheatParam(uint32 A, uint8 V)
{
 char name[256];

 printf("Name: ");
 GetString(name);
 printf("Address [$%04x]: ",(unsigned int)A);
 A=GetH16(A);
 printf("Value [%03d]: ",(unsigned int)V);
 V=Get8(V);
 printf("Add cheat \"%s\" for address $%04x with value %03d?",name,(unsigned int)A,(unsigned int)V);
 if(GetYN(0))
 {
  if(FCEUI_AddCheat(name,A,V))
   puts("Cheat added.");
  else
   puts("Error adding cheat.");
 }
}

static void AddCheat(void)
{
 AddCheatParam(0,0);
}

static int lid;
static int clistcallb(char *name, uint32 a, uint8 v, int s)
{
 char tmp[512];
 int ret;

 sprintf(tmp,"%s $%04x:%03d - %s",s?"*":" ",(unsigned int)a,(unsigned int)v,name);
 ret=AddToList(tmp,lid);
 lid++;
 return(ret);
}

static void ListCheats(void)
{
 int which;
 lid=0;

 BeginListShow();
 FCEUI_ListCheats(clistcallb);
 which=EndListShow();
 if(which>=0)
 {
  char tmp[32];
  printf(" <(T)oggle status, (M)odify, or (D)elete this cheat.> ");
  fgets(tmp,32,stdin);
  switch(tolower(tmp[0]))
  {
   case 't':ToggleCheat(which);
	    break;
   case 'd':if(!FCEUI_DelCheat(which))
 	     puts("Error deleting cheat!");
	    else 
	     puts("Cheat has been deleted.");
	    break;
   case 'm':ModifyCheat(which);
	    break;
  }
 }
}

static void ResetSearch(void)
{
 FCEUI_CheatSearchBegin();
 puts("Done.");
}

static int srescallb(uint32 a, uint8 last, uint8 current)
{
 char tmp[13];
 sprintf(tmp, "$%04x:%03d:%03d",(unsigned int)a,(unsigned int)last,(unsigned int)current);
 return(AddToList(tmp,a));
}

static void ShowRes(void)
{
 int n=FCEUI_CheatSearchGetCount();
 printf(" %d results:\n",n);
 if(n)
 {
  int which;
  BeginListShow();
  FCEUI_CheatSearchGet(srescallb);
  which=EndListShow();
  if(which>=0)
   AddCheatParam(which,0);
 }
}

static int ShowShortList(char *moe[], int n, int def)
{
 int x,c;
 unsigned int baa;
 char tmp[16];

 red:
 for(x=0;x<n;x++)
  printf("%d) %s\n",x+1,moe[x]);
 puts("D) Display List");
 clo:

 printf("\nSelection [%d]> ",def+1);
 fgets(tmp,256,stdin);
 if(tmp[0]=='\n')
  return def;
 c=tolower(tmp[0]);
 baa=c-'1';

 if(baa<n)
  return baa;
 else if(c=='d')
  goto red;
 else
 {
  puts("Invalid selection.");
  goto clo;
 }
}

static void DoSearch(void)
{
 static int v1=0,v2=0;
 static int method=0;
 char *m[4]={"O==V1 && C==V2","O==V1 && |O-C|==V2","|O-C|==V2","O!=C"};
 printf("\nSearch Filter:\n");

 method=ShowShortList(m,4,method);
 if(method<=1)
 {
  printf("V1 [%03d]: ",v1);
  v1=Get8(v1);
 }
 if(method<=2)
 {
  printf("V2 [%03d]: ",v2);
  v2=Get8(v2);
 }
 FCEUI_CheatSearchEnd(method,v1,v2);
 puts("Search completed.\n");
}


static MENU NewCheatsMenu[7]={
 {"Add Cheat",AddCheat,1},
 {"Reset Search",ResetSearch,1},
 {"Do Search",DoSearch,1},
 {"Set Original to Current",SetOC,1},
 {"Unhide Excluded",UnhideEx,1},
 {"Show Results",ShowRes,1},
 {0}
};

static MENU MainMenu[3]={
 {"List Cheats",ListCheats,1},
 {"New Cheats...",NewCheatsMenu,0},
 {0}
};

static void DoMenu(MENU *men)
{
 int x=0;

 redisplay:
 x=0;
 puts("");
 while(men[x].text)
 {
  printf("%d) %s\n",x+1,men[x].text);
  x++;
 }
 puts("D) Display Menu\nX) Return to Previous\n");
 {
  char buf[32];
  int c;

  recommand:
  printf("Command> ");
  fgets(buf,32,stdin);
  c=tolower(buf[0]);
  if(c=='\n')
   goto recommand;
  else if(c=='d')
   goto redisplay;
  else if(c=='x')
  {
   return;
  }
  else if(sscanf(buf,"%d",&c))
  {
   if(c>x) goto invalid;
   if(men[c-1].type)
   {
    void (*func)(void)=men[c-1].action;
    func();
   }
   else
    DoMenu(men[c-1].action);	/* Mmm...recursivey goodness. */
   goto redisplay;
  }
  else
  {
   invalid:
   puts("Invalid command.\n");
   goto recommand;
  }

 }
}

void DoConsoleCheatConfig(void)
{
 MENU *curmenu=MainMenu;

 DoMenu(curmenu);
}
