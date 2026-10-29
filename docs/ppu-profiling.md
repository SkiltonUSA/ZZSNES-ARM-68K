# Single-core raster experiments

These experiments start from the corrected 0.7.4 core. They are opt-in build
variants; they do not change the installed player or release package.

## Candidates

| Build | Change | Preserved behaviour |
| --- | --- | --- |
| `baseline` | Unmodified release raster path | Same ARM payload as 0.7.4 |
| `fixed` | Specialise full-strength fixed-colour addition/subtraction | Colour-window depth, clipping, transparency and priority |
| `sprite` | Calculate horizontal clipping once per sprite/window span and reuse it across tile rows | Existing row batches, ordering, flips and overlaps |
| `ppu` | Reuse an unchanged sprite renderer selection within one draw call; invalidate clipping only when effective window bits change | Every register write still flushes pending rendering; timing and register values remain intact |
| `all` | Combine the three candidates | No extra core, frameskip or event coalescing |
| `profile` | Count work in the baseline path | Instrumentation only; do not use its timing as a baseline |

The existing core already batches sprite rows, caches decoded tiles and caches
colour tables. The sprite candidate moves invariant setup out of the row loop;
it does not reorder sprites or batch across mid-frame PPU changes. Renderer
selection reuse ends at the end of each `DrawOBJS` call.

The fixed-colour functions derive from the core's existing half-strength
implementations, using its full-strength saturation operations. Subscreen
blending and mosaic retain their original implementations.

## Reproduce

```sh
make test-ppu
python3 scripts/test_ppu_experiment.py --profile --rom /path/to/game.sfc --frames 3600
python3 scripts/build_current.py --release-ax --ppu-test baseline
python3 scripts/build_current.py --release-ax --ppu-test all
python3 scripts/test_ppu_arm.py /path/to/game.sfc /path/to/32-bit-checkpoint.state
```

Repeat the ARM build with `fixed`, `sprite` or `ppu` to isolate a candidate.
The usual ARM toolchain settings apply. Each variant has its own
`.context/ppu-NAME-build` directory. Builds and native tests do not deploy or
start an application on the Amiga. Game inputs and saves are not included.
The ARM-model command requires the existing Unicorn/LLVM test environment;
use that environment's Python when it differs from `python3`.

The synthetic fixture exercises 8,192 PPU window-register writes followed by
2,048 sprite scenes covering clipping, priority, size, flipping and colour math.
It compares visible window spans, framebuffer and depth output against the
baseline under AddressSanitizer and UndefinedBehaviorSanitizer. Optional game
tests compare video hashes, stereo sample counts and PCM hashes on every frame.

Timing must use matched ARM builds and the same saved position, input, region,
display and audio settings. ARM core time and end-to-end time are separate
measurements. The core includes CPU, GSU where present, PPU and audio work;
its improvement is not automatically an equivalent displayed-FPS improvement.

Profile counts are workload-specific. Window invalidations and actual window
rebuilds are distinct: several writes may precede one rebuild. Neither native
test timing nor operation counts establish a physical A9 speed gain.

## Work observed

Counts from 3,600 frames per game, with the same scripted input:

| Workload | Clip rebuilds | Avoidable invalidation writes | Renderer selections | Repeated sprite selections |
| --- | ---: | ---: | ---: | ---: |
| Stunt Race FX | 9,863 | 1,548 | 106,325 | 0 |
| Super Mario World | 7,965 | 16 | 58,436 | 23,093 |
| Star Fox 2 | 396,748 | 10 | 3,224,075 | 359,205 |
| Doom | 13,399 | 17 | 26,824 | 0 |

Star Fox 2's frequent window rebuilds are largely necessary for this workload;
simply suppressing them would risk incorrect colours or clipping. Repeated
sprite renderer selection is a narrower opportunity. Full-strength fixed-colour
renderer selections were 7,160 for Stunt Race FX and 3,078 for Star Fox 2, and zero
for the other two workloads. These are selections, not blended-pixel counts.

All five uninstrumented variants match the synthetic fixture and all 3,600
video/stereo-PCM frames in each game. The instrumented baseline also matches.
The combined ARM variant matches 30 resumed frames at two relocation addresses
for Stunt Race FX, Super Mario World and Star Fox 2. Model addresses and execution
budgets are test settings, not physical DDR allocations or performance results.

