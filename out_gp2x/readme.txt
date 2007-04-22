=================================================================================
                     GPFCE - NES emulator for the GP2X
=================================================================================
                         Ported by:        zzhu8192
                         Optimized by:     notaz
                         Current version:  0.3
                         Email:            zzhu8192@yahoo.com
                         Web Site:         www.unicorn-jockey.com
                         Web Site Admin:   Lil-kun
=================================================================================


This is a gp2x port of the **great** Open Source NES emulator FCE Ultra:
http://fceultra.sourceforge.net.   If you enjoyed using this emulator, please
keep in mind that this would not have been possible without the hard work and
dedication of the FCE Ultra developers.

In case you don't know what the NES is about, see:
http://en.wikipedia.org/wiki/Famicom.

My main reasons for working on this port is to get some game programming 
experience on smaller devices.  Although coding/porting an emu is 
generally not the same thing, it's still pretty cool.
I'm an Enterprise Java software developer by trade, so this is a nice side project
for me, and a welcome change of pace.  I'm also planning to write some 
original games in Java for the GP2x.  This should be interesting....  


------------------------------------------------------------------
What's new
------------------------------------------------------------------

Many usability features were added, thanks to some great input 
from developers and users on the gp32x.com board.  Some of the
changes went into the selector frontend.

(I have sent my selector customization changes to kounch, 
 so hopefully the changes will make it into
 version 1.2 or later for other projects to use)

There are now 4 executable scripts.
gpfce
gpfce_showfps
gpfce_swapbuttons
gpfce_showfps_swapbuttons

showfps:  this means FPS is displayed on the upper left of the screen
          (in non-stretch mode only).   

swapbuttons:  this means instead of Y/A as NES_B  and B/X as NES_A
              use  A/X as NES_B, and Y/B as NES_A.  (think NES MAX)
              
Volume meter is now shown on the OSD when adjusted.


See version history below for more details.

Depending on feedback, speed and even more compability will 
probably be the major focus going forward.


------------------------------------------------------------------
Current Features
------------------------------------------------------------------
- .zip file support
- 22050 Hz Mono Sound support with volume control support
- OSD Volume bar
- Load/Save state (up to 10 slots, pick by holding down R)
- Hardware stretch (See controls)
- Soft Reset
- Savegame support
- 60 FPS without frame skipping on many games
- Configurable Turbo Fire (hold L to toggle)
- Game genie/Cheat code (functionality already exists in FCEU 0.81)
- Sorted display of 2048 roms per subdirectory (recommended 512-1024)?
- Semi-Configurable button layout (startup only)
- configurable FPS display on upper left hand side

--------------------------------------------------------------------
 Version History
--------------------------------------------------------------------


ver 0.4 (by notaz)

          - Compatibility fixes for the asm core.
          - Fixed an anligment problem in MMC5 mapper.


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
Usage
------------------------------------------------------------------
1)  Untar the emulator tarball onto some directory on your SD card.
2)  You must have a directory called /roms/nes on your SD card.
    Put the roms in there, i.e. /mnt/sd/roms/nes is the gp2x path.
    Rom files can be zipped.
3)  The emulator will create a subdirectory under roms  
    /roms/nes/fceultra.  Save states etc. go here.
4)  To start a different rom while running one, hit L+R+JOY.
    To exit the file selection menu, press start.
5)  For FDS support, put disksys.rom in /mnt/sd/roms/nes/fceultra
    Note: FDS roms must not be zipped.
6)  For GameGenie support, put gg.rom in /mnt/sd/roms/nes/fceultra

------------------------------------------------------------------
Cheats
------------------------------------------------------------------

gpfce uses the cheat mechanisms already provided by
http://mednafen.com/documentation/cheat.html  Note this feature
is untested on gpfce as of version 0.2

To use game genie, place appropraite gg.rom into /mnt/sd/roms/nes/fceultra.
Use -gg on commandline to activate the game genie rom.


------------------------------------------------------------------
Controls
------------------------------------------------------------------

Note: JOY means press in on the joystick (i.e. not up/down/left/right)


In file selector mode
(This is based on selector version 1.1)
----------------------------------------------
Start        - EXIT file selector, back to main menu.
B,A,X,Y, JOY - start rom
L            - page up
R            - page down
UP           - scroll up one, wraps if at top
DOWN         - scroll down one, wraps if at bottom
LEFT         - jump up list by first letter of rom name
RIGHT        - jump down list by first letter of rom name
               



In game
------------------------
Y         - NES B
A         - NES B
B         - NES A
X         - NES A
SELECT    - NES SELECT
START     - NES START
VOL +/-   - Vol control.  

hold L    - toggles between  
            No Turbo Fire, 
            Upper row of buttons turbo fire
            Lower row of buttons turbo fire  
             
hold R    - cycles through save state slots

L & JOY   - Save state
R & JOY   - Load State
SEL & JOY - Stretch screen toggle
L & R     - Reset NES

L + R + JOY - Exit back to menu


FDS only
-------------------------------
L         - insert disk
R         - eject disk
JOY       - select disk



------------------------------------------------------------------
Known issues
------------------------------------------------------------------
1)  Stretch mode could use a better aspect ratio.  Looks a bit odd.
    Might look into some fancy non-2x filtering algorthms, if there are such things.
2)  Not thoroughly QA'd
3)  S-Video not working
4)  FDS does not seem to be working
5)  Some compatibility issues
6)  Can't sustain 60fps on some games
7)  Some clipping issues in some games
8)  Game genie behaves a bit odd, although code works...

------------------------------------------------------------------
 Might have time to do list
------------------------------------------------------------------
1) Fix more known issues
2) Better looking stretch?
3) File based Game genie support
4) Cleaner build
5) Full speed on all games.  (This may require some work)
6) Better compability
7) Multiplayer support via cable - this one is obviously pretty
   tricky.  Will need cables and a usb hub first.
   It's definitely possible, but is not a priority at this point.    

       
------------------------------------------------------------------
 Many thanks
------------------------------------------------------------------
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


