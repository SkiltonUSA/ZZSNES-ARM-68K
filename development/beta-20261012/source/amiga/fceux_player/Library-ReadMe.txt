ZZNes Library 0.4.0-ui-preview (development preview)
======================================

Double-click ZZNesLibrary to open the native MUI interface.
Requires AmigaOS 3.x, MUI 3.8, ZZ9000 XX19c / XACP 1.7 and the
existing 320x240 32-bit RTG screen mode used by ZZFceux.
Keep ZZFceux-ui beside ZZNesLibrary. The supplied icon sets a 64K stack.
Close other ZZ9000 ARM applications before playing.

GAMES
The shared ZZNES/ZZSNES MUI layout has a searchable game list, All games /
Recently played / Favourites buttons, and a selected-game panel.
Add game imports one file; Add folder imports files directly in that drawer.
Up to 128 games. Set title edits a display name without renaming the ROM.
Artwork links a local <=512x512, <=1 MiB picture through a V43 picture datatype.
PNG/JPEG support depends on installed datatypes; artwork is optional.
Without artwork, the game captures a library picture after 300 frames if none
exists. Press F9 at the title screen to replace it; this also works paused.
It is a timed capture, not title recognition, and briefly pauses presentation.
Snapshots are local and require no picture datatype or download.
Play from start begins a new session. Continue latest save restores the newest
readable matching save by Amiga file date. Choose save state opens the gallery.
Missing/unsupported ROMs and unavailable saves disable the relevant actions.
Removing a library entry never deletes its ROM, artwork or saves.
Q or Escape returns from the game to the library.

DISPLAY
Choose your monitor's physical shape: 4:3 or 16:9 widescreen. The game keeps
a 4:3 shape. Set the monitor to fill its panel; if it already adds 4:3 side
bars, choose 4:3 in the app to avoid adding a second set of bars.
Left/right and top/bottom masks independently hide 0, 8 or 16 NES pixels per
edge without zooming. Default: 8 left/right, none top/bottom.
Choose black, white, console grey/red, cream/burgundy or starfield borders.
These are original procedural designs, not a CRT shader or Nintendo art pack.

CONTROLLERS
The live test reads the physical Amiga joystick port 2 (lowlevel port 1).
Move the stick or press fire to allow automatic type detection.
A standard C64 stick has one button: choose Fire = A or Fire = B.
Keyboard: arrows = directions; Z = A; X = B; Enter = Start;
left Shift = Select; P = pause; M = mute; Q/Escape = quit.
A supported multi-button controller also maps blue to the other face button,
green to Select and Play to Start. The mouse port is not reconfigured.

SAVE STATES
Five cards show slot number, date/time, preview, Use slot and Resume.
Existing NES states contain no screenshot; the cards explicitly say so.
Use slot selects where F5 saves / F8 restores while playing, including paused.
Previous saves remain as .bak files. The player still checks core compatibility
when restoring. Timestamps come from the Amiga clock.
States stay in States/, keyed by the original ROM library path; changing the
display title does not change the key. Moving/renaming a ROM still does.
The private .zzs state ABI is unchanged; desktop .mst/.fcs are not supported.

SUPPORTED CORE
This fast player runs pinned FCEUX on Cortex-A9 Core1, not Mesen.
Currently NTSC iNES mapper 0 and mapper 90, CHR ROM only; no battery, trainer
or four-screen cartridges. ZIP and SNES files are not supported.
AHI unit 0 supplies sound. No firmware or global AHI preferences are changed.
The earlier accepted player remains a separate installation.

FILES
Library.cfg: v2 settings, titles, artwork links, favourites and history.
Library.bak: prior copy. Library.pre-browser: preserved v1 migration copy.
Downgrading requires restoring both the old launcher and its old Library.cfg.
Session.cfg: settings for the current launch. Last-session.log: diagnostics.
States/: five save states per ROM path. Keep this drawer writable.
Snapshots/: game-library pictures keyed by ROM content; previous file is .bak.
Only the original Controller-Test cartridge is included; supply your own games.

STATUS
Cross-built and sanitizer-tested. Automatic capture and clean return passed
on the A4000TX in a separate preview drawer. Broader UI acceptance is pending.
The 68k player gained snapshot hooks; ARM runtime and save ABI are unchanged.

CREDITS
Visual-library ideas: github.com/sc0ttj/rom-loader and launchbox-app.com.
Original native MUI implementation; no code/artwork copied from either.
Native MUI structure based on xSysInfo (BSD-2-Clause, LICENSE-xSysInfo).
Emulation: Rezzy-dev/fceux, pinned 4d01530b6689b06a6d7cf0679b63625e3270ae9a,
GPL-2.0-or-later. Integration: GPL-3.0-or-later.
XANI's XACP-ZZ9000 platform and fullscreen/PAN examples made this possible.
See overscan.md for the requested Mesen-S #122 discussion and its credited
comparison images. Those are SNES reference screenshots, not bundled bezels.

Source: https://github.com/SkiltonUSA/ZZNes-ARM-68K
