# Star Fox 2 audio investigation

The reported stutter occurred at **Normal**, before Maximum was tried.
The current investigation reproduces missed audio deadlines at Normal. It is
separate from the PCM publication bug corrected in 0.7.11.

[Upstream routine comparison](superfx-audio-upstream-review.md) traces Snes9x,
Snes9x GX, Snes9x-Z and MesenCE's GSU scheduling, audio synthesis and buffering.

## Controlled physical runs

A4000TX / ZZ9000, XX19c / XACP 1.7. Each comparison starts the same supplied
NTSC Star Fox 2 ROM from reset and runs 1,200 core frames with no input in a 2×
window, software Soft scanlines and the FPS overlay disabled. These are boot /
attract-sequence measurements, not a complete interactive gameplay test.
The test profiles are private; installed settings are unchanged.

Normal requires about 20.03 seconds for 1,200 NTSC core frames. Core-frame
throughput is not the game's unique rendered-picture rate. Maximum changes
emulated 65C816 timing; it is not an ARM clock boost and may alter workload.

| CPU / output / rendering | Play time | ARM core time | Audio result |
| --- | ---: | ---: | --- |
| Maximum / configured AHI / all frames | 42.872 s | 30.794 s | 355 linked-request gaps |
| Normal / configured AHI / all frames | 38.753 s | 26.345 s | 346 linked-request gaps |
| Normal / Paula HiFi 22 kHz / all frames | 35.191 s | 26.346 s | 348 callback starvation events |
| Normal / Paula HiFi 22 kHz / automatic frameskip | 32.205 s | 23.141 s | 347 callback starvation events; 277 pictures skipped |
| Normal / Paula HiFi 22 kHz / timed Super FX | 39.384 s | 30.758 s | 346 callback starvation events |
| Normal / Paula HiFi 22 kHz / auto threshold 50% | 24.549 s | 15.200 s | 194 callback starvation events; 1,013 pictures skipped |

The configured-AHI and explicit-Paula backends use different diagnostics.
Zero `gaps` on the callback backend, or zero `starvations` on the linked-request
backend, does not mean uninterrupted audio. Automatic frameskip reduced
scheduled silence buffers from 755 to 606, but did not eliminate underruns.
All six runs completed all 641,658 stereo PCM frames with no AHI error and
clean register/MMU/cache-witness restoration and owned-memory release.

## Interpretation

At Normal, core execution alone averaged 21.95 ms per frame, above the 16.69 ms
NTSC budget. The explicit Paula backend reduced host overhead without changing
core work materially. Rendering fewer pictures helped, but remained below the
sustained production rate needed for continuous sound. Larger buffers can absorb
brief variation; they cannot repair this sustained shortfall and add latency.

In the legacy core, Star Fox 2 is outside the `WinterGold` title list and enters
`FxEmulate(~0)`, running the GSU until it stops rather than using the other titles'
per-scanline instruction budget. This is a candidate contributor to long core
bursts, not a measured attribution of all elapsed time to Super FX.
The separate timed Super FX experiment accounts for instruction and memory
costs within scanline budgets. Its timing differs and needs its own visual,
sound and game-speed acceptance; lower elapsed time alone cannot justify
replacing the default.

No global AHI, firmware, installed player, library or save was changed.

## Timed experiment result

Rebuilt the existing timed Super FX experiment with the same current PCM mapping
and audio fixes. Native timing/state fixtures pass; relocated ARM checks at two
addresses pass Legacy video/PCM equivalence, old-player Legacy state resume and
Timed state/PCM replay. This is model correctness evidence, not hardware timing.

On hardware, Timed took 39.384 seconds versus 35.191 seconds for Legacy with
Normal CPU / explicit Paula. It still starved and scheduled 988 silence buffers
versus 755. Different GSU timing changes the game trajectory, so these are not
identical-work CPU benchmarks. It is nevertheless **not an audio remedy** for
this boot sequence and is not promoted or installed.

[Raw logs and results](../amiga/records/2026-10-30/starfox2-audio/).

## Sample-publication check

