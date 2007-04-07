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

/****************************************************************/
/*			FCE Ultra				*/
/*								*/
/*	This file contains code to interface to the standard    */
/*	FCE Ultra configuration file saving/loading code.	*/
/*								*/
/****************************************************************/
#include "../common/config.h"

static CFGSTRUCT fceuconfig[]={

        ACS(rfiles[0]),
        ACS(rfiles[1]),
        ACS(rfiles[2]),
        ACS(rfiles[3]),
        ACS(rfiles[4]),
        ACS(rfiles[5]),
        ACS(rfiles[6]),
        ACS(rfiles[7]),
        ACS(rfiles[8]),
        ACS(rfiles[9]),

        AC(ntsccol),AC(ntsctint),AC(ntschue),

        AC(soundsleep),
        NAC("palyo",palyo),
	NAC("genie",genie),
	NAC("fs",fullscreen),
	NAC("vgamode",vmod),
	NAC("sound",soundo),

        ACS(gfsdir),

        NACS("odcheats",DOvers[0]),
        NACS("odmisc",DOvers[1]),
        NACS("odnonvol",DOvers[2]),
        NACS("odstates",DOvers[3]),
        NACS("odsnaps",DOvers[4]),
        NACS("odbase",DOvers[5]),

	NAC("winsizemul",winsizemul),

	AC(soundrate),
        AC(soundbuftime),
	AC(soundoptions),
        AC(soundvolume),

	NAC("eoptions",eoptions),
        NACA("cpalette",cpalette),

        ACA(joy),
	ACA(joyA),ACA(joyB),ACA(joySelect),ACA(joyStart),

        AC(joyOptions),
        ACA(joyUp),ACA(joyDown),ACA(joyLeft),ACA(joyRight),

        NACA("InputType",UsrInputType),
	NAC("keyben",keybEnable),

	NACA("keybm0",keyBMap[0]),
	NACA("keybm1",keyBMap[1]),
	NACA("keybm2",keyBMap[2]),
	NACA("keybm3",keyBMap[3]),
	NACA("ppasc0",powerpadsc[0]),
	NACA("ppasc1",powerpadsc[1]),

	NAC("ppaside",powerpadside),
	NAC("vmcx",vmodes[0].x),
	NAC("vmcy",vmodes[0].y),
	NAC("vmcb",vmodes[0].bpp),
	NAC("vmcf",vmodes[0].flags),
	NAC("vmcxs",vmodes[0].xscale),
	NAC("vmcys",vmodes[0].yscale),

        NAC("srendline",srendlinen),
        NAC("erendline",erendlinen),
        NAC("srendlinep",srendlinep),
        NAC("erendlinep",erendlinep),

        AC(UsrInputTypeFC),
	AC(winsync),
        AC(fssync),
        AC(NoFourScore),
        ACA(fkbmap),
        ENDCFGSTRUCT
};

static void SaveConfig(char *filename)
{
        DriverInterface(DES_GETNTSCTINT,&ntsctint);
        DriverInterface(DES_GETNTSCHUE,&ntschue);
        SaveFCEUConfig(filename,fceuconfig);
}

static void LoadConfig(char *filename)
{
        DriverInterface(DES_GETNTSCTINT,&ntsctint);
        DriverInterface(DES_GETNTSCHUE,&ntschue);
        LoadFCEUConfig(filename,fceuconfig);
	DriverInterface(DES_NTSCCOL,&ntsccol);
        DriverInterface(DES_SETNTSCTINT,&ntsctint);
        DriverInterface(DES_SETNTSCHUE,&ntschue);

	palyo&=1;
	FCEUI_SetVidSystem(palyo);
	genie&=1;
	FCEUI_SetGameGenie(genie);
	fullscreen&=1;
        soundo&=1;
        FCEUI_SetSoundVolume(soundvolume);
}

