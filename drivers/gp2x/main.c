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


#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <string.h>
#include <strings.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include "main.h"
#include "minimal.h"
#include "throttle.h"
#include "menu.h"
#include "gp2x.h"

#include "../common/config.h"
#include "../common/args.h"
#include "../common/unixdsp.h"
#include "../common/cheat.h"

#include "dface.h"

// just for printing some iNES info for user..
#include "../../fce.h"
#include "../../ines.h"

// internals
extern char lastLoadedGameName[2048];
extern uint8 Exit; // exit emu loop flag
void CloseGame(void);

FCEUGI *fceugi = NULL;
static int ntsccol=0,ntschue=-1,ntsctint=-1;
int soundvol=70;
int inited=0;

int srendlinev[2]={0,0};
int erendlinev[2]={239,239};
int srendline,erendline;


static char BaseDirectory[2048];

int eoptions=0;

static void DriverKill(void);
static int DriverInitialize(void);

static int gametype;
#include "input.c"

static void ParseGI(FCEUGI *gi)
{
 gametype=gi->type;

 InputType[0]=UsrInputType[0];
 InputType[1]=UsrInputType[1];
 InputTypeFC=UsrInputTypeFC;

 if(gi->input[0]>=0)
  InputType[0]=gi->input[0];
 if(gi->input[1]>=0)
  InputType[1]=gi->input[1];
 if(gi->inputfc>=0)
  InputTypeFC=gi->inputfc;
 FCEUI_GetCurrentVidSystem(&srendline,&erendline);
}

void FCEUD_PrintError(char *s)
{
 puts(s);
}

static char *cpalette=0;
static void LoadCPalette(void)
{
 char tmpp[192];
 FILE *fp;

 if(!(fp=fopen(cpalette,"rb")))
 {
  printf(" Error loading custom palette from file: %s\n",cpalette);
  return;
 }
 fread(tmpp,1,192,fp);
 FCEUI_SetPaletteArray((uint8 *)tmpp);
 fclose(fp);
}

static CFGSTRUCT fceuconfig[]={
	AC(soundvol),
	ACS(cpalette),
	AC(ntsctint),
	AC(ntschue),
	AC(ntsccol),
	AC(UsrInputTypeFC),
	ACA(UsrInputType),
	AC(powerpadside),
	AC(powerpadsc),
	AC(eoptions),
	ACA(srendlinev),
	ACA(erendlinev),
	ADDCFGSTRUCT(DriverConfig),
	ENDCFGSTRUCT
};

void SaveConfig(const char *name)
{
	char tdir[2048];
	if (name)
	     sprintf(tdir,"%s"PSS"cfg"PSS"%s.cfg",BaseDirectory,name);
	else sprintf(tdir,"%s"PSS"fceu2.cfg",BaseDirectory);
        DriverInterface(DES_GETNTSCTINT,&ntsctint);
        DriverInterface(DES_GETNTSCHUE,&ntschue);
        SaveFCEUConfig(tdir,fceuconfig);
}

static void LoadConfig(const char *name)
{
	char tdir[2048];
	if (name)
	     sprintf(tdir,"%s"PSS"cfg"PSS"%s.cfg",BaseDirectory,name);
	else sprintf(tdir,"%s"PSS"fceu2.cfg",BaseDirectory);
        LoadFCEUConfig(tdir,fceuconfig);
        if(ntsctint>=0) DriverInterface(DES_SETNTSCTINT,&ntsctint);
        if(ntschue>=0) DriverInterface(DES_SETNTSCHUE,&ntschue);
}

static void LoadLLGN(void)
{
 char tdir[2048];
 FILE *f;
 int len;
 sprintf(tdir,"%s"PSS"last_rom.txt",BaseDirectory);
 f=fopen(tdir, "r");
 if(f)
 {
  len = fread(lastLoadedGameName, 1, sizeof(lastLoadedGameName)-1, f);
  lastLoadedGameName[len] = 0;
  fclose(f);
 }
}

static void SaveLLGN(void)
{
 // save last loaded game name
 if (lastLoadedGameName[0])
 {
  char tdir[2048];
  FILE *f;
  sprintf(tdir,"%s"PSS"last_rom.txt",BaseDirectory);
  f=fopen(tdir, "w");
  if(f)
  {
   fwrite(lastLoadedGameName, 1, strlen(lastLoadedGameName), f);
   fclose(f);
   sync();
  }
 }
}


static void CreateDirs(void)
{
 char *subs[]={"fcs","snaps","gameinfo","sav","cheats","cfg"};
 char tdir[2048];
 int x;

 mkdir(BaseDirectory,S_IRWXU);
 for(x=0;x<sizeof(subs)/sizeof(subs[0]);x++)
 {
  sprintf(tdir,"%s"PSS"%s",BaseDirectory,subs[x]);
  mkdir(tdir,S_IRWXU);
 }
}

static void SetSignals(void (*t)(int))
{
  int sigs[11]={SIGINT,SIGTERM,SIGHUP,SIGPIPE,SIGSEGV,SIGFPE,SIGKILL,SIGALRM,SIGABRT,SIGUSR1,SIGUSR2};
  int x;
  for(x=0;x<11;x++)
   signal(sigs[x],t);
}

