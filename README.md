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

Measured **12 October 2026**, TF4060 / ZZ9000, XX19c / XACP 1.7, with AHI audio.
**Emulation FPS** counts completed CPU/PPU/APU frames. **Presented pictures/s**
counts pictures submitted through the display pipeline; it is not monitor
refresh or a measurement of tear-free scanout.

| Emulator / workload | Before: emulation FPS | Beta scheduling: emulation FPS | Before: presented pictures/s | Beta scheduling: presented pictures/s | Measurement window |
| --- | ---: | ---: | ---: | ---: | --- |
| ZZNES — Contra, NTSC | 55.908 | **59.893** | 55.908 | **58.395** | Frames 600–1,200 of a 1,200-frame run |
| ZZSNES — Super Mario World, NTSC | 57.758 | **59.920** | 57.758 | **58.053** | Frames 600–18,000 of an 18,000-frame run |
| ZZNES — Asterix, PAL | 49.920 | **50.005** | 49.837 | **50.005** | Frames 600–1,200 of a 1,200-frame run |
| ZZSNES — Cannon Fodder, PAL | 49.735 | **49.743** | 49.735 | **49.743** | Whole 1,200-frame loop, including startup |

Steady rows use differences between logged progress samples. PAL SNES has no
comparable steady sample window in its record; its whole-loop figure is shown
explicitly. These are per-title measurements, not universal emulator speeds.

For comparison, the **whole-loop NTSC** rates including startup were:

| Workload | Emulation FPS, before → after | Presented pictures/s, before → after |
| --- | --- | --- |
| Contra, 1,200 frames | 53.652 → **57.570** | 53.652 → **54.980** |
| Super Mario World, 18,000 frames | 57.720 → **59.856** | 57.720 → **57.991** |

## Where the improvement came from

The latest change is **68k host scheduling and regional frame pacing**. It keeps
every emulation/APU frame and every PCM sample, but may omit a late completed
picture, with at most two consecutive omissions. It fixes the zero-deadline
reset after pause, save/load or thumbnail capture. The ARM payload, AHI worker
and each emulator’s accepted fullscreen handoff were unchanged in these A/B
comparisons. This gain does not come from SDL2, new NEON code or new firmware.

| Whole-run metric | ZZNES Contra: before → after | ZZSNES SMW: before → after |
| --- | --- | --- |
| Pictures omitted | 0 → 54 / 1,200 (4.50%) | 0 → 561 / 18,000 (3.12%) |
| Audio scheduling gap indicators | 254 → 15 | 1,005 → 0 |
| Completed audio | 638,901 mono samples in each run | 9,624,392 stereo sample pairs in each run |

Gap counters are scheduling indicators, not recordings of audible dropouts.
Some long display handoffs remain; these results do not prove perfect audio,
latency or elimination of horizontal tearing. The user reported the SNES
interactive candidate was “so much better”; NES visual acceptance remains
separate from its measured improvement.

[ZZNES logs and analysis](amiga/records/2026-10-12/zznes-cadence/) ·
[ZZSNES logs and analysis](amiga/records/2026-10-12/zzsnes-cadence/) ·
[Beta install/lifecycle evidence](amiga/records/2026-10-12/emulator-beta/)

Timing comes from the diagnostic A/B hosts whose scheduling policy is now in
the Betas. The versioned Beta binaries subsequently passed launch/save/resume
lifecycle checks; the five-minute FPS test was not rerun on the renamed Beta.

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
