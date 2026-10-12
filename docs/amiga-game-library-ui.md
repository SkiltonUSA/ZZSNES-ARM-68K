# Shared ZZNES / ZZSNES game libraries

Current packages: **ZZNES 0.5.0 Beta** and **ZZSNES 0.4.0 Beta** (12 October
2026). Workbench launchers are named `ZZNES` and `ZZSNES`, with the version
shown in the window title and About tab. Installation drawers:

- `SD032G:Emulator/ZZNES` — 11 games, `ZZFceux-ui` player.
- `SD032G:Emulator/ZZSNES` — 10 games, `ZZSnes-ui` player.

Each drawer holds its ROMs, States, Snapshots, settings, credits and licences.
ROM basenames are at most 20 characters, leaving room for `.sav.bak` and
`.sav.tmp` on the target filesystem. Full display titles remain unchanged.
Library ROM paths use `PROGDIR:ROMs/`; path-keyed NES save filenames are
remapped during migration. SNES saves and captured thumbnails retain their
content keys. Previous development drawers remain as recovery copies.

Both Betas include the accepted host cadence scheduling and bounded picture
omission, retaining every emulation/APU frame. The SNES Beta promotes this
from a timed diagnostic into the normal interactive host, with save/load,
SRAM, snapshots and unrestricted play restored. ARM payloads, fullscreen
handoff, audio worker and state ABI are unchanged from the accepted bases.

Build: `PYTHONPATH=. python3 scripts/build_emulator_beta.py`, using the
verified local caches identified in its build manifest. The one-time
`scripts/prepare_emulator_move.py` consumes captured library configurations
and inventories; it is not a generic installer. ROMs and user saves are
excluded from source control and release packages.

## Design and credits