static void CloseStuff(int signum)
{
	DriverKill();
        printf("\nSignal %d has been caught and dealt with...\n",signum);
        switch(signum)
        {
         case SIGINT:printf("How DARE you interrupt me!\n");break;
         case SIGTERM:printf("MUST TERMINATE ALL HUMANS\n");break;
         case SIGHUP:printf("Reach out and hang-up on someone.\n");break;
         case SIGPIPE:printf("The pipe has broken!  Better watch out for floods...\n");break;
         case SIGSEGV:printf("Iyeeeeeeeee!!!  A segmentation fault has occurred.  Have a fluffy day.\n");break;
	 /* So much SIGBUS evil. */
	 #ifdef SIGBUS
	 #if(SIGBUS!=SIGSEGV)
         case SIGBUS:printf("I told you to be nice to the driver.\n");break;
	 #endif
	 #endif
         case SIGFPE:printf("Those darn floating points.  Ne'er know when they'll bite!\n");break;
         case SIGALRM:printf("Don't throw your clock at the meowing cats!\n");break;
         case SIGABRT:printf("Abort, Retry, Ignore, Fail?\n");break;
         case SIGUSR1:
         case SIGUSR2:printf("Killing your processes is not nice.\n");break;
        }
        exit(1);
}

static int DoArgs(int argc, char *argv[])
{
        static char *cortab[5]={"none","gamepad","zapper","powerpad","arkanoid"};
        static int cortabi[5]={SI_NONE,SI_GAMEPAD,
                               SI_ZAPPER,SI_POWERPADA,SI_ARKANOID};
	static char *fccortab[5]={"none","arkanoid","shadow","4player","fkb"};
	static int fccortabi[5]={SIFC_NONE,SIFC_ARKANOID,SIFC_SHADOW,
			         SIFC_4PLAYER,SIFC_FKB};

	int x, ret;
	static char *inputa[2]={0,0};
	static char *fcexp=0;
	static int docheckie[4];

        static ARGPSTRUCT FCEUArgs[]={
         {"-soundvol",0,&soundvol,0},
         {"-cpalette",0,&cpalette,0x4001},

         {"-ntsccol",0,&ntsccol,0},
         {"-pal",&docheckie[0],0,0},
         {"-input1",0,&inputa[0],0x4001},{"-input2",0,&inputa[1],0x4001},
         {"-fcexp",0,&fcexp,0x4001},

         {"-gg",&docheckie[1],0,0},
         {"-no8lim",0,&eoptions,0x8001},
         {"-subase",0,&eoptions,0x8002},
         {"-snapname",0,&eoptions,0x8000|EO_SNAPNAME},
	 {"-nofs",0,&eoptions,0x8000|EO_NOFOURSCORE},
         {"-clipsides",0,&eoptions,0x8000|EO_CLIPSIDES},
	 {"-nothrottle",0,&eoptions,0x8000|EO_NOTHROTTLE},
         {"-slstart",0,&srendlinev[0],0},{"-slend",0,&erendlinev[0],0},
         {"-slstartp",0,&srendlinev[1],0},{"-slendp",0,&erendlinev[1],0},
	 {0,(void *)DriverArgs,0,0},
	 {0,0,0,0}
        };

        memset(docheckie,0,sizeof(docheckie));
	ret=ParseArguments(argc, argv, FCEUArgs);
	if(cpalette)
	{
  	 if(cpalette[0]=='0')
	  if(cpalette[1]==0)
	  {
	   free(cpalette);
	   cpalette=0;
	  }
	}
	if(docheckie[0])
	 Settings.region_force=2;
	if(docheckie[1])
	 FCEUI_SetGameGenie(1);
        FCEUI_DisableSpriteLimitation(1);
        FCEUI_SaveExtraDataUnderBase(eoptions&2);
	FCEUI_SetSnapName(eoptions&EO_SNAPNAME);

	for(x=0;x<2;x++)
	{
         if(srendlinev[x]<0 || srendlinev[x]>239) srendlinev[x]=0;
         if(erendlinev[x]<srendlinev[x] || erendlinev[x]>239) erendlinev[x]=239;
	}

	printf("main() setrendered lines: %d, %d, %d, %d\n",srendlinev[0],erendlinev[0],srendlinev[1],erendlinev[1]);
        printf("main() clip sides %d\n", eoptions&EO_CLIPSIDES);
        srendlinev[0]=0;
        FCEUI_SetRenderedLines(srendlinev[0],erendlinev[0],srendlinev[1],erendlinev[1]);
        FCEUI_SetRenderedLines(0,erendlinev[0],srendlinev[1],erendlinev[1]);
        FCEUI_SetSoundVolume(soundvol);
	DriverInterface(DES_NTSCCOL,&ntsccol); // TODO
	DoDriverArgs();

	if(fcexp)
	{
	 int y;
         for(y=0;y<5;y++)
         {
          if(!strncmp(fccortab[y],fcexp,8))
          {
	   UsrInputTypeFC=fccortabi[y];
	   break;
          }
         }
	 free(fcexp);
	}
	for(x=0;x<2;x++)
	{
	 int y;

         if(!inputa[x])
	  continue;

	 for(y=0;y<5;y++)
	 {
	  if(!strncmp(cortab[y],inputa[x],8))
	  {
	   UsrInputType[x]=cortabi[y];
	   if(y==3)
	   {
	    powerpadside&=~(1<<x);
	    powerpadside|=((((inputa[x][8])-'a')&1)^1)<<x;
	   }
	   free(inputa[x]);
	  }
	 }
	}
	return ret;
}


