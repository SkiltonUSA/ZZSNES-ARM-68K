# AHI servicing and bitmap-lock profiling

AHI submission calls account for much more measured elapsed time than copying
host-owned PCM into the AHI buffers. Four-frame batching did not improve overall
frame timing, so the existing three-frame default remains unchanged.

## Call costs

A4000TX / ZZ9000, 29 October 2026. Fixed presentation, 660 frames per run, first
60 excluded. Each value below is the mean elapsed total across two runs for the
following 600 frames. NES used Mario Bros.; SNES used Super Mario World.

| Measured calls | ZZNES | ZZSNES |
|---|---:|---:|
| P96 lock + unlock | 643.16 ms | 464.60 ms |
| AHI SendIO | 1,896.25 ms | 1,652.17 ms |
| AHI WaitIO | 5.39 ms | 5.12 ms |
| Worker PCM batching copies | 51.26 ms | 81.38 ms |

These are **elapsed call intervals, not CPU utilization**. Preemption and driver
waiting can contribute; intervals from different tasks can overlap and must not
be added as independent frame costs. The copy row covers host-queue-to-AHI-buffer
copies, not the earlier ARM-to-host PCM transfer. Long lock calls do not establish
that graphics-library execution itself consumes that time.

Most AHI submissions began while the bitmap was locked, but the players spend
much of their time in that state. That correlation does not prove lock contention
causes the submission delays. No scheduling, global preferences or firmware were
changed for this profile.

The histograms cover <16, <64, <256, <1000, <4000, <16000, <64000 and >=64000
microseconds. Kind 0/1 means lock/unlock, 2/3 SendIO with the bitmap locked/unlocked
at entry, 4 WaitIO and 5 worker PCM copy. No per-call text output is produced during
measurement. Two initial SNES runs missed the stereo copy expression; the generator
now requires an exact match and both SNES runs were repeated. Published analysis
uses NES runs 1/2 and SNES runs 3/4.

## Output configuration finding

The read-only preference snapshot and AHI mode enumeration show:

| Selection | Configured mode | Mixing frequency |
|---|---|---:|
| Device unit 0 | Paula: Fast 8-bit mono (`0x0002000c`) | 8,000 Hz |
| Music unit | ZZ9000AX: 16-bit stereo (`0xcafe0001`) | 32,000 Hz |

Both tested players call `OpenDevice(AHINAME, 0, ...)`. Selecting ZZ9000AX for the
music unit therefore does not select it for these device-unit-0 streams. Their
submitted PCM remains 32,000 Hz mono for NES and 32,040 Hz stereo for SNES; that
format is distinct from the configured output mixer format.

The installed SDK reports the 32-byte preference record and offsets 0/4/8 for
unit/mode/frequency, matching the [AHI preference structure definitions](https://gitlab.com/freepascal.org/fpc/source/-/blob/f9893ac71056e954d4fe5322932f66780ec78d31/packages/amunits/src/otherlibs/ahi.pas).
This establishes the configuration, not an acoustic measurement or proof that
changing the device will improve FPS. No preferences were written.

The next useful investigation is an **app-selected ZZ9000AX audio path**, retaining
the existing unit-0 option and avoiding system-wide preference changes. That
backend has not been implemented or benchmarked in this round.

## Four-frame batch experiment

The isolated candidate batches four frames instead of three, with a six-frame
prebuffer. NES buffers grow from 6,144 to 8,192 bytes; SNES stereo buffers grow
from 12,288 to 16,384 bytes. SNES queue-capacity feedback accounts for two
four-frame requests. Sample rates, sample formats and ARM emulation stay unchanged.

The actual-worker native test delivers 600 blocks with **150 submissions instead
of 200**, preserving ordered samples. Short/forced flushes, save-state draining,
allocation/open failures, device errors, cancellation and cleanup pass under
AddressSanitizer/UndefinedBehaviorSanitizer.

Matched hardware order was reference–candidate–candidate–reference for each
console. Periodic readbacks and automatic captures were disabled for timing;
initial pixel verification remained enabled. The mean of two 600-frame timing
sums is:

| Console | Three-frame reference | Four-frame candidate | Extra time |
|---|---:|---:|---:|
| ZZNES | 10.5684 s | 10.6814 s | 1.07% |
| ZZSNES | 10.9737 s | 11.0364 s | 0.57% |

No useful frame-rate improvement was established. NES late-link counts changed
from 133/132 to 85/84; SNES from 98/81 to 103/80. With fewer submissions and longer
buffers, raw late-link counts alone do not prove better audio. No listening test
is claimed. Longer batches increase buffering; NES's startup prebuffer also grows
from four to six frames. The normal release keeps the original policy.

All eight timing runs completed 660 flip acknowledgements, the initial pixel
check, every PCM sample and register/MMU/cache restoration with owned memory
released. Both candidates also passed pause/resume, fullscreen → window → fullscreen and
clean quit, using a fresh first-frame readiness marker before input. All 16
player runs in this investigation ended cleanly, including the two superseded
SNES profiling runs. The earlier unrelated NES freeze remains unresolved.

## Reproduction

Use the existing toolchain environment. Baseline and experiments are separate
build drawers; none deploy automatically:

```sh
python3 scripts/build_current.py --expanded --jit-registers --presentation-test serial
python3 scripts/build_current.py --expanded --jit-registers --presentation-test serial --host-service-test profile
python3 scripts/build_current.py --expanded --jit-registers --presentation-test serial --host-service-test batch4
```

`--host-service-test` requires `--presentation-test serial`, which in turn requires
the register-experiment build and original ACK/audio-wake policies. Use private,
bounded sessions such as `--frames 660`. These are developer tests, not release
presets. The ARM image and MMU headers remain byte-identical to the tested base.
Fresh integrated builds reproduce the tested host behavior.

```sh
python3 scripts/test_audio_batch_worker.py PATH_TO_PLAYER_STAGE 4
clang -std=c99 -Wall -Wextra -Werror development/host-service-20261029/test_cost.c -o .context/test-host-cost
.context/test-host-cost
```

[Raw evidence and action log](../amiga/records/2026-10-29/host-service/) include
histograms, timing runs, lifecycle checks, buffer patches and build hashes.
No game data or executables are included.