The user requested the organisation of
[rom-loader](https://github.com/sc0ttj/rom-loader) and the visual-library
approach of [LaunchBox](https://www.launchbox-app.com/). The supplied
[LaunchBox-Emulator GitHub account](https://github.com/LaunchBox-Emulator)
only exposed a profile repository during review; it was not verified as
LaunchBox's official source repository.

Ideas adopted: library filters, selected-game details/artwork, clear launch
actions, and game-specific save cards. This is original native MUI code;
no Linux shell/GTK or LaunchBox code, artwork or metadata database is bundled.
The five-slot save gallery is our design for our existing state formats.
MUI construction and lifecycle patterns retain the xSysInfo BSD attribution;
the shared frontend remains GPL-3.0-or-later. XANI's XACP fullscreen handoff
remains in the emulation players.

## Games page

- Search titles or original paths, without case sensitivity; alphabetical
  display or newest-played order. All games / Recently played / Favourites
  are visible buttons. The existing 128-game limit remains explicit.
- Game list on the left; title, header compatibility, preview, save summary
  and launch actions on the right. Hover over compatibility text for the
  original ROM path. The last selected entry and filter are remembered.
- **Set title** edits the display name. Default titles omit the extension
  and replace underscores with spaces. Region/revision tags remain visible;
  ROM filenames and existing save keys are never renamed.
- **Artwork...** links a local picture through OS picture datatypes. Use
  images no larger than 512x512 and 1 MiB; PNG/JPEG availability depends on
  installed datatypes with the V43 pixel-array interface. Missing artwork
  does not prevent play. Artwork is scaled proportionally into a small
  palette preview; no online lookup or image service is required.
- Without explicit artwork, a captured game screenshot supplies the preview.
  The newest SNES save screenshot is the fallback. NES save cards still have
  no embedded screenshot and explicitly say so.
- **Play from start**, **Continue latest save**, **Choose save state...**
  distinguish their effects. Missing/unsupported ROMs disable launch;
  Continue is disabled without a readable matching save envelope.
- Folder import reads the selected folder, not subfolders or ZIP archives.
  Removing an entry or artwork link leaves the corresponding files intact.

## Save-state gallery

Five slots **per game** appear together on a separate **Save states** tab, labelled with
the selected game. Each card has a preview, slot number, date/time,
**Resume** and **Delete...**. Empty or unreadable states are labelled and
cannot be resumed. `*` marks the slot selected for F5 save / F8 restore.
The **Active save slot** selector sets the target for in-game F5/F8; it does
not immediately save or load. It can select an empty slot for the next save.

Delete opens a confirmation naming the game and slot. Confirming removes that
slot's `.zzs` and `.zzs.bak`, then refreshes its card and Continue latest save.
Unreadable saves and backup-only slots can also be deleted; empty slots disable
Delete. Other slots, ROM files and library artwork are unaffected. A failure
removing the main save preserves its backup; a backup failure after main-file
deletion is reported explicitly. Cancel leaves both files intact.

SNES cards display the actual embedded 128x112 RGB565 snapshot, reduced to
112x98 for the gallery. Existing NES states show **No screenshot**; this
change does not invent previews or change their state ABI. The player still
validates its full internal core layout before restoring; browser validation
checks the envelope, ROM identity, length, ABI fields and checksum.

Continue selects the newest readable matching slot by Amiga file timestamp;
ties choose the lowest-numbered slot. The accuracy depends on the Amiga clock.
Existing `.bak` state recovery is unchanged unless the user explicitly deletes
that slot. The library never rewrites saves.

## Controls and settings

Both applications share Games, Display, Controllers and Save states tabs.
Display and controller preferences remain global to the application.
Physical-port-2 joystick up/down selects games; fire focuses Play for mouse
or keyboard activation. Live joystick testing and the existing in-game
keyboard mapping remain available in Controllers. The library hides while
the synchronous player runs, then refreshes saves on return.

Cover grids, automatic metadata lookup, per-game settings overrides and
complete joystick-only navigation remain future work.

## Local game screenshots

The player takes one automatic picture after 300 emulated frames if there is
no valid picture for that ROM. This is a timing heuristic, **not title-screen
recognition**: some cartridges still show a publisher logo, a fade or gameplay.
Press **F9** at the desired title or game screen to replace it; P pauses first
if needed. Capture is independent of F5/F8 save states. The browser refreshes
on return from play. No artwork download or picture datatype is required.

`Snapshots/<ROM-content hashes>.zzp` is a checksummed 128x112 RGB565 preview
(28,704 bytes); its 128x96 game image keeps 4:3 proportions within that canvas.
NES uses the cartridge's content hash; SNES uses its two existing normalized
content hashes. A renamed ROM can therefore reuse its library picture.
Console surrounds are cropped; selected in-game overscan masks remain visible.
F9 retains the previous picture as `.zzp.bak`. Explicit Artwork links override
a captured picture. Delete the corresponding `.zzp` to allow auto-capture again.

The 68k host reads its own completed offscreen bitmap after the ARM PRESENT
acknowledgement, drains outstanding flips, and releases the P96 lock before
thumbnail conversion and file I/O. This is an occasional Zorro readback, not a
per-frame change. Physical automatic captures caused roughly a quarter-second
hitch, so capture is unsuitable as a continuous recording feature. No new ARM
opcode, shared-memory allocation, cache policy or firmware feature is needed.
Invalid/blank automatic images are not substituted; F9 permits a deliberate
replacement. This first implementation makes one automatic attempt per launch.

## Configuration and rollback

`ZZNESUI1` / `ZZSNESUI1` configurations are read unchanged and written as
version 2 on save, adding title/artwork paths and remembered selection/filter.
The first migration preserves `Library.pre-browser` if it does not already
exist. `Library.bak` continues to hold the preceding configuration. A malformed
configuration stops startup instead of being silently replaced.

For a hardware trial, copy the existing application drawer first, preserving
its ROMs, States, config and icon; install the new library and matching
snapshot-enabled 68k player in that copy. Launchers require a 65536-byte stack, set in the Workbench icon or
with `Stack 65536` in Shell. Check the copied icon launches the correct binary.
Artwork needs graphics.library 39+ and optional datatypes.library 43+; MUI
3.8 and the previous player requirements still apply.

To downgrade a migrated drawer, restore its previous launcher and original
configuration (`Library.pre-browser` or the pre-trial copy). Old launchers
do not understand the v2 library configuration. ROM and state files require
no conversion.

## Structure and verification

Shared code is in `amiga/fceux_player/ui_model.h`, `library_mui.h`,
`library_preview.h` and `library_saves.h`. Each application has a small
`launcher.c` adapter and its own cartridge rules. The SNES staging script
copies these same sources; there are no independently drifting layouts.

```sh
python3 scripts/test_game_libraries.py
python3 scripts/build_game_libraries.py --preview
python3 scripts/test_game_snapshots.py
python3 scripts/build_game_snapshots.py
```

The standalone build produces `.context/amiga/game-libraries/ZZNesLibrary/`
and `ZZSnesLibrary/`, with Amiga Hunk executables and compiler logs; the root
`build.json` records source hashes and compiler image. It never rebuilds ARM,
installs software or contacts the Amiga. Full existing emulator build scripts
also use the shared source.

Offline validation: both launchers cross-compile with warnings treated as
errors. Native ASan/UBSan checks cover v1 migration, v2 round-trip, hostile
paths, search/filter ordering, limits, history rollover, save-envelope
corruption/truncation/ROM mismatch and newest-file ordering. Optional existing
local save fixtures are read only; they are neither shipped nor uploaded.

**Hardware verification:** staged separately in `SD032G:Dev/ZZNes-Snapshot/`
and `SD032G:Dev/ZZSnes-Snapshot/`; stable application drawers remain intact.
Mario Bros and Super Mario World each completed a 320-frame run, wrote a valid
library image, restored ARM registers/cache/MMU, released Core1 and returned to
Workbench with the bridge still responsive. The NES automatic image showed its
title; the SNES image caught a title fade, illustrating why F9 is provided.
Manual P/F9/P/Q capture also passed on both, with the previous picture
preserved byte-for-byte as .bak. Both real MUI Play/F9/Q/return paths passed,
including the generated session file. The Games pages display the captured
titles, and the SNES save gallery displays one existing saved thumbnail plus
four empty slots. Exact hashes/logs are in
`amiga/records/2026-10-11/game-snapshots/`.

**Pending broader validation:** optional picture datatypes, physical joystick
navigation, every UI control, save/resume across a wider ROM library, and user
acceptance of the layout. These remain development previews, not releases.

## Supplied Nintendo border artwork (11 October 2026)

A separate matching player/launcher preview adds **Nintendo artwork** and
**Super Nintendo artwork** under Display → Border / wallpaper in both libraries.
The user's two reference files contain game screenshots, so only the outer
Nintendo side panels are compiled into the players. See
[asset provenance and conversion](../amiga/borders/README.md). They do not add
scanlines or a curved-screen shader. Existing 4:3 game geometry and independent
edge masks remain intact; covered game edges are black for these themes.
Widescreen mode exposes the full side panels. NES 4:3 fills its display and
therefore has no external side panels to decorate.

Unlike the earlier snapshot-only preview, these builds **replace the ARM
presenter object** and its bounded artwork tables as well as the 68k player.
Other emulator objects, state ABI, framebuffer ownership/cache policy, XACP
flip scheduling and audio are unchanged. SNES retains its accepted LUT/STMIA
presenter; it does not incorporate the unaccepted flip experiments. NES caches
converted artwork panels to avoid per-pixel division/conversion during play.
No firmware update or runtime PNG decoding is needed.

Build with `make amiga-border-assets` (Pillow required), then
`python3 scripts/build_game_borders.py`. The latter uses existing verified
release-ui NES and LUT/STMIA SNES object caches, relinks within the original
payload bounds and builds matching Hunk executables plus MUI launchers under
`.context/amiga/game-borders/{nes,snes}/`. It performs no installation itself.
Themes 5/6 require those matching players; older players reject the new IDs.
Existing theme IDs and defaults in the source are preserved.

The hardware preview drawers are `SD032G:Dev/ZZNes-Borders` and
`SD032G:Dev/ZZSnes-Borders`. Open the corresponding Library icon in each drawer.
They contain copies of the previous preview's history/preferences/states and
snapshots; ROM paths refer to existing files. Only these new drawers default to
widescreen and the matching Nintendo artwork. Their saves are independent of
the original applications. Reopen the original drawer to roll back.

Validation: native ASan/UBSan checks pass for the shared model, state gallery,
snapshot/session parsing and all valid renderer options in both pixel formats.
Both new artwork themes passed 120-frame hardware runs in each emulator, with
first-frame pixel readback, complete flip counts and clean ARM/Workbench return.
The SNES MUI launcher opens successfully. Records are under
`amiga/records/2026-10-11/game-borders/`. Appearance and tearing acceptance still
need owner review; these checks do not establish a speed gain or tear-free output.

## Visual joystick tester (11 October 2026)

The Controllers tab now follows the owner's controller-configuration reference:
D-pad inside an outline on the left, Select/Start in the centre, NES B/A or the
SNES four-button diamond with L/R on the right. Held joystick controls highlight
green. The physical port, detected device type and raw flags remain visible.
The primary-fire selector immediately updates both the labels and highlights;
Save settings persists the same mapping used by the players.

This is a live physical-joystick indicator. Keyboard labels describe in-game
keys and do not indicate key presses in the library. A standard C64 joystick
provides directions and Fire; a second button requires suitable hardware.
Gamepad Green/Play map to Select/Start. The SNES A/X/L/R controls remain keyboard
only with the current player mapping. Mouse/unknown port types are not decoded
as joystick input. Arbitrary rebinding, four key sets and turbo controls shown
in the reference are not implemented by this visual update.

Implementation: `amiga/fceux_player/library_controller.h`, shared by both
launchers. Native MUI drawing uses no raster artwork, ARM execution or new
player protocol. It polls at the existing 50 ms interval and redraws the diagram
when the decoded input or mapping changes. The game hides the library and retains
exclusive input polling while playing. Custom drawing resources are released
on MUI cleanup; the previous launcher is kept as `.pre-controller` in each
Borders preview drawer. Player binaries, saves and configuration format remain
unchanged.

`python3 tests/amiga/fceux_player/check_controller.py` compares all 8,192
input/type/mapping combinations against the actual generated player input code
under ASan/UBSan. `python3 scripts/build_game_libraries.py --preview` builds the
matching native launchers. Both are installed in the Borders preview drawers.
Hardware GUI layout and standard-joystick detection are checked; a human still
needs to move the physical stick and confirm each highlighted direction/Fire.

## PAL and Contra III update

The ZZSNES region build supports automatic PAL/NTSC selection and a bounded
Contra III header exception. See [validation and compatibility limits](amiga-zzsnes-pal-compatibility.md),
including the two first-frame save-replay discrepancies. Install its matching
player and library; the older cached player remains NTSC-only.

## About tab — 12 October 2026

Both libraries include an About tab with separate credits for the Amiga port,
the active emulator core, XANI/XACP and MUI (Magic User Interface). The accompanying xSysInfo licence notice is retained for adapted code. ZZNES uses FCEUX; ZZSNES uses Snes9x2002/PocketSNES. Core licence
terms are shown independently from the interface licence. Full attribution
and pinned source revisions are in `amiga/fceux_player/Credits.txt`, also
installed in both emulator drawers with the licence notices.

`python3 scripts/build_game_about.py` rebuilds only the MUI interfaces from the
verified NES compatibility and SNES region header caches. Both installed tabs
were visually verified on the A4000TX; players, library settings and user saves
were preserved. Old libraries have `.pre-about-20261012` backups.
