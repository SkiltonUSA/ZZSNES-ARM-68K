# FCEUX on ZZ9000: headless core experiment

This is a bounded CPU/PPU/APU benchmark, not a replacement ZZNes player.
It runs the [Rezzy-dev FCEUX fork](https://github.com/Rezzy-dev/fceux) at commit
`4d01530b6689b06a6d7cf0679b63625e3270ae9a` on ZZ9000 Core1. The original fork
is a Dingux/Miyoo Linux application. Its selected emulation sources are vendored
unchanged with SHA-256 hashes and upstream GPL license; platform integration
is generated during the build and recorded in `integration.patch`.

## Verified A4000TX results — 9 October 2026

XX19c, XACP 1.7, ZZ9000 Cortex-A9 Core1, private L1 cache, nominal 666 MHz.
300 no-input startup frames per ROM; actual ARM frame time, including CPU,
PPU, APU and PCM nonzero counting, excludes hashes, IPC and frontend work.

| ROM | Previous Mesen ARM core | FCEUX ARM core | Core-time ratio | Slowest FCEUX frame |
|---|---:|---:|---:|---:|
| Mario Bros, mapper 0 | 28.961 ms | **4.946 ms** | **5.86×** | 5.340 ms |
| Super Mario World, mapper 90 | 31.141 ms | **6.104 ms** | **5.10×** | 6.599 ms |

FCEUX's complete headless test, including hashes, polling and logging, processed
**146.63 frames/s** and **125.86 frames/s** respectively. These are unpaced
benchmark rates, not displayed FPS or real-time audio playback. The prior Mesen
full-player rates were 23.754 and 22.584 displayed FPS; comparing those directly
with headless FCEUX would overstate the gain. The table compares each core's
measured computation time inside its own harness.

All 600 hardware frames matched native FCEUX exactly: CPU cycles, raw pixels
and emphasis, PCM hashes, sample counts and nonzero counts. Mario produced
159,725 samples (132,935 nonzero); mapper 90 produced 159,724 (156,971 nonzero).
Every measured FCEUX core frame was below 16.639 ms. Short launch plus both
300-frame runs passed mapping, RET1, MMU/register/witness checks and released
owned memory. These are separate native-equivalence and hardware-return
checks, not an independent proof of NES accuracy or whole-game compatibility.

**Conclusion:** this is a strong candidate for a 60 Hz player. Next integrate
FCEUX with the existing Xanxi-style fullscreen buffer flipping, controls and
AHI frontend, then measure displayed frames and listen for gaps on hardware.
No NEON, SDL or firmware performance change produced these results: the major
change is the lighter FCEUX CPU/PPU/APU model and settings. Its compatibility
and sound-quality tradeoffs need owner testing before promotion.

Installed separately: `SD032G:Dev/FceuxBench/ZZFceuxBench`, 186,344 bytes,
MD5 `b3562f5e0f3ec45ddb84cb7a11c1ddc4`. Existing ZZNes 0.3.2-dev remains
unchanged, MD5 `d54d086434df49e7f5ece528316a6f29`.
Left Workbench idle; ROADIE and AMIGABRIDGE alive, no Core1 owner.
Evidence: [hardware logs and build provenance](../records/2026-10-09/fceux-bench/).

## Scope

- NTSC iNES mapper 0 or 90, PRG/CHR ROM, no battery/trainer/four-screen/NES2.
- CPU, old PPU, APU, mapper IRQs and every frame rendered; no frameskip.
- Fixed 32 kHz mono, upstream sound quality 0, eight-sprite hardware limit.
- Zero controller input, deterministic alternating 55/AA initial CPU RAM.
- Pixel indices and emphasis hashed separately from PCM; no RGB conversion,
  screen presentation, AHI playback, battery saves, debugger or menus.
- Tests are startup workloads, not whole-game compatibility or audio acceptance.

FCEUX's old PPU and quality-0 audio trade accuracy for speed differently from
Mesen. Treat measurements as a candidate-core comparison, not an optimization
of the same emulation model. Headless throughput is not displayed FPS.

## Build and checks

```sh
python3 scripts/build_fceux_bench.py --host \
  --container-command '["podman","--connection","nuflix-converter-root"]'
python3 -m unittest discover -s tests/amiga/fceux -v
python3 tests/amiga/fceux/check_native.py /private/path/game.nes 300
.context/amiga/mesen-bench/venv/bin/python tests/amiga/fceux/check_arm.py /private/path/game.nes 120
make test-amiga-arm test-amiga
```

Outputs: `.context/amiga/fceux-bench/`. Clang targets Cortex-A9 ARM state,
soft-float, O3, explicit signed wrapping, no unaligned accesses. No VFP/NEON
or Linux/SDL dependencies. Fixed NTSC/32k initialization tables remove otherwise
unused soft-float runtime dependencies; all frame/cycle/PCM hashes must match
the original floating initialization. Native memory tests enable ASan and
UBSan, except array-bounds and pointer-overflow: upstream bank maps intentionally
form biased pointers before buffers. Actual accesses remain ASan-instrumented.
This is not a claim of fully defined ISO C++ pointer arithmetic.

The PPU's word casts over byte-aligned buffers use an alignment-1, may-alias
word type, and the adapter's repeated-word fill is byte-safe. ARM instruction
model tests reject unaligned multi-byte access and compare every tested frame
with native output. These tests do not establish physical cache coherence.

## XACP and memory contract

Uses our existing XX19c/XACP 1.7 runtime mapping launcher and Core1-only private
MMU/L1 contract, built on [Xanxi's XACP platform](https://github.com/Xanxi-Amiga/XACP-ZZ9000).
No firmware modification, global L2 maintenance or second Core1 app.

All offsets below are relative to an actual 4 MiB Exec-owned allocation,
verified by the launcher's runtime mapping probe; they are not SDK allocations:

| Region | Relative range |
|---|---|
| ARM code/data/relocations | below `0x40000` |
| Private translation tables | `0x40000..0x50000` |
| Cache witness | `0x60000` |
| Uncached mailbox/MMU reports | `0x80000..0x84000` |
| Cartridge | `0x100000..0x200010` maximum |
| Zero-initialized core BSS | `0x220000`, bounded below `0x3d0000` |
| Stack reserve | above `0x3d0000`, top rounded down to owned page |

The benchmark-only ELF packer permits relocation targets only in loaded code/
data or its bounded BSS, rejecting targets in tables, mailboxes, ROM, stack
or firmware. The launcher zeros the entire owned allocation before copying
and relocating the image. Existing RET1, register restoration, witness checks
and synchronous Core1 reset run before memory is released.

## Amiga invocation

Only after checking Core1 is idle:

```text
Stack 65536
SD032G:Dev/FceuxBench/ZZFceuxBench 9fc00101 RUN "SD032G:Dev/ZZNesPlayer/ROMs/011 Mario Bros.nes" 300 >SD032G:Dev/FceuxBench/mario300.log
```

Use a fresh nonzero hexadecimal nonce. Frame limit is 1..3600. Ctrl-C requests
shutdown; mailbox requests time out and return through the existing launcher
cleanup. A frame is timed around CPU/PPU/APU processing, including the PCM
nonzero count, before hash validation and host IPC. Clock is calibrated against
Amiga timer.device. Host elapsed time also contains mailbox polling and logging.

No commercial ROM is included in source, evidence or distribution.