#include "usage.h"

int CLImain(int argc, char *argv[])
{
	int last_arg_parsed;
        /* TODO if(argc<=1)
        {
         ShowUsage(argv[0]);
         return 1;
        }*/

        if(!DriverInitialize())
        {
	 return 1;
        }

	if(!FCEUI_Initialize())
         return(1);
        GetBaseDirectory(BaseDirectory);
	FCEUI_SetBaseDirectory(BaseDirectory);
	lastLoadedGameName[0] = 0;

	CreateDirs();
        LoadConfig(NULL);
        last_arg_parsed=DoArgs(argc-1,&argv[1]);
	gp2x_opt_setup();
	gp2x_cpuclock_gamma_update();
	LoadLLGN();
	if(cpalette)
	 LoadCPalette();
	if(InitSound())
	 inited|=1;

	if (argc > 1 && !last_arg_parsed)
	{
	 strncpy(lastLoadedGameName, argv[argc-1], sizeof(lastLoadedGameName));
	 lastLoadedGameName[sizeof(lastLoadedGameName)-1] = 0;
	 Exit = 0;
	}
	else
	{
	 Exit = 1;
	}

	while (1)
	{
	 if(!Exit)
	 {
	  if (fceugi)
	   CloseGame();
          fceugi=FCEUI_LoadGame(lastLoadedGameName);
	  if (fceugi)
	  {
	   LoadConfig(lastLoadedGameName);
	   if (Settings.region_force)
	    FCEUI_SetVidSystem(Settings.region_force - 1);
	   ParseGI(fceugi);
	   //RefreshThrottleFPS();
	   InitOtherInput();

	   // additional print for gpfce
	   // TODO: handlers for other formats then iNES
	   {
	  int MapperNo;
	  iNES_HEADER *head = iNESGetHead(); // TODO: ReMake
          MapperNo = (head->ROM_type>>4);
          MapperNo|=(head->ROM_type2&0xF0);
	  FCEU_DispMessage("%s, Mapper: %d%s%s", PAL?"PAL":"NTSC", MapperNo, (head->ROM_type&2)?", BB":"", (head->ROM_type&4)?", T":"");
	   }
	  }
	  else
	  {
	   switch(LoadGameLastError) {
	    default: strcpy(menuErrorMsg, "failed to load ROM"); break;
	    case  2: strcpy(menuErrorMsg, "Can't find a ROM for movie"); break;
	    case 10: strcpy(menuErrorMsg, "FDS BIOS ROM is missing, read docs"); break;
	    case 11: strcpy(menuErrorMsg, "Error reading auxillary FDS file"); break;
	   }
	  }
	 }
         if(Exit || !fceugi)
         {
          int ret;
	  ret = gp2x_menu_do();
	  if (ret == 1) break;		// exit emu
	  if (ret == 2) {		// reload ROM
	   Exit = 0;
	   continue;
	  }
         }

	 PrepareOtherInput();
	 gp2x_video_changemode(Settings.scaling == 3 ? 15 : 8);
	 switch (Settings.scaling & 3) {
		 case 0: gp2x_video_RGB_setscaling(0, 320, 240); gp2x_video_set_offs(0); break;
		 case 1: gp2x_video_RGB_setscaling(0, 256, 240); gp2x_video_set_offs(32); break;
		 case 2: gp2x_video_RGB_setscaling(0, 256, 240); gp2x_video_set_offs(32); break; // TODO
		 case 3: gp2x_video_RGB_setscaling(0, 320, 240); gp2x_video_set_offs(32); break;
	 }
	 gp2x_start_sound(Settings.sound_rate, 16, 0);
	 FCEUI_Emulate();
	}

	if (fceugi)
	 CloseGame();

	SaveLLGN();
	DriverKill();
        return 0;
}

static int DriverInitialize(void)
{
   SetSignals((void *)CloseStuff);

   if(!InitVideo()) return 0;
   inited|=4;
   return 1;
}

static void DriverKill(void)
{
 // SaveConfig(NULL); // done explicitly in menu now
 SetSignals(SIG_IGN);

 if(inited&4)
  KillVideo();
 if(inited&1)
  KillSound();
 inited=0;
}

void FCEUD_Update(uint8 *xbuf, int16 *Buffer, int Count)
{
 if(!Count && !(eoptions&EO_NOTHROTTLE))
  SpeedThrottle();
 BlitScreen(xbuf);
 if(Count && !(eoptions&EO_NOTHROTTLE))
  WriteSound(Buffer,Count);
 FCEUD_UpdateInput();
}

