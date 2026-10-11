# ZZSNES-ARM-68K

A Workbench SNES emulator for the **Amiga 4000TX / ZZ9000 Cortex-A9**, with a
native **MUI ROM library**, stereo AHI output and illustrated save slots.
The 68k manages AmigaOS, files, input, audio and display lifecycle; the ARM runs
PocketSNES/Snes9x2002 and writes directly into ZZ9000 display buffers through
**XANI's XACP interface**.

**[Download v0.2.0-dev](https://github.com/SkiltonUSA/ZZSNES-ARM-68K/releases/tag/v0.2.0-dev)** ·
[Installation](docs/install-library.txt) · [Controls and limits](amiga/pocketsnes/library/README.md)

This is a private experimental development prerelease. The ROM-free ZIP
contains the exact tested 68k executables, Workbench icons, an empty library,
documentation and license notices. Source, ARM payloads, build support and test
evidence are retained here. It does not contain AmigaOS or firmware.

## Current application

- Add ROMs or a folder; browse favourites and recently played games.
- Five save slots per ROM, each with a captured **128x112 screenshot**.
- **F5 saves**, **F8 loads**, and **Resume slot** restores in a new player session.
  Previous saves are retained as `.bak`; states are identified by ROM contents.
- Controller test and fire-button mapping, keyboard and physical-port-2 joystick.
- 4:3 / widescreen monitor settings, edge masks, black/white surrounds and original
  SNES purple, Super Famicom four-colour and charcoal console bezels.
- Fullscreen XACP buffer flipping and clean return to the MUI library.

Install in a writable drawer and double-click `ZZSnesLibrary`. Keep its
`ZZSnes-ui` player beside it. On the development Amiga it is installed at
`SD032G:Dev/ZZSnesLibrary/`. Existing Mesen and SMW-only previews remain separate.

Verified setup: A4000TX, TF4060, AmigaOS 3.2.3, ZZ9000 **XX19c / XACP 1.7**,
Picasso96, MUI and AHI, with a free 32 MiB ZZ9000 Fast RAM allocation. Other
firmware/hardware combinations are unverified. Close other Core1 applications
before launching. No global audio or firmware configuration changes are made.

## Measured results

Physical measurements on 10 October 2026; core-only and displayed rates have
different scopes and must not be compared as equivalent game FPS.

| Test | Measured result | Scope |
| --- | ---: | --- |
| Mesen2 scalar-word diagnostic | 39.945 ms/frame | Earlier headless original checkerboard |
| PocketSNES original checkerboard | 4.6166 ms/frame | Ten six-frame headless batches |
| SMW ARM core | 5.017 ms/frame | 600 startup/menu frames; excludes display/audio playback |
| Plain fullscreen preview | 54.933 displayed FPS | 600 scripted-Start frames, stereo AHI |
| **MUI player with scaled console bezel** | **52.447 displayed FPS** | 600 scripted-Start frames, stereo AHI |

The final bezel renderer averages **3.320 ms/frame**, down from about 11.2 ms
in the initial uncached implementation. It caches scaling coordinates and
static artwork. The plain preview used 2.272 ms/frame for presentation; the
new bezel adds scaling work. These tests do not establish sustained 60 FPS.

The final bordered run completed all 600 flips and 320,813 stereo sample pairs,
with zero display faults or AHI device errors. It recorded **83 late-link
scheduling events**: audible continuity still needs refinement. AHI unit 0 on
the test Amiga is currently Paula; direct ARM/ZZ9000AX audio is not implemented.

[Library results and logs](amiga/records/2026-10-10/pocketsnes-library/) ·
[Plain player](amiga/records/2026-10-10/pocketsnes-player/) ·
[SMW core](amiga/records/2026-10-10/pocketsnes-smw-arm/) ·
[Original diagnostic](amiga/pocketsnes/README.md)

## Save-state and hardware verification

SMW and the original 32 KiB test cartridge have independent saves and visibly
correct thumbnails. Hardware tests pass save, paused load, fresh-process
resume, overwrite/backup rotation and clean exit. States are 686,196 bytes on
A9, include SRAM and are tied to this player/version; standalone battery `.srm`
files are not yet written.

Native ASan/UBSan and relocated A9 instruction models reproduce the next 30
video frames and stereo PCM exactly after restoration, including loading at
a different ARM allocation address. Additional state covers upstream snapshot
omissions in mixer, echo/filter history, fractional audio timing and rendering.
Runtime pointers are rebuilt. Corrupt-thumbnail and wrong-ROM saves are rejected.

Physical runs confirm RET1, all 14 restored registers, cache/MMU restoration,
zero witness mismatches and owned-memory release. The single-Core1 guard,
runtime mapping probe and temporary framebuffer mappings remain mandatory.
Instruction-model tests alone do not establish physical cache correctness.

## Compatibility and next work

Current gate: standard NTSC Japan/USA LoROM/HiROM, up to 4 MiB, cartridge types
0–2, no enhancement chips, 256x224 video. `.sfc` and `.smc`, with or without a
512-byte copier header, are accepted. PAL, high-resolution/interlaced output,
enhancement chips and broad game compatibility are not established. Only SMW
and an original fixture have been validated, not an entire catalogue.

Next: audio/frame pacing, longer gameplay and joystick tests, battery SRAM
persistence, then more cartridges. See [roadmap](docs/roadmap.md).

## Source, build and acknowledgements

[Build and verification](docs/build.md) · [Handoff](docs/session-memory.md) ·
[Backup scope](docs/backup.md) · [Component notices](THIRD_PARTY.md)

Core: [libretro/snes9x2002](https://github.com/libretro/snes9x2002), pinned to
`6ffbf9ef4f0063e1f1b78a40d10c50fc52f2524c`, portable C in ARM mode; no VFP/NEON,
frame skipping or game speed patches. The Mesen2 diagnostic/reference work is
preserved separately. This repository does not relicense imported components.

Thanks to **XANI / Xanxi** for
[XACP-ZZ9000](https://github.com/Xanxi-Amiga/XACP-ZZ9000), its firmware interface,
fullscreen examples and assistance. Shared ARM ownership/MMU tooling comes
from [Amiga-MCP-Debugger](https://github.com/SkiltonUSA/Amiga-MCP-Debugger).
MUI patterns come from [xSysInfo](https://github.com/SkiltonUSA/xSysInfo) and
our [ZZNES application](https://github.com/SkiltonUSA/ZZNes-ARM-68K).
Console bezels are original procedural artwork.
