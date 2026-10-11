# ZZSnes-Mesen

Separate SNES development application for the A4000TX / ZZ9000. Version 0.2.0-dev
runs Mesen2 CPU and **complete-system diagnostics**, not SNES games. The existing
ZZNes/ZZFceux application, ROMs, settings and states remain independent.

## Review and scope

Upstream: [SourMesen/Mesen2](https://github.com/SourMesen/Mesen2), pinned to
`b9fa69ddc6d0a331fb103fdb5eef6904305703c2`, GPL-3.0-or-later. This is the
archived Mesen2 tree requested by the owner; it is different from the older
NES-only Mesen extraction. The current maintained successor is Mesen CE.

The source provides distinct SNES components:

| Component | Upstream implementation | Port status |
| --- | --- | --- |
| 65C816 CPU | `Core/SNES/SnesCpu*` | Extracted, built for A9, original smoke fixture verified on real hardware |
| System timing and DMA/HDMA | `SnesMemoryManager`, `SnesDmaController`, `InternalRegisters` | Integrated in the system test; ordinary DMA verified, HDMA-specific tests pending |
| Graphics | `SnesPpu` | Integrated; Mode 0 original tile fixture verified on A9; other modes/sprites/hires/interlace pending |
| Audio | `Spc` and `DSP/Dsp` | Integrated; IPL boot and silent PCM timing verified; audible fixture/playback pending |
| Cartridges | `BaseCartridge`, handlers and memory mappings | Strict 32 KiB ordinary LoROM adapter; other sizes, HiROM and SRAM rejected |
| Enhancement chips | SA-1, GSU/Super FX, NEC DSP, CX4, S-DD1, SPC7110 and others | Present upstream, not included or claimed by this build |
| Save states | Upstream `Serializer` infrastructure | Future integration; CPU diagnostic removes serializer dependencies |
| Amiga frontend | Native MUI 3.8 | Separate development window with CPU/System buttons and result summaries |

`SnesConsole::RunFrame` drives CPU instructions until the PPU ends a frame,
then processes SPC audio. DMA can cross frame boundaries. Preserve that
scheduling when bringing up the system: replacing all peripherals with no-ops
would produce a misleading fast CPU test rather than an emulator.

The desktop frontend is Avalonia/.NET, not suitable for this Amiga port. The
native MUI frontend follows our xSysInfo-based UI patterns. Future presentation
should use the existing XANI-style XACP fullscreen flipping with ARM conversion
into a verified, OS-owned RTG bitmap. Do not assume the NES fixed 256x240 layout
covers SNES hires/interlace. Audio initially needs the proven host/AHI queue;
no direct ARM-to-ZZ9000AX audio ownership is assumed.

XACP acknowledgement: [XANI's XACP-ZZ9000 platform](https://github.com/Xanxi-Amiga/XACP-ZZ9000).
The current loader uses the verified XX19c/XACP 1.7 launch contract. No firmware
change, newer 1.8 interface or additional static DDR address is introduced.

## First milestone

- Unmodified CPU source files and individual SHA-256 hashes are in `vendor/`.
- The builder generates a reviewable `integration.patch`; opcode/addressing
  algorithms remain unchanged. Desktop headers, serialization inheritance
  and the virtual destructor are removed. Debug/cheat notifications are no-ops.
- An original 60-byte test program switches to native 16-bit mode, sums
  1024..1, exercises a bank-$7e store/load, stack, decimal ADC/SBC, switches
  back to emulation mode, writes a marker and stops. No commercial ROM/BIOS
  or third-party test-vector bytes are included.
- Each run executes 5,144 instructions / 15,436 emulated CPU cycles. Verification
  checks 1,024 arithmetic-loop results, the completed sum and final state/RAM.
  `checked=5144` counts executed verification-run instructions; it is **not**
  a full external instruction-by-instruction reference trace.
- Test memory is a flat 64 KiB bank with an explicit $7e alias. Unmapped banks
  count as errors. This does not implement SNES mapping, bus timing or DMA.
- Cancellation is sampled every 64 instructions. The fixed test has a 6,000
  instruction ceiling. Stress cancellation uses the existing bounded worker.
- The existing runtime mapping probe, Exec allocation, private L1/outer-NC
  cache policy, uncached mailbox, restoration/witness audit and reset-before-
  free contract are reused without modifying the shared launcher/MMU sources.

Native ASan/UBSan and relocated Cortex-A9 instruction-model checks pass,
including a deliberately corrupted program and cancellation. These are smoke
checks, not a complete 65C816 conformance suite.

## Build and test

```sh
python3 scripts/build_mesen_snes.py
python3 scripts/build_mesen_snes.py --arm \
  --container-command '["podman","--connection","nuflix-converter-root"]'
.context/amiga/mesen-bench/venv/bin/python tests/amiga/mesen_snes/check_arm.py
make test-amiga-arm test-amiga
```

The instruction-model checker requires Unicorn 2.1.4. Build products are in
`.context/amiga/mesen-snes/`: `ZZSnes-Mesen` (MUI), `ZZSnes-Mesen-CPU` (68k
launcher with ARM payload), `snes.elf`, native log, patch and build manifest.

## Installed development app

`SD032G:Dev/ZZSnes-Mesen/ZZSnes-Mesen` opens the separate MUI window from its
Workbench icon. Close other ARM games before selecting **CPU test** or **System test**.
The result appears in the window; `Last-CPU.log` retains detailed results.
ROMs/States drawers are reserved for later integration, not currently used.

Shell alternative:

```text
Stack 65536
SD032G:Dev/ZZSnes-Mesen/ZZSnes-Mesen-CPU >RAM:ZZSnes-CPU.log
```

## Physical result, 10 October 2026

The initial A4000TX test passed: native and physical signatures both
`e4b09881`, 5,144 instructions, 15,436 CPU cycles. Ten warm samples took
1,286–1,288 us (mean **1,287.2 us**), excluding initial setup and final hashing.
This arithmetic-heavy CPU microbenchmark says nothing about SNES game FPS.
It must not be compared to complete NES frames or treated as full-speed SNES.

Cancellation during active work, RET1, all 14 saved registers, MMU restoration
and cache witness checks passed; the owned allocation was released. Network
services remained alive. Records: `../records/2026-10-10/mesen-snes/`.

## Complete-system milestone, 10 October 2026

`system/` integrates the actual CPU, memory manager, internal registers, DMA,
PPU, SPC700 and DSP. Forty-one unmodified upstream files are pinned and hashed
in `system/vendor/upstream.json`; generated adaptation is in `integration.patch`.
An original 32 KiB LoROM transfers tile and palette bytes using DMA, then runs
six frames of a red/green checkerboard. The test validates every visible/output
pixel from frame 2 onward, plus CPU/SPC cycles and cumulative PCM samples/hash.
The partial first startup frame is intentionally included in the timing batch.

Native ASan/UBSan checks cover DMA destinations, WRAM mirroring, SPC boot ports,
serial controller shifting, cancellation and invalid-cartridge rejection.
The integer clock conversion needed for bare-metal ARM exactly matches the
floating-point reference across 60 frames (cycles, pixels and sample hashes).
Two relocated A9 instruction-model runs pass, as do a corrupted-palette negative
test and cancellation. The instruction model does not verify physical caches.

The physical A4000TX result matches the reference:

| Check | Six-frame result |
| --- | ---: |
| CPU cycles | 277,071 |
| Master clocks | 2,093,734 |
| SPC cycles | 199,650 |
| Final video hash | `1e1e29c5` |
| Stereo sample pairs | 3,119 |
| Cumulative PCM hash (silence) | `2cfa6c7d` |
| Mean of ten warm batches | 244,300.5 us |
| Mean time per emulated frame | **40.717 ms** |

The arithmetic throughput equivalent is approximately 24.56 frames/s, **not
observed game or display FPS**. This simple startup workload excludes loading,
initialization, final validation, RTG conversion/presentation and audio playback.
It exceeds the approximately 16.6 ms NTSC budget even before those costs.
Full-speed SNES is not established. The old 1.2872 ms CPU microbenchmark is a
different workload and provides no speedup comparison.

Active cancellation, RET1, all 14 restored registers, MMU/cache witness and owned
memory release passed physically. The existing runtime mapping probe and 4 MiB
allocation layout are unchanged. Sparse ROM initialization keeps the payload
below the existing page-table boundary; the checked 0x160000-byte arena is in
owned high BSS. No new static board address or direct audio interface is assumed.

Limitations: Gaussian interpolation only; fixed NTSC configuration; simple
serial controller adapter (physical input and autojoypad interactions untested);
no enhancement chips, serializer, audible test, external game loading or RTG
presentation. The DSP processes silence, which does not validate active voices.
The SPC IPL bytes come from the pinned upstream `Spc.h`; no external BIOS download
is needed. The original CPU-only test remains separate.

Build and validate the system diagnostic:

```sh
python3 scripts/build_mesen_snes_system.py --frames 60
python3 scripts/build_mesen_snes_system.py --frames 60 --integer-clock
cmp .context/amiga/mesen-snes-system/native.log .context/amiga/mesen-snes-system/native-integer.log
python3 scripts/build_mesen_snes_system_arm.py \
  --container-command '["podman","--connection","nuflix-converter-root"]'
.context/amiga/mesen-bench/venv/bin/python tests/amiga/mesen_snes/check_system_arm.py
make test-amiga-arm test-amiga
```

Products are in `.context/amiga/mesen-snes-system/`, including MUI
`ZZSnes-Mesen`, the embedded-payload `ZZSnes-Mesen-System`, original
`System-Test.sfc`, patch, source/output hash manifest and native logs.
The system builder supplies the current UI; the CPU builder supplies the
independent CPU diagnostic. Install both diagnostics beside the UI.

On Amiga, select **System test**; detailed results go to `Last-System.log`.
Or run with `Stack 65536` and redirect `ZZSnes-Mesen-System` to a log.
The previous UI is retained as `ZZSnes-Mesen.before-system`. Original CPU
`build.json` is retained; the new manifest is `system-build.json`.
Evidence: [system records](../records/2026-10-10/mesen-snes-system/).

## Next milestones

1. Profile complete frames by CPU/bus, PPU and SPC/DSP costs. Improve measured
   bottlenecks while preserving pixel/cycle/PCM signatures. The current simple
   fixture is already above the NTSC budget, so performance is the priority.
2. Add a non-silent original SPC program and broader PPU/DMA/controller fixtures.
3. Connect direct RTG display, AHI sound and SNES controls with bounded ownership.
   A single-button C64 joystick needs configurable SNES button mapping.
4. Extend cartridge mappings before introducing external ROM loading. Add
   library/history/favourites and validated states to the separate app.
5. Test enhancement chips independently; upstream support is not port support.

There is no `.sfc`/`.smc` game-loading capability in the current Amiga diagnostic.

## Stage profiling

See [profiling method and validation](system/PROFILING.md).

### Speed-versus-size compiler experiment

Pass `--ppu-opt O2` to build a separate candidate in
`.context/amiga/mesen-snes-profile-o2/`, named `ZZSnes-Mesen-Profile-O2`.
Only the generated PPU translation unit changes from `-Os` to `-O2`; other
translation units, fixture and emulated scheduling remain identical. The strict
payload packer must still accept the original memory boundaries. Run the same
ARM checker on that directory's ELF with `--profile-clock` before hardware use.
Never infer a performance gain from the compiler flag alone.

### Physical profiling results, 10 October 2026

All native signatures and both relocated A9-model checks passed, including
corrupted-palette rejection and active cancellation. Physical profiling passed
all output and teardown checks. Ten warm six-frame batches per configuration:

| Work | Size-optimised PPU (`-Os`) | Speed-optimised PPU (`-O2`) |
| --- | ---: | ---: |
| PPU graphics | 20.326 ms/frame | 20.588 ms/frame |
| SPC700 + DSP | 6.420 ms/frame | 6.241 ms/frame |
| CPU/bus/scheduling + remaining overhead | 14.051 ms/frame | 13.876 ms/frame |
| Instrumented total | **40.797 ms/frame** | **40.705 ms/frame** |

Fresh uninstrumented baseline: **40.698 ms/frame**. The instrumented baseline
is about 0.24% higher; this observed difference includes timer probes and code
layout effects, not a pure measurement of profiler overhead. The -O2 candidate
changes instruction layout and increases the packed image from 232,048 to
236,416 bytes. Its 0.23% total reduction versus the instrumented -Os build is
not a useful improvement; measured graphics time was slightly higher. Neither
candidate was promoted over the installed System test. No frame skipping,
audio muting shortcut or emulation accuracy reduction was introduced.

For this fixture, graphics accounts for 49.82% of the profiled total, audio
15.74% and the CPU/bus/scheduling remainder 34.44%. The remaining ~20.47 ms
outside the graphics scopes already exceeds the approximate NTSC budget.
Thus speeding up graphics alone cannot make this particular measured pipeline
real-time, even in the idealised case of eliminating its measured cost.
This is an inference from this simple silent workload, not a game prediction.

Next: break graphics into tile fetch/rendering, sprite evaluation and colour
composition, then investigate CPU/bus scheduling. Preserve output and cycle
signatures during each change; add active audio and broader fixtures before
claiming compatibility. This pass does not measure Zorro transfer or RTG output.

Both optional diagnostic executables and `ReadMe-profiling.txt` are installed
beside the original app. Evidence: [profiling records](../records/2026-10-10/mesen-snes-profile/).

## NES-style word-memory optimisation, 10 October 2026

The accepted NES work used wider palette/pixel writes and host/audio batching.
The latter does not apply to this headless SNES workload yet. We independently
implemented the applicable scalar word-copy/clear technique in `system/memory.c`:
aligned 32-bit operations in bounded blocks, byte prologues/tails, and byte-copy
fallback when source/destination alignments differ. No VFP/NEON or emulation
algorithm/timing changes are introduced. Memory functions are for normal RAM,
not MMIO; memcpy keeps its standard non-overlap contract.

| Physical comparison | Before | Word-memory candidate |
| --- | ---: | ---: |
| Profiled complete frame | 40.800 ms | 39.737 / 39.764 ms |
| Profiled PPU | 20.324 ms | 19.226 / 19.228 ms |
| Uninstrumented complete frame | **40.701 ms** | **39.945 ms** |
| Installed Workbench System button | — | **39.947 ms** |

Each result is the mean of ten six-frame startup batches after one warmup.
Use the uninstrumented comparison for the deployment claim: approximately
**1.9% less frame time**, a small gain. Do not mix profiled and unprofiled costs
to claim a larger gain; instrumentation and code layout affect results.
This remains a silent, headless fixture, not game FPS or real-time SNES.

Validation: native ASan/UBSan memory tests cover lengths 0..257, 8x8 alignments,
all byte fill values, guards, unchanged source and null zero-length calls.
A9 memory checks validate 3,096 calls with exact data read/write boundaries,
alignment and return values. Both complete-core builds pass two relocated
A9-model runs, deliberately corrupted palette rejection and cancellation.
All physical runs match pixel/master/CPU/SPC/PCM signatures and pass active
cancel, RET1, 14 restored registers, MMU/cache witness and owned-memory release.
The Workbench result was captured and visually checked; original CPU and NES
executables remain unchanged. Common ARM/MCP suites pass.

Build with `scripts/build_mesen_snes_system_arm.py --word-memory` and the same
container argument as above. Add `--profile` for the separate profiled tool.
Outputs are `.context/amiga/mesen-snes-system-words/` and
`.context/amiga/mesen-snes-profile-words/`. Run the existing ARM checker on each
ELF (`--profile-clock` for the profiled build), plus
`tests/amiga/mesen_snes/check_arm_memory.py <elf>` using the Unicorn environment.
Native memory tests compile `system/memory.c` with `-Dmemcpy=ss_memcpy`,
`-Dmemset=ss_memset`, `-fno-builtin` and ASan/UBSan, together with
`tests/amiga/mesen_snes/memory.c`.

Installed System test now uses ZZSnes-Mesen-System-Words (303,456 bytes, MD5
`f1a45b2fe4c9e5f73b1e33cf7951dfd8`) under the normal `ZZSnes-Mesen-System` name.
The original is `ZZSnes-Mesen-System.before-words`; original metadata/readme
have corresponding before-words backups. The existing MUI System button works.
Evidence: [word-memory records](../records/2026-10-10/mesen-snes-words/).

The owner's three ARM references are reviewed in
[ARM-CORE-REFERENCES.md](ARM-CORE-REFERENCES.md). They suggest testing a separate
PocketSNES-family core for a larger performance opportunity, just as FCEUX was
compared against Mesen for NES; none has yet been run on this Amiga.
