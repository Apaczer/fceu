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
#include "throttle.h"
#include "config.h"
#include "args.h"
#include "unixdsp.h"
#include "cheat.h"
#include "dface.h"
#include "platform.h"
#include "menu.h"
#include "settings.h"

#include "../../fce.h"
#include "../../cart.h"


// these are now here to try to make compatible configs
// between different versions of the emu
DSETTINGS Settings;
CFGSTRUCT DriverConfig[]={
	AC(Settings.turbo_rate_add),
	AC(Settings.sound_rate),
	AC(Settings.showfps),
	AC(Settings.scaling),
	AC(Settings.frameskip),
	AC(Settings.sstate_confirm),
	AC(Settings.region_force),
	AC(Settings.cpuclock),
	AC(Settings.mmuhack),
	AC(Settings.ramtimings),
	AC(Settings.gamma),
	AC(Settings.perfect_vsync),
	AC(Settings.accurate_mode),
	ENDCFGSTRUCT
};

void CleanSurface(void);

// internals
extern uint8 Exit; // exit emu loop flag
extern int FSkip;
void CloseGame(void);

FCEUGI *fceugi = NULL;
int ntsccol=0,ntschue=-1,ntsctint=-1;
int soundvol=70;
int inited=0;

int srendlinev[2]={8,0};
int erendlinev[2]={231,239};
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
}

void FCEUD_PrintError(char *s)
{
 puts(s);
}

