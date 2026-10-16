# ZZSnes MUI library (0.2.0 development build)

The new **0.3.0-ui-preview launcher** shares ZZNES's searchable game-library
layout, local artwork, Play/Continue actions and five-card save gallery with
actual SNES thumbnails. Its matching 68k host adds local library screenshots;
the existing ARM image and save ABI are unchanged.
See [shared UI design, build, migration and validation](../../../docs/amiga-game-library-ui.md).
Build both launchers with `python3 scripts/build_game_libraries.py`.
A separate preview is installed at `SD032G:Dev/ZZSnes-Snapshot/`. Automatic
capture and cleanup passed on the A4000TX. F9 replaces the library picture
with the current game/title screen, including while paused; no download or
save state is needed. The sections below describe the existing player and
prior library behaviour.

A separate Workbench launcher for the PocketSNES/Snes9x2002 Cortex-A9 player.
Install the drawer as `SD032G:Dev/ZZSnesLibrary` and double-click
`ZZSnesLibrary`. It leaves the earlier Mesen and SMW-only previews available.

## Frame-rate preview — 11 October 2026

`SD032G:Dev/ZZSnes-Snapshot` now has an experimental faster ARM presenter:
RGB565 lookup plus ARM multiword framebuffer stores, with capped host lookahead
and a six-frame audio prebuffer. A matched 1,800-frame SMW test measured
**55.89 FPS**, compared with **53.57 / 54.12 FPS** baseline runs (about +3.8%).
ARM presentation fell from 3.31 to 2.94 ms. Every frame and audio sample was
retained; sustained 60 FPS and audible continuity remain unproven. The regular
installation is unchanged; the previous preview player is `ZZSnes-ui.pre-fps`.
See [measurements, tests, build and rollback](../../records/2026-10-11/pocketsnes-fps/).

## Library and controls

Add individual `.sfc` / `.smc` files or scan a folder; filter by recent games or
favourites. Play starts the selected cartridge. The Display tab selects a 4:3
or widescreen monitor, black/white surrounds or one of three original console
bezels: SNES purple, Super Famicom four-colour and charcoal. Optional 8/16-pixel
masks conceal each pair of edges. The displayed game keeps a physical 4:3
aspect on the selected monitor type.

Controllers includes a live physical-port-2 joystick test and fire-button
mapping. Keyboard: arrows move, Z=B, X=Y, A=A, S=X, C/V=L/R, Enter=Start,
left Shift=Select. P pauses, M mutes, Q/Escape returns to the library.

## Save states and thumbnails

Select one of five slots in Save slots before playing. **F5 saves; F8 restores**
the selected slot, including while paused. **Resume slot** starts a fresh player
process with that saved state. The panel shows each slot's date/size and a
128x112 thumbnail of the selected slot. The timestamp comes from the Amiga clock.

States are stored in `States/<two ROM-content hashes>-<slot>.zzs`; names are
independent of ROM filenames and normalize a 512-byte copier header. Different
ROM content gets different slots. Each bounded, checksummed file includes its
thumbnail, emulated RAM/SRAM, CPU/PPU/SPC/DSP state, the current frame and missing
upstream mixer/render state. Runtime pointers are rebuilt when loading.

Writes go to a temporary file, are reread and verified, then replace the slot;
the previous save is retained as `.bak`. These are this player's version-2
states, not interchangeable with other emulators or guaranteed future versions.
Independent battery `.srm` persistence remains disabled; save states include
SRAM. Do not publish commercial ROMs or their save-state files in source backups.

## Current compatibility

Conservative gate: standard NTSC Japan/USA LoROM/HiROM up to 4 MiB; cartridge
types 0–2, no enhancement chips. Both plain and 512-byte-header ROMs are accepted.
Only 256x224 output is supported. PAL, interlace/high-resolution modes and broad
game compatibility are not established. SMW and an original checkerboard ROM
are the validation cases. No commercial ROM is included in builds.

## Architecture and attribution

Native 68k MUI handles files, controls, AHI and lifecycle; Core1 runs the SNES
core, builds RGB565 frames and presents directly to locked ZZ9000 bitmaps.
Xanxi's XACP fullscreen buffer-flipping approach is retained. Audio uses the
existing AHI unit 0 setting (Paula on this machine); no driver settings change.

The 32 MiB allocation remains Exec-owned with runtime mapping verification,
bounded mailbox/ROM/state areas, temporary framebuffer mappings, cache/MMU
restore and the single-Core1 owner guard. The MUI launcher owns no ARM resources.

Core: [libretro/snes9x2002](https://github.com/libretro/snes9x2002), pinned in the
parent build, under its retained non-commercial Snes9x terms. UI structure:
[xSysInfo](https://github.com/SkiltonUSA/xSysInfo), BSD-2-Clause (see
`LICENSE-xSysInfo`), and our ZZNES launcher. XACP:
[Xanxi-Amiga/XACP-ZZ9000](https://github.com/Xanxi-Amiga/XACP-ZZ9000).
The original console bezels are procedural graphics. Optional themes 5/6 use
user-supplied Nintendo side panels; see [provenance](../../borders/README.md).

Build: `python3 scripts/build_pocketsnes_library.py`; output goes to the ignored
`.context/amiga/pocketsnes-library` directory. The build does not deploy.
Native sanitizer tests: `python3 scripts/test_pocketsnes_library_native.py`.
ARM model: `tests/amiga/pocketsnes/check_library_arm.py` with Unicorn installed.
Both tests use the private local SMW input plus an original generated fixture;
they do not download a commercial cartridge. Hardware results are recorded in
`amiga/records/2026-10-10/pocketsnes-library/`.
