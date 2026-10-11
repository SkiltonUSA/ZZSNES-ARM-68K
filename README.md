# ZZSNES-ARM-68K

SNES emulation research for an **Amiga 4000TX with a ZZ9000 Cortex-A9**, using
**XANI's XACP firmware interface** to launch and coordinate ARM work from 68k
AmigaOS. This development backup contains the Mesen2 and PocketSNES-family
experiments, sources, build scripts, test evidence and exact saved executables.

The current milestone is a verified **headless core benchmark**. External game
loading, fullscreen gameplay, audible output, save states and controller input
are future work. The Mesen branch has a separate native MUI diagnostic window.

## Real-hardware result

Measured on the owner's A4000TX / TF4060 / ZZ9000, XX19c firmware and XACP 1.7,
on 10 October 2026. Both cores run our original 32 KiB SNES checkerboard fixture
with CPU, memory/bus, DMA, PPU, SPC700 and DSP work enabled.

| Complete-system startup diagnostic | Mean time per frame | Scope |
| --- | ---: | --- |
| Mesen2, earlier scalar-word-memory build | 39.945 ms | Previous verified comparison baseline |
| **PocketSNES / Snes9x2002, portable C** | **4.6166 ms** | Ten six-frame batches after one warm-up |

PocketSNES measured **4.6047–4.6297 ms/frame**, about **8.65× the fixture
throughput** of the Mesen baseline. It leaves encouraging headroom within a
16.67 ms / 60 Hz frame budget. **This is not demonstrated game or display FPS.**

The tested PocketSNES source is [libretro/snes9x2002](https://github.com/libretro/snes9x2002),
commit `6ffbf9ef4f0063e1f1b78a40d10c50fc52f2524c`, formerly called PocketSNES.
It uses the portable C CPU/renderer/SPC path compiled in ARM mode, without
frame skipping, idle shutdown, game speed patches, VFP or NEON. It is **not**
a test of DrPocketSNES's assembly CPU or the older ARMSNES handheld renderer.

Important comparison limits:

- This fixture draws a simple Mode 0 checkerboard; DSP output is silence.
  Active voices, sprites, other video modes and real games remain untested.
- PocketSNES returns cropped 256x224 RGB565 and 3,208 stereo samples at
  32,040 Hz. Mesen returns 256x239 SNES-colour pixels and 3,119 samples.
  Startup frame boundaries and audio rates differ, so the ratio does not
  represent identical cycle-accurate work or equal compatibility.
- Timing excludes initialisation, ROM loading, shutdown and video hashing.
  Silent PCM hashing is inside the audio callback. Display and audio-device
  transfers are absent.

[Detailed method and results](amiga/pocketsnes/README.md) ·
[Raw physical log](amiga/records/2026-10-10/pocketsnes/benchmark.log) ·
[Timing summary](amiga/records/2026-10-10/pocketsnes/results.json) ·
[Mesen development history](amiga/mesen_snes/README.md)

## Verification

- Native AddressSanitizer/UndefinedBehaviorSanitizer: repeated passes.
- Relocated Cortex-A9 instruction-model execution: two bases, strict alignment,
  corrupt-palette rejection, cancellation and synthetic timer checks passed.
- Physical video `67711dc5`, silent PCM `77d82d05`, 3,208 sample pairs matched
  the reference; the SPC700 AA/BB boot-ROM handshake also passed.
- Active cancellation completed in 134 ms. RET1, all 14 restored registers,
  MMU/cache witness and release of owned RAM passed. No crash/reboot observed.
- Workbench, Roadie and Amiga-Bridge stayed alive; existing NES and Mesen
  executable hashes were unchanged.

The initial Thumb experiment failed instruction-model validation and was never
run on the Amiga. The saved executable is the verified ARM-mode build.

## Saved executable

[Download ZZSnes-Pocket-Bench](artifacts/pocketsnes/ZZSnes-Pocket-Bench)
(542,084 bytes, MD5 `78a67d1434e85d6b304916333514e262`). Copy it to an Amiga
directory, close all other Core1 applications, then run:

```text
Protect ZZSnes-Pocket-Bench +e
Stack 65536
ZZSnes-Pocket-Bench >RAM:PocketSNES-test.log
Type RAM:PocketSNES-test.log
```

It opens no game screen. The original installation is
`SD032G:Dev/ZZSnes-Pocket/ZZSnes-Pocket-Bench`. Saved Mesen CPU, MUI and
scalar-word system executables are also under [artifacts/](artifacts/).
Checksums for every saved artifact are in [SHA256.json](artifacts/SHA256.json).
These are diagnostic snapshots, not a finished emulator release.

## XACP and memory ownership

Thanks to **XANI / Xanxi** for [XACP-ZZ9000](https://github.com/Xanxi-Amiga/XACP-ZZ9000),
the firmware documentation, application examples and assistance that make this
work possible. The shared launcher comes from our
[Amiga-MCP-Debugger](https://github.com/SkiltonUSA/Amiga-MCP-Debugger) work.

The 68k reserves an actual 24 MiB Exec-owned allocation and probes its ARM
mapping before launch. ARM runs the core using private MMU/L1 mappings; the
mailbox and outer cache are noncacheable. The host enforces bounded requests,
single-Core1 ownership and restoration/reset before freeing memory. No fixed
DDR address is assumed available. This benchmark does not change firmware,
global audio configuration or system startup.

## Build, provenance and next work

[Next-step roadmap](docs/roadmap.md) · [Build instructions](docs/build.md) · [Agent handoff](docs/session-memory.md) ·
[Third-party notices](THIRD_PARTY.md) · [Backup scope](docs/backup.md)

Sources for both cores are pinned and retained with their notices. The backup
contains only the original synthetic SNES cartridge, not commercial games,
AmigaOS installations, firmware images or a disk backup.

Next milestones: a real-game compatibility test with active audio, then the
proven XACP fullscreen-buffer path, 68k input and AHI delivery. Measure the
complete pipeline before claiming playable 50/60 Hz. The NES application
remains a separate project: [ZZNes-ARM-68K](https://github.com/SkiltonUSA/ZZNes-ARM-68K).