The isolated `fixed`, `sprite` and `ppu` ARM builds also match 30 Stunt Race FX
checkpoint frames at both relocation addresses. The ordinary native core suite,
expanded cartridge/option tests and Super FX colour/state regressions pass.

## Hardware method and limits

Physical tests use one application on Core1, 96 MiB of runtime-probed Exec-owned
ZZ9000 RAM and the existing 32 MiB executable reserve. Each run starts afresh,
loads the same private checkpoint for its game, and completes 660 frames with
zero controller input, NTSC timing, fullscreen output, scanlines off and the
per-app ZZ9000AX audio backend. The three checkpoints are a Stunt Race FX race
save and frame-3,000 saves for Super Mario World and Star Fox 2. Thus the native
boot-to-game counts above and the physical resumed workloads cover different
intervals; their numbers should not be directly divided or correlated.

The combined comparison uses baseline/candidate/candidate/baseline order.
Individual variants are screening runs, followed by another baseline for the
two later workloads. The Star Fox 2 PPU-only candidate was then repeated with
a final baseline check. Core hashes are sampled at frames 1 and 600; they match
within each game's hardware runs. Every-frame video/PCM equivalence comes from
the separate native and ARM-model checks, not from those two samples.

Complete PCM submission/retirement and zero accounting gaps do **not** establish
uninterrupted audible playback. The AX logs record scheduled silence, and these
were not listening tests. Displayed flips can be fewer than emulated frames;
`660 / elapsed` must not be presented as the number of distinct pictures shown.

Raw logs, binary identities, input digests and numerical summaries are in
[the records directory](../amiga/records/2026-10-29/ppu/). ROMs and saves remain
private. Recorded mapping addresses are observations, not reusable allocations.

All 25 physical runs complete their 660 frames, retire all submitted PCM, finish
pending flips, pass register/MMU/cache-witness restoration and release owned
memory. The temporary test drawer was removed. Core1 is idle, the bridge responds,
and saved screenmode and live AHI preference checksums are unchanged. The installed
launcher and player still identify as 0.7.4-dev.

## Decision

Combined-candidate mean ARM core time per 660-frame run:

| Workload | Baseline | Combined | Core-time change |
| --- | ---: | ---: | ---: |
| Stunt Race FX | 15.313 s | 15.319 s | +0.04% |
| Super Mario World | 5.469 s | 5.520 s | +0.94% |
| Star Fox 2 | 9.691 s | 9.708 s | +0.17% |

Positive means **slower**. Combined results use two runs; baseline means include
the later stability checks where available. End-to-end timing was more variable
than ARM core time. A shorter total time in one candidate run does not overturn
the core-time regression or establish a rendering/FPS improvement.

The PPU-only Star Fox 2 result repeats: 9,638,681 and 9,639,471 microseconds of
ARM core time, approximately **0.54% less** than baseline. This is a small,
scene-specific core saving, not a proven displayed-FPS gain. Stunt Race FX and
Super Mario World are effectively unchanged in the PPU-only screening runs.
The sprite-only Star Fox 2 screening run is about 0.32% lower, but Super Mario
World is about 0.42% higher; that result has not been repeated. Fixed-colour
specialisation is slower in all three screening runs.

Do not promote the combined candidate: it preserves the tested output but does
not reduce ARM core time in the tested scenes. Normal builds and the 0.7.4 release
retain their existing renderer. The isolated variants remain available to test
other workloads without changing the default.

These results do not establish that fixed-colour specialisation or sprite
batching can never help. They show that these particular changes, on this core
and A9 configuration, do not justify a general speed claim. Function sizes and
code placement also change; attributing the regression specifically to cache
behaviour would require further measurements. Individual percentages must not
be added to predict the combination.

For Stunt Race FX, the next separate profiling target is GSU instruction
dispatch, RAM access and pixel plotting. The Super FX executes integer/fixed-point
operations; enabling the A9's VFP does not directly accelerate its guest code.
For example, [Snes9x's `FMULT` implementation](https://github.com/snes9xgit/snes9x/blob/master/fxinst.cpp#L2578-L2592)
uses a signed integer multiplication and extracts the upper 16 bits.