A separate 600-frame Normal / Paula run captured PCM before AHI. Its ARM payload
matches the release byte-for-byte; only the host recording instrumentation differs.
The capture contains 320,829 stereo frames (641,658 individual samples).
Every sample matches execution of that exact ARM payload in the instruction
model; zero differences and zero stale-frame mismatches. The physical run still
reports 148 starvation events, while all PCM completes without an AHI error.

This distinguishes transport corruption from delivery gaps in the captured boot
sequence. It does not establish that PocketSNES synthesis or GSU timing matches
original SNES hardware, nor does it prove every gameplay scene correct.
No game audio or ROM data is included in the report artifacts.

## Threshold test and next work

Auto threshold 50% intervened earlier and reduced scheduled silence buffers to
246, but rendered only 187 of 1,200 pictures and still starved 194 times. That
tradeoff is not accepted as a default or a complete fix. The CPU remains Normal;
only the frameskip mode/threshold changed in this private test.

The following stage-level profiling and renderer experiments target repeatable
checkpoints, reducing rendering cost while preserving video/PCM output. Buffer-depth changes should follow measured production
headroom; they cannot manufacture the missing emulation throughput. Validate
any candidate with interactive gameplay and listening as well as counters.

All tests returned cleanly. Postflight bridge responds and Core1 is free.
The installed 0.7.11 release and user settings were left intact. Results here
are local investigation records, not a new release or a claim of resolved audio.

## Stage profiling and rendering candidates

The opt-in `--release-ax --core-profile` build measures three boundaries with
the already-mapped, read-only Cortex-A9 global timer: Super FX execution, PPU
screen updates and the normal PCM mixer. Per-frame unsigned timer deltas are
published in the private diagnostic reply; the host accumulates them in 64 bits.
It neither enables PMU facilities nor changes guest timing. Instrumented timings
are screening data, not an uninstrumented optimisation baseline.

The same 1,200-frame Normal / Paula boot sequence measured:

| Stage | ARM time | Share of measured core time |
| --- | ---: | ---: |
| Super FX | 8.367 s | 31.78% |
| PPU rendering | 10.326 s | 39.22% |
| PCM mixing | 0.456 s | 1.73% |
| Remaining CPU/APU, frame hashing and adapter work | 7.179 s | 27.27% |

The profiler's video and stereo PCM match the release across 30 saved-position
frames at two ARM relocation addresses. The modeled timer stays zero, so that
check proves output transparency, not physical timing.

Two opt-in renderer candidates retain the normal release path by default:

- `--ppu-test row`: snapshot palette, priority and target pointers once per
  ordinary RGB565 tile rather than reloading renderer state for pixel groups.
- `--ppu-test clip`: traverse the visible unflipped RGB565 span directly,
  avoiding masked temporary four-pixel groups. The inherited horizontal-flip
  path remains intact; colour-math and high-resolution routines are unchanged.

The clipped experiment initially exposed a separate inherited C macro problem:
`SWAP_DWORD(dword)` computes a value without assigning it, while flipped-clipping
call sites discard the expression. ARM assembly builds have a different macro,
but this portable-C build does not select it. Correcting that would change
existing pixels; it is a separate graphics fix, not silently included in this
performance experiment. The original flipped path is retained for equivalence.

Reproduce local correctness checks (ROMs/states are supplied separately):

```sh
python3 scripts/test_ppu_experiment.py --mode row --mode clip --rom /path/game.sfc --frames 1200
python3 scripts/build_current.py --release-ax --core-profile
python3 scripts/test_core_profile_arm.py /path/game.sfc /path/game.state
python3 scripts/build_current.py --release-ax --ppu-test row
python3 scripts/test_ppu_arm.py /path/game.sfc /path/game.state --baseline .context/ax-release-build --candidate .context/ppu-row-build --frames 60
```

Use the established ARM toolchain environment for compilation and Unicorn/LLVM
environment for model tests. Model instruction profiling requires clearing the
model translation cache after installing block hooks; otherwise cached blocks
can escape observation. Such incomplete profiles were discarded.

