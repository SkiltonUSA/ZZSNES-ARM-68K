# Cortex-A9 shared-audio publication

## Problem

The Cortex-A9 synthesizes PCM in private core buffers, then writes a big-endian
copy into an Exec-owned ZZ9000 RAM window. The 68k copies that window into its
AHI queue. These are two processors with separate cache-maintenance paths;
sharing an address does not make their caches coherent.

The previous ARM page policy treated the PCM window as private write-back,
write-allocate memory, like the core's working buffers. The worker cleaned the
written range before replying, and the 68k performed its own cache maintenance
before copying. In the ZZNES investigation, this path nevertheless returned
stale samples from earlier frames. The fault was present before AHI mixing.

A recording made **after the 68k copy** had initially been mistaken for a direct
recording of synthesis. Comparing it with the exact ARM payload in an instruction
model corrected that diagnosis. Of 78,575 differing NES samples, 78,505 matched
the preceding frame, commonly along 32-byte boundaries. An extra barrier alone
did not cure the problem. Mapping the shared PCM window as noncacheable did:
all 638,940 captured NES samples then matched the model byte-for-byte.

That is evidence for a shared-memory publication fault in the tested path, not
proof of a particular Cortex-A9 silicon erratum or a general fault in AHI.
ZZSNES had the same private-cache policy for its stereo publication window.
The independent SNES comparison reproduced the stale-sample pattern too.

## Fix in code

The build adapter [prepare_audio_publication.py](../scripts/prepare_audio_publication.py)
modifies the generated `core/mmu_plan.h` actually compiled by `mmu.c`.
For SNES the bounded window is 1,024 stereo signed-16 sample frames:

```c
#define MB_AUDIO_BEGIN 0x170000u
#define MB_AUDIO_END (MB_AUDIO_BEGIN + 4096u)

/* Added alongside the existing shared mailbox/table checks. */
if (mb_overlap(page, base + MB_AUDIO_BEGIN, base + MB_AUDIO_END))
    attr = MB_PAGE_SHARED;
```

`base` is the runtime-probed ARM address of the launcher's Exec allocation.
These constants are offsets within that allocation, never fixed physical DDR
addresses. Every page intersecting the window is included. Exec allocations
are 64-byte aligned, not necessarily 4 KiB aligned; the 4 KiB stereo window can
therefore occupy parts of two pages.

| Region | ARM short-descriptor policy |
| --- | --- |
| Core RAM and synthesis buffers | Normal inner write-back/write-allocate, outer noncacheable; `MB_PAGE_PRIVATE = 0x116` |
| Shared PCM, mailbox and page tables | Normal inner/outer noncacheable, shareable; `MB_PAGE_SHARED = 0x452` |
| PCM execution permission | Execute-never added by the existing data-region rule; descriptor attributes `0x453` |

The worker still bounds the count to 1,024 frames, publishes big-endian stereo
samples, performs its existing range synchronization, and replies through the
shared mailbox. The 68k still synchronizes and copies only the valid `count * 4`
bytes. Source synthesis, sample rate, byte order and buffer ownership are
unchanged. Only the pages used for communication lose private ARM caching.

This is a cache-policy correction, not new DMA hardware, a synthesis algorithm,
or a change to the audio driver. `volatile` alone would not solve it: it affects
compiler access rules, not ARM page attributes. The 68k's `CacheClearE` also does
not configure the ARM's page tables or establish coherent caches between CPUs.

## Pause and resume

The shared AHI worker now treats an intentional user pause as draining, like a
save-state operation. Once queued PCM has retired it clears `primed`; resuming
collects the normal startup batch again. This prevents a deliberate pause from
being counted as starvation and avoids restarting with an undersized batch.
The cross-task pause flag is volatile; existing task synchronization and queue
ownership are retained. This host-side change is separate from the ARM mapping
fix.

SNES already serializes mixer history and sample phase. No state format or
synthesis-quality mode is changed by this update. The NES HQ FIR/state work is
NES-specific and is not transplanted into PocketSNES.

## Validation