char *cpalette=0;
void LoadCPalette(void)
{
 char tmpp[192];
 FILE *fp;

 if(!(fp=fopen(cpalette,"rb")))
 {
  printf(" Error loading custom palette from file: %s\n",cpalette);
  free(cpalette);
  cpalette=0;
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

static const char *skip_path(const char *path)
{
	const char *p;
	if (path == NULL) return NULL;
	for (p = path+strlen(path)-1; p > path && *p != '/'; p--);
	if (*p == '/') p++;
	return p;
}

int SaveConfig(const char *llgn_path)
{
	const char *name = skip_path(llgn_path);
	char tdir[2048];
	int ret;
	if (name)
	     sprintf(tdir,"%s"PSS"cfg"PSS"%s.cfg",BaseDirectory,name);
	else sprintf(tdir,"%s"PSS"fceu2.cfg",BaseDirectory);
	printf("saving cfg to %s ... ", tdir); fflush(stdout);
	FCEUI_GetNTSCTH(&ntsctint, &ntschue);
        ret=SaveFCEUConfig(tdir,fceuconfig);
	printf(ret == 0 ? "done\n" : "failed\n");
	return ret;
}

static int LoadConfig(const char *llgn_path)
{
	const char *name = skip_path(llgn_path);
	char tdir[2048];
	int ret;
	if (name)
	     sprintf(tdir,"%s"PSS"cfg"PSS"%s.cfg",BaseDirectory,name);
	else sprintf(tdir,"%s"PSS"fceu2.cfg",BaseDirectory);
	printf("loading cfg from %s ... ", tdir); fflush(stdout);
	FCEUI_GetNTSCTH(&ntsctint, &ntschue); /* Get default settings for if no config file exists. */
        ret=LoadFCEUConfig(tdir,fceuconfig);
	FCEUI_SetNTSCTH(ntsccol, ntsctint, ntschue);
	printf(ret == 0 ? "done\n" : "failed\n");
	return ret;
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
 if (lastLoadedGameName[0] && !(eoptions&EO_NOAUTOWRITE))
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
 char *subs[]={"fcs","snaps","gameinfo","sav","cheats","cfg","pal"};
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
	 #if(SIGBUS!=SIGSEGV)
         case SIGBUS:printf("I told you to be nice to the driver.\n");break;
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
#ifdef NETWORK
        static int docheckie2[2]={0,0};
#endif

        static ARGPSTRUCT FCEUArgs[]={
         {"-soundvol",0,&soundvol,0},
         {"-cpalette",0,&cpalette,0x4001},

         {"-ntsccol",0,&ntsccol,0},
         {"-pal",&docheckie[0],0,0},
         {"-input1",0,&inputa[0],0x4001},{"-input2",0,&inputa[1],0x4001},
         {"-fcexp",0,&fcexp,0x4001},

         {"-gg",0,&eoptions,0x8000|EO_GG},
         {"-no8lim",0,&eoptions,0x8000|EO_NO8LIM},
         {"-snapname",0,&eoptions,0x8000|EO_SNAPNAME},
	 {"-nofs",0,&eoptions,0x8000|EO_NOFOURSCORE},
         {"-clipsides",0,&eoptions,0x8000|EO_CLIPSIDES},
	 {"-nothrottle",0,&eoptions,0x8000|EO_NOTHROTTLE},
	 {"-noautowrite",0,&eoptions,0x8000|EO_NOAUTOWRITE},
         {"-slstart",0,&srendlinev[0],0},{"-slend",0,&erendlinev[0],0},
         {"-slstartp",0,&srendlinev[1],0},{"-slendp",0,&erendlinev[1],0},
         {"-sound",0,&Settings.sound_rate,0},
         {"-showfps",0,&Settings.showfps,0},
         #ifdef NETWORK
         {"-connect",&docheckie2[0],&netplayhost,0x4001},
         {"-server",&docheckie2[1],0,0},
         {"-netport",0,&Port,0},
         #endif
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
	FCEUI_SetGameGenie(eoptions&EO_GG);
        FCEUI_DisableSpriteLimitation(eoptions&EO_NO8LIM);
	FCEUI_SetSnapName(eoptions&EO_SNAPNAME);

	for(x=0;x<2;x++)
	{
         if(srendlinev[x]<0 || srendlinev[x]>239) srendlinev[x]=0;
         if(erendlinev[x]<srendlinev[x] || erendlinev[x]>239) erendlinev[x]=239;
	}

        FCEUI_SetRenderedLines(srendlinev[0],erendlinev[0],srendlinev[1],erendlinev[1]);
        FCEUI_SetSoundVolume(80);
	#ifdef NETWORK
        if(docheckie2[0])
         netplay=2;
        else if(docheckie2[1])
         netplay=1;

        if(netplay)
         FCEUI_SetNetworkPlay(netplay);
	#endif
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

int main(int argc, char *argv[])
{
	int last_arg_parsed, ret;
        /* TODO if(argc<=1)
        {
         ShowUsage(argv[0]);
         return 1;
        }*/

        platform_init();
        if(!DriverInitialize())
        {
	 return 1;
        }

	if(!FCEUI_Initialize())
         return(1);
        GetBaseDirectory(BaseDirectory);
	FCEUI_SetBaseDirectory(BaseDirectory);
	lastLoadedGameName[0] = 0;
	menu_init();
	in_init();

	CreateDirs();
        LoadConfig(NULL);
        last_arg_parsed=DoArgs(argc-1,&argv[1]);
	platform_late_init();

	LoadLLGN();
	FCEUI_SetNTSCTH(ntsccol, ntsctint, ntschue);
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
	  ret=LoadConfig(lastLoadedGameName);
	  if (ret != 0)
	  {
	   LoadConfig(NULL);
	  }
	  FCEUI_SetEmuMode(Settings.accurate_mode);
          fceugi=FCEUI_LoadGame(lastLoadedGameName);
	  if (fceugi)
	  {
	   char infostring[32];
	   if (Settings.region_force)
	    FCEUI_SetVidSystem(Settings.region_force - 1);
	   ParseGI(fceugi);
	   InitOtherInput();

	   if ((eoptions&EO_GG) && geniestage == 0) {
	    strcpy(infostring, "gg.rom is missing, GG disabled");
	    eoptions&=~EO_GG;
	    FCEUI_SetGameGenie(0);
	   } else
	    GameInterface(GI_INFOSTRING, infostring);
	   FCEU_DispMessage("%s", infostring);
	  }
	  else
	  {
	   switch(LoadGameLastError) {
	    default: menu_update_msg("failed to load ROM"); break;
	    case  2: menu_update_msg("Can't find a ROM for ips/movie"); break;
	    case 10: menu_update_msg("FDS BIOS ROM is missing, read docs"); break;
	    case 11: menu_update_msg("Error reading auxillary FDS file"); break;
	   }
	  }
	 }
         if(Exit || !fceugi)
         {
          int ret;
	  ret = menu_loop();
	  if (ret == 1) break;		// exit emu
	  if (ret == 2) {		// reload ROM
	   Exit = 0;
	   continue;
	  }
         }

	 PrepareOtherInput();
	 FCEUI_GetCurrentVidSystem(&srendline,&erendline);
	 platform_apply_config();
	 CleanSurface();
	 StartSound();
	 RefreshThrottleFPS();
	 FCEUI_Emulate();
	}

	if (fceugi)
	 CloseGame();

	SaveLLGN();
	FCEUI_Kill();
	DriverKill();
	platform_finish();
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
 SetSignals(SIG_DFL);

 if(cpalette) free(cpalette);
 cpalette=0;

 if(inited&4)
  KillVideo();
 if(inited&1)
  KillSound();
 inited=0;
}

void FCEUD_Update(uint8 *xbuf, int16 *Buffer, int Count)
{
 FCEUD_UpdateInput();	// must update input before blitting because of save confirmation stuff
 BlitPrepare(xbuf == NULL);
 if(!(eoptions&EO_NOTHROTTLE))
 {
  if(Count)
   WriteSound(Buffer,Count);
  SpeedThrottle();
 }
 BlitScreen(xbuf == NULL);
 // make sure last frame won't get skipped, because we need it for menu bg
 if (Exit) FSkip=0;
}


