# Per-application ZZ9000AX audio experiment

**Routing correction:** normal development builds again use configured AHI unit 0.
Promoting automatic AX selection caused silence for speakers connected to Paula.
The following AX measurements remain experimental; use `--ax-audio-test` explicitly.


**Release update:** included in [ZZSNES 0.7.3 Beta](https://github.com/SkiltonUSA/ZZSNES-ARM-68K/releases/tag/v0.7.3-beta). Normal-play
release validation is recorded in [release-validation.md](release-validation.md).
The measurements below describe the earlier controlled experiment.

This optional backend selects the installed **ZZ9000AX 16-bit stereo AHI mode**
without editing global AHI preferences. The normal release still uses unit 0.
It is an audio-output and host-scheduling experiment, not an ARM-core/JIT change.

## Why this test

Read-only inspection found unit 0 configured for **Paula Fast 8-bit mono at
8 kHz**, while the Music preference selected ZZ9000AX. Both console players
opened unit 0. The existing source PCM is 32 kHz mono for NES and 32.04 kHz
stereo for SNES. Those source formats remain unchanged here.

Opening device unit 255 alone does not route streaming requests to AX: it is
AHI's low-level access unit. This experiment instead enumerates registered
modes, checks the named AX mode's stereo/16-bit/realtime capabilities, and
allocates a per-application AHI controller. Failure to obtain it falls back to
the original unit-0 worker. AX playback-start errors terminate audio cleanly;
they do not silently replay queued samples on another backend.

## Buffer and task ownership

- Keep the eight-slot producer ring, three-frame batches and existing startup
  thresholds: four NES frames or six SNES frames.
- The audio worker owns the controller and three dynamic PCM buffers. They can
  be current, scheduled and ready; a fourth permanent buffer supplies silence.
- SoundFunc retires the **previous** sound, schedules the next ready buffer and
  signals the worker. It never allocates, waits, copies PCM, accesses ARM memory
  or calls graphics/DOS functions.
- Publication uses a short interrupt exclusion. PCM is immutable until retired.
  Ring positions stay bounded rather than relying on wrapping counters modulo 3.
- Buffer allocation follows AHI's reported minimum and actual mix rate. The
  installed driver reports 3,840 sample frames; active silence segments are 20 ms.
- Pause/state operations flush partial batches. Shutdown joins the worker;
  AHI playback/controller teardown precedes releasing samples, signals and task
  code. Original ARM reset-before-free and display-lock rules stay in place.
- SNES feedback includes nine pending frame blocks on AX and six on fallback.

This remains AHI on the 68k, with host PCM copies and driver mixing/transport.
It is **not direct ARM-to-AX DMA**, HDMI audio, or removal of Zorro traffic.
Low-level hardware allocation can also fail if another application has it.

## Validation

Native sanitizer checks exercise the real queue, worker and API adapter:
ordered mono/stereo PCM; reuse; partial streams; state drain/resume; cancellation;
missing mode/capabilities; allocation/load/query/start failures; fallback; and
controller teardown before buffer release. They do not emulate the audio driver.

Standalone silent mono/stereo probes each account for 96,000 sample frames and
close successfully on the Amiga. The initial two-buffer design inserted avoidable
silence; three buffers remove that regular alternation. Initial logs are retained.
Two exploratory 120-frame emulator runs then passed; final bounded-index code is
used by the repeated timing and lifecycle tests below.

The full integrated builds match the tested host/backend and recorded ARM payload,
MMU headers and runtime allocations. Existing native/model checks pass. The first
model invocation lacked llvm-nm on PATH; rerunning that check with the configured
LLVM path passed. Product identities remain NES 0.8.2-dev / SNES 0.7.2-dev, distinct
from packaged release versions.

| Test | Unit 0 FPS | Per-app AX FPS | FPS gain | Less frame time |
| --- | ---: | ---: | ---: | ---: |
| Mario Bros. — NES | 57.18 | 59.04 | **3.25%** | 3.14% |
| Super Mario World — SNES | 56.43 | 59.00 | **4.56%** | 4.36% |

Mean of two runs per configuration. NES reference/AX: 10.492331/10.162544 s;
SNES: 10.632564/10.169162 s for 600 steady frames. Each candidate run is faster
than either reference run, but this is a short, two-repeat comparison.
AX silence schedules were NES 14/16, SNES 8/13; audible quality remains unverified.

All timing runs use the same game, ARM payload, region pacing, fullscreen,
three-frame batch size, original ACK polling and every-frame presentation.
Each runs 660 frames: 60 warmup and 600 measured, in reference/AX/AX/reference
order. Initial exact pixel readback remains; recurring readbacks and automatic
snapshots are disabled. Final unaudited hash sentinels are not pixel-equivalence
proof. These are game-specific measured emulation/presentation rates, not display
refresh rates, a claim of optical tear-free scanout or all-title compatibility.

The comparison changes both AHI output configuration and buffer scheduling. It
cannot attribute the gain solely to the AX hardware or promise the same result
when unit 0 is already configured differently. Percentages must not be added to
unrelated JIT/pixel microbenchmarks.

AHI completion means the mixer accepted/consumed the stream; it does not prove
DAC timing or that somebody listened. `silence_queued` counts fallback silence
schedules, including startup/end/pause; it is not comparable to the unit-0 linked
request `gaps` counter. No claim of inaudible gaps or long-run sound quality.

All eight timing runs complete every PCM frame, every display acknowledgement,
register/MMU/cache-witness restoration and owned-memory release. Both final
candidates also pass paused save/load, window/fullscreen round trips, resume and
quit. Twelve emulator runs and four standalone probes in total this round.
Postflight: bridge responsive, Core1 idle; live AHI and saved display checksums
unchanged. The private test drawer was removed.

The release players, user libraries/saves, firmware, startup, global AHI settings
and saved display mode remain unchanged. The earlier unexplained NES freeze is
not resolved by these short successful tests.

## Build and next step

```sh
python3 scripts/build_current.py --expanded --jit-registers \
  --presentation-test serial --ax-audio-test
```

Run `python3 scripts/test_ax_audio.py` for the native tests. Build flags reject
combinations with overlap, earlier audio notification, throttled ACK polling or
host-service profiling/batch4. Builds never deploy automatically.

The next gate is listened playback and longer sessions, including a system whose
unit 0 already selects AX. Keep this opt-in until those comparisons establish
quality and latency as well as throughput; do not change global preferences to
make a benchmark faster.

## References

- [AHI developer guide: obtaining audio hardware](https://www.lysator.liu.se/ahi/v4-site/files/ahi/doc/ahidev_8.html)
- [AHI: dynamic samples and buffer sizing](https://www.lysator.liu.se/ahi/v4-site/files/ahi/doc/ahidev_9.html)
- [AHI API reference: callbacks, scheduling and teardown](https://www.lysator.liu.se/ahi/v4-site/files/ahi/doc/ahi.html)
- [Previous AHI and bitmap-lock profiling](host-service-profiling.md)
- [Raw hardware records](../amiga/records/2026-10-29/ax-audio/)