- The page-policy test covers every 64-byte allocation offset within a page,
  both audio-page crossings, adjacent private pages, ROM, mailbox and allocation
  bounds: [test_audio_pages.cpp](../development/audio-publication-20261030/test_audio_pages.cpp).
- The actual host worker is exercised in mono and stereo for ordered PCM,
  pause/re-prime, state drain/resume, partial batches, cancellation and device
  startup fallback.
- ARM-model captures provide an independent reference for the PCM reaching the
  68k. Model execution alone cannot validate physical cache coherence.
- Physical runs must check sample delivery, starvation, state operations,
  register/MMU/cache restoration and reset-before-free. Successful counters do
  not establish analogue listening quality.

No FPS gain or SNES sound-quality percentage is claimed from this change.
The NES synthesis speedups have a different cause and are not SNES measurements.

NES investigation and release:
[ZZNES audio-synthesis report](https://github.com/SkiltonUSA/ZZNes-ARM-68K/blob/main/docs/audio-synthesis.md).

## ZZSNES hardware comparison

The same Chrono Trigger ROM, 600 frames, no input, default core options and
Paula HiFi 22,030 Hz output were used for both private capture builds. The
reference runs the rebuilt ARM payload in Unicorn. Capture occurs immediately
after the 68k copies PCM, before AHI conversion; it is held in memory and written
to disk after playback stops. ROM and audio data are not distributed.

| Measurement | Previous cached PCM policy | Noncacheable shared PCM |
| --- | ---: | ---: |
| Stereo sample frames | 320,829 | 320,829 |
| Individual left/right samples compared | 641,658 | 641,658 |
| Samples differing from ARM reference | 46,129 | **0** |
| Differing samples matching the preceding frame | 46,101 | 0 |
| Audio starvation transitions | 0 | 0 |
| Completed sample frames | 320,829 | 320,829 |
| Clean register/MMU/cache restoration and release | Pass | Pass |

The previous capture uses the preserved 0.7.8 ARM payload with the preceding
0.7.11 host-audio candidate, rather than silently substituting an old ELF file.
The corrected capture uses the newly rebuilt release ARM payload. Native core
settings and synthesis are unchanged. After byte-order conversion, every
captured corrected sample equals the reference. Instruction-model timing is not
a physical speed measurement, and this comparison does not measure the DAC or
prove every game's audio is free of core-emulation defects.

The captured builds add memory recording overhead; their elapsed times are not
used for FPS claims. See [logs, hashes and comparisons](../amiga/records/2026-10-30/audio-publication/).

Reproduce local checks after `python3 scripts/build_current.py --release-ax`:

```sh
make test-audio-publication
python3 development/audio-publication-20261030/capture_arm.py \
  /path/to/your/chrono.sfc .context/ax-release-build .context/chrono --frames 600
python3 development/audio-publication-20261030/test_states.py \
  /path/to/your/chrono.sfc .context/ax-release-build
```

The model tools require Unicorn and LLVM's `llvm-nm` on PATH (or set `LLVM_NM` to its executable path). Hardware runs separately
require a free Core1 and the normal guarded launcher. No test command above
connects to or modifies an Amiga.

The optional [capture-player builder](../development/audio-publication-20261030/build_capture.py)
adds bounded in-memory recording to an isolated host build. It is intended for
600 NTSC frames; check the logged byte count against the model before comparing.
It does not change the release player or deploy anything. Compare the downloaded
big-endian capture with the little-endian model using
[compare_pcm.py](../development/audio-publication-20261030/compare_pcm.py), passing
`--require-match` for an accepted build. Hashes alone do not replace the full
sample comparison performed by this tool.

## Release lifecycle results

| Test | Frames | Completed stereo sample frames | Starvation |
| --- | ---: | ---: | ---: |
| Chrono Trigger: fresh-process resume | 120 | 64,166 | 0 |
| Chrono Trigger: Paula fullscreen, pause/save/load | 1,800 | 962,488 | 0 |
| Super Mario World: older v3 state, Paula | 120 | 64,166 | 0 |
| Super Mario World: AX, 2× window | 1,800 | 962,488 | 0 |

All runs completed queued PCM and passed register/MMU/cache restoration and
owned-memory release. Silence during deliberate pauses is not an underrun.
