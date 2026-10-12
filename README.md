# ZZSNES — 0.4.0 Beta

A native MUI emulator library for **AmigaOS / ZZ9000 Cortex-A9**, using
**PocketSNES / Snes9x2002** and **[XANI’s XACP platform](https://github.com/Xanxi-Amiga/XACP-ZZ9000)**.
The ARM runs emulation and renders into ZZ9000 display buffers; the 68k handles
AmigaOS, ROM files, input, display coordination and AHI audio delivery.

**[Current Beta executables](artifacts/beta-0.4.0/)** ·
[Build and backup](docs/beta-backup.md) ·
[Library guide](docs/amiga-game-library-ui.md) ·
[Compatibility and timing details](docs/amiga-zzsnes-pal-compatibility.md)

This backup contains the exact installed Beta executables, editable source,
verified build inputs, ARM payload and hardware test records. ROMs, personal
library settings, save files, AmigaOS and firmware are not included. Older
GitHub releases and build targets remain historical; they do not contain this
Beta’s complete feature set.

## Frame rates on the A4000TX

| Game / emulator | Steady emulation FPS |
| --- | ---: |
| Contra — ZZNES | **59.893** |
| Super Mario World — ZZSNES | **59.920** |

[Benchmark details](docs/amiga-zznes-pal-compatibility.md) ·
[SNES benchmark details](docs/amiga-zzsnes-pal-compatibility.md)

## Workbench library

- Games, Display, Controllers, Save states and About tabs.
- ROM/folder import, search, recent history, favourites and local game snapshots.
- Joystick test and fire mapping, 4:3/widescreen correction, borders and overscan masks.
- Five save slots **per game**, save deletion and continue/resume selection.
- F5 saves, F8 loads, F9 captures library artwork; P pauses, Q/Escape returns.

SNES save slots contain miniature screenshots. Existing NES state files do
not contain embedded screenshots; its game library uses separate captures.

On the owner’s Amiga, open **`SD032G:Emulator/ZZSNES/ZZSNES`**.
Keep `ZZSnes-ui` in the same writable drawer. The window/About page identifies
**0.4.0 Beta**. ROMs, States and Snapshots live alongside the application.
For a fresh installation, copy the current artifact drawer to a writable
location and add your own ROMs through the library. Use short ROM filenames
(at most 20 characters on short-name filesystems) to leave space for `.sav.bak`
and `.sav.tmp`; display titles may remain long.

Requires the verified AmigaOS 3.2.3 / Picasso96 / MUI / AHI setup and compatible
XX19c / XACP 1.7 firmware. Only one Core1 application may run at a time.
Other firmware combinations are not established by these tests. No global AHI
or firmware changes are included. Direct ARM-to-ZZ9000AX audio is not implemented.

## Compatibility and validation

Automatic PAL/NTSC handling is implemented for supported standard LoROM/HiROM
cartridges; high-resolution/interlaced output and enhancement-chip support are
not established. See the compatibility guide for Contra III and cartridge
restrictions. Current audio is 32,040 Hz stereo; the accepted four-slot C0
fullscreen handoff is retained.

The Beta promotes the timed cadence experiment into normal unlimited play,
with real input, SRAM, states and snapshots enabled. It passed a 627-frame
interactive lifecycle with paused save/load and snapshots, then a 120-frame
fresh-process resume. Standalone battery files use `.sav`; save states also
include SRAM.

The recorded physical runs completed audio delivery, display acknowledgements,
register/MMU/cache-witness restoration and owned-memory release without reported
errors. Native/instruction-model tests do not substitute for these hardware checks.

## Build, backup and credits

Run `make verify-beta` to verify archived hashes and recalculate the frame-rate
results from raw logs. `make beta` rebuilds the 68k player and MUI library from
archived inputs, retaining the exact tested ARM payload. See
[backup/build scope](docs/beta-backup.md) for the editable source snapshot and
historical core rebuild paths. Neither command launches the Amiga.

Development: **Codex assisted, Amiga-Bridge MCP**. Interface: **MUI**.
Thanks to **XANI / Xanxi** for XACP and to the FCEUX/FCE Ultra and
Snes9x/PocketSNES developers for their cores. Component licences and original
attributions remain with their sources and executable packages.

[Earlier README and measurements](docs/history/readme-before-beta-20261012.md).
