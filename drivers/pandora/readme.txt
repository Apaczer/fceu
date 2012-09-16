=================================================================================
                     GPFCE - NES emulator for Pandora
=================================================================================


This is Pandora port of optimized GP2X port of FCE Ultra NES emulator.
Homepages:
http://notaz.gp2x.de/gpfce.php
http://fceultra.sourceforge.net


------------------------------------------------------------------
 Usage
------------------------------------------------------------------

The emulator does not require any setup to play regular ROM images,
except for FDS support, for which disksys.rom is needed (see below).
ROM images gan be selected using built-in file browser, .zip files
are supported.

On pandora, saves and other files go to appdata directory, which is
usually in <SD card>/pandora/appdata/gpfce/ .
From here on this file will refer to it as <root>.


------------------------------------------------------------------
 Famicom Disk System
------------------------------------------------------------------

You will need the FDS BIOS ROM image in <root>/fceultra directory.
It's size should be 8192 bytes and it must be named "disksys.rom".
gpfce will not load FDS games without this file.

You will also probably need to configure keys for swapping the virtual FDS disks,
they are configurable in Controls menu.

Two types of FDS disk images are supported: disk images with the FWNES-style header,
and disk images with no header. The number of sides on headerless disk images is
calculated by the total file size, so don't put extraneous data at the end of the file.


------------------------------------------------------------------
 VS Unisystem
------------------------------------------------------------------

gpfce currently only supports VS Unisystem ROM images in the iNES format. See
FCE Ultra manual for the list of supported games.

You will need to configure a key to insert coins, see Controls option in the menu.


------------------------------------------------------------------
 Cheats
------------------------------------------------------------------

For cheating you can ether use the authentic game genie support, or the .cht files.

To use authentic game genie, place appropriate gg.rom into
<root>/fceultra.
Use the FCE Ultra option menu or -gg on commandline to activate the game genie rom.
The ROM image may either be the 24592 byte iNES-format image, or the 4352
byte raw ROM image.

The .cht files fould be placed into <root>/fceultra/cheats/ directory.
They should have the same base filename as the game you wish to associate with
with the extension "cht". Examples:

                File name:              Cheat file name:
                 BigBad.nes              BigBad.cht
                 BigBad.zip              BigBad.cht
                 BigBad.Better.nes       BigBad.Better.cht

If you have done everything correctly, Cheat option will appear in the main menu
after you load the ROM. You can activate/deactivate cheats by pressing B.
For description of .cht file format, see http://fceultra.sourceforge.net/cheat.php


------------------------------------------------------------------
 IPS patch support
------------------------------------------------------------------

Place the IPS files in the same directory as the ROM to load, and name them
filename.ips or filename.something.ips. Examples:


         File name:              IPS file names:
          BigBad.nes              BigBad.nes.ips           BigBad.nes.somehack.ips
          BigBad.zip              BigBad.zip.ips           BigBad.zip.badhack.ips
          BigBad.Better.nes       BigBad.Better.nes.ips    BigBad.Better.nes.evenbetterhack.ips

Patching is supported for all supported formats (iNES, FDS, UNIF, and NSF), but
it will probably only be useful for the iNES and FDS formats. UNIF files can't be
patched well with the IPS format because they are chunk-based with no fixed offsets.


------------------------------------------------------------------
 FCM movies
------------------------------------------------------------------

Version 0.4 has partial FCM movie support. Most of the movies desync due to
different timing, but some of them can be played. Note that 'accurate renderer'
option solves some desync problems. There is only playback support.
Files should be placed in the ROMs directory along with the ROMs themselves.
Naming is the same as for IPS patches (see previous section), buf use .fcm
extension instead of .ips.


--------------------------------------------------------------------
 Version History
--------------------------------------------------------------------


r1 (for pandora)
          - Ported to pandora, unified the code.
          - Added M-HT's filters and some pandora specific options/tweaks.