### Ordinary tile result

From the same frame-3,000 Star Fox 2 checkpoint, two baseline and two candidate
runs each completed 660 frames with Normal CPU timing, Paula HiFi 22 kHz, 2×
window and Soft scanlines. Order: baseline, row, baseline, row.

| Variant | Mean ARM core time | Audio starvation events per run |
| --- | ---: | --- |
| Release | 9.759962 s | 148, 145 |
| Ordinary tile candidate | 9.266535 s | 126, 126 |

That is **5.06% less core time in this scene**, not a general emulator speedup.
The cold 1,200-frame opening comparison was effectively unchanged: 26.345 s
release versus 26.345 s candidate. The opening uses clipped tile rendering;
the later checkpoint uses ordinary tiles. The gains must not be added to other
historical percentages. Audio still starves, so this is not a completed sound fix.

Both renderer variants match randomized window/sprite/tile/depth tests under
ASan/UBSan and 1,200 video/stereo-PCM frames each for Star Fox 2, Stunt Race FX
and Super Mario World. The ordinary tile variant also matches 60 checkpoint
frames for each game at two ARM relocation addresses. Physical runs match sampled
frame hashes, complete all submitted audio and restore/free hardware state cleanly.

[Profiling and candidate records](../amiga/records/2026-10-30/starfox2-profile/).

### Clipped-span result

Two cold 1,200-frame runs of the clipped-span candidate used 25.759062 and
25.760570 seconds of ARM core time, approximately **2.22% less** than the
26.345-second baseline. Audio starvation remained at 348 and 347 events.
This is a repeatable CPU saving, not an audible-stability fix.

Further model inspection of frames 601–630 found 733,380 full-width calls to
`DrawClippedTile16`, plus 23,160 genuinely partial-span calls: **96.94% of those
calls need no clipping**. The `rows` variant combines ordinary tile snapshots,
partial-span traversal and a full-width dispatch to the ordinary renderer.
It keeps the same flipping, priority and transparency behavior and remains
separate from release output. Its costs must be measured as a combination;
individual savings must not be added.

### Combined renderer result

Two uninstrumented cold runs of `rows` used 25.482074 and 25.481647 seconds
of ARM core time. The matched release run used 26.346503 seconds: **3.28% less
core time**. Sampled frame hashes match. Neither run skips emulated or rendered
frames. Audio still starved 347 and 348 times, with 702 and 708 silence buffers.
All submitted PCM retired and both runs restored registers/MMU/cache witnesses
and released their allocation cleanly.

Before hardware testing, the combined build passed randomized raster/depth
fixtures under ASan/UBSan, 3,600 per-frame video/stereo-PCM comparisons each for
Star Fox 2, Stunt Race FX and Super Mario World, and 60 saved-position frames
for each game at two ARM relocation addresses. These checks establish equality
with this release core, not accuracy against original SNES hardware.

The opening still averages 21.23 ms of core work per emulated frame, above the
16.69 ms NTSC budget before display/audio host costs. The candidate remains
opt-in; installed 0.7.11 and saved settings are unchanged. There is no new
release or claim that the reported stutter is solved.

Next profiling targets are `DrawBackgroundOffset` and the GSU `fx_run` dispatch,
register and memory helpers. Preserve GSU stop/IRQ semantics and timing. The
[separate flipped-clipping correction](clipped-tiles.md) passes pixel/depth
oracles; physical validation remains pending. Audio buffering can absorb short bursts,
but sustained production must first meet the playback rate.

The combined candidate also completed 660 frames from the frame-3,000 checkpoint:
9.202805 s core time versus the two-release-run mean of 9.759962 s (**5.71%
less**, one combined run). Audio still starved 122 times and queued 143 silence
buffers. Sampled pictures match the baseline and teardown passed. This is a
scene-specific core measurement, not an equivalent gameplay-FPS gain.
Postflight bridge responsive, Core1 idle. Raw logs and build hashes are retained
in the profiling records linked above; commercial ROMs, states and PCM are excluded.