ver 0.4 (by notaz)
  ret 313
          - Improved open bus emulation, fixes missing ground in some SMB3 levels.
          - Improved auto frameskip behavior in cases when emu is not fast
            enough to maintain 50/60 fps.
          - Fixed a bug which prevented some key combo configurations from working.
          - Some other minor changes.
  rev 171
          - Added optional "Accurate renderer", which is the original FCE Ultra
            0.98.x renderer + PPU emulation code. It's much slower, but it can
            handle games which need more precise PPU timing emulation (like
            Marble Madness).
          - Fixed saving and loading of game specific configs.
          - Some other minor changes.
  rev 163
          - Added A r k's fast-direction-change fix for usbjoy lib.
          - Fixed an issue of usbjoys stopping to work when "Perfect vsync"
            is enabled.
  rev 162
          - Fixed savestate subsections (were causing some mapper data not
            to be saved).
          - Fixed an issue of MapIRQHook getting lost after loading a savestate
            (glitched Akumajou Densetsu and other games after savestate load).
          - A bug, which prevented configuring multiple USB pads fixed.
          - Fixed sound breaking bug after switching it on/off multiple times.
          - Added "Perfect VSYNC" option, which changes GP2X refresh rate and
            syncs emu timing to LCD vsync.
          - Fixed IPS patch support.
  rev 153
          - Lots of work on the asm core. Timing fixed for some instructions.
            Some missing undocumented instruction handlers added. Lots of
            tweaking to make it compatible with all that mapper code.
          - Completely new PicoDrive style menu added with most standard
            FCE Ultra and some additional options. Selector removed, ROM list
            is now built-in and only limited by available memory.
          - Merged in most of code from 0.98.1x versions. Only ppu/rendering
            and sound emulation code left from 0.81 (which is less accurate
            but much faster).
          - Default palette changed to one from later versions.
          - Fixed some alignment problems in MMC5 and some other mappers.
          - Some generic optimizations and code cleanup/refactoring.
          - The built-in NSF player fixed.
          - Authentic GameGenie support fixed.
          - FDS support fixed.
          - VS Unisystem support fixed.
          - Ingame saves fixed (not sure when they got broken).
          - Increased maximum sound volume.
          - Added Zapper emulation just for fun.
          - Added partial FCM movie support.
          - Fixed some memory leaks.
          - Fixed a bug which caused USB connection from GP2X menu
            to hang after using gpfce.
          - Software scaler added.
          - USB gamepad support added.
          - TV out fixed.
          - Documentation updated.


ver 0.3 (by notaz)

          - Major improvement: added ARM asm CPU core from LJGP32,
            which itself was adapted from FCA by Yoyofr.
            The core required substantial changes to make it work in
            FCE ultra.
          - The emulator renders directly to frame buffer now (previously
            it was drawing to offscreen buffer, which was then copied to
            framebuffer).
          - Squidge's MMU hack added.
          - Added sync() calls after savestate writes.
          - Some additional tweaking here and there to get a few more FPS.
          - Volume middle now can be used as shift to emulator functions
            instead stick click (saving, stretching, etc.).
          - Added frameskip selection with shift+A and shift+Y (shift is
            stick click or volume middle).
          - Probably some more changes I forgot about.



ver 0.2   5/29/2006  MD5SUM: dd75fa3f090f9298f9f4afff01ab96f2 *gpfce

          - Sound output issue with stereo fixed, now using
            22050 khz 16-bit mono.  I've tried interpolating to 
            44khz mono, but the results seemed similar.
          - selector supports up to 2048 files, sorted, with
            alpha scrolling via left/right in addition to 
            page up/down via L/R.
          - additional startup scripts to select button and fps  
            configurations
          - can load FDS files, but does not seem to work yet
          - configurable buttons  (use swapbuttons version )
          - configurable fps (use showfps version)
          - Configurable turbo fire control
          - Selectable save slots from 0-9
          - Volume bar
          - compiled with GCC 4.1.0 -O3 with profiling


ver 0.1   5/23/2006  MD5SUM: 13681f25713ad04c535c23f8c61f1e0b *gpfce

          - Initial version
          - Around 60 fps with sound
          - Load/Save State
          - Hardware Stretch
          - Soft reset support
          - No GUI, using selector with config
          - Hard coded 22050 audio, 16-bit, stereo
          - compiled with GCC 4.1.0 -O3 with profiling
          - Hard coded config path.  This is to prevent users
            from filling up the gp2x space by accident


      
------------------------------------------------------------------
 Credits/thanks
------------------------------------------------------------------

- Original base code of FCE Ultra by Xodnizel, Bero.
- Porting/optimization/integration/frontend for 0.3 and 0.4 by notaz.
- Versions 0.1 and 0.2 by zzhu8192 (http://www.unicorn-jockey.com).
- asm CPU core from FCA and Little John GP32.
- Minimal library by rlyeh.
- Additional low-level GP2X libs by Hermes/PS2Reality, theoddbot,
  god_at_hell, Puck2099.

additional thanks:
- Cruel and DaveC from gp32x boards for beta testing.
- All FCE Ultra contributors listed in
  http://fceultra.sourceforge.net/docs.php

...and everyone whose name my mind has misplaced.

zzhu8192's thanks:
- To lots of talented developers on the http://www.gp32x.com/board/
  Reesy, Squidge, etc.  for responding to my technical questions.
- Thanks to 
- Lil-kun - for the neat GPFCE logo and the Web Site (under construction) :D
- Referenced source code from MameGP2X (Franxis) and FCEU-0.3 gp2x (Noname)
- Awesome wiki: http://wiki.gp2x.org/wiki/Main_Page
- Awesome gp2x site: http://www.gp32x.com/
- ryleh's minimal lib - w/o which this wouldn't have worked
- FCE Ultra developers (http://fceultra.sourceforge.net/) 
  for the wonderful and feature rich NES emulator.
- kounch for Selector frontend - works great for lazy developers like me.  :-D
  I have sent my changes to kounch, so hopefully the changes will make it into
  version 1.2 or later.
- gp2x community - just plain rocks


