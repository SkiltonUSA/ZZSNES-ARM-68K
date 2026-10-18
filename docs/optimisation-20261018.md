# ARM and library optimisation — 0.6.2-dev

This is an opt-in development candidate. Existing release downloads and default
release rebuilds are unchanged. The tests below use an A4000TX/68060 with a
ZZ9000, XX19c/XACP 1.7 and AHI. They do not establish slower-68k performance.

## Changes borrowed from ZZDosBox experience

- Move invariant work out of pixel loops. Cache valid row spans and process
  left border, game image and right border separately.
- Measure compiler choices: GCC `-O2` replaces `-Os` in this candidate. Soft
  float and existing numerical rules remain; no fast-math or speculative JIT.
- Reduce 68k formatting overhead: temporary sort-title cache, block hex writes
  and bounded token parsing for library records.
- Keep timing, framebuffer accuracy and state restoration separate. Direct ARM
  framebuffer output, overlapped flip handling and batched AHI already existed;
  they are not new gains from this pass.

DOSBox's x86 dynamic recompiler and TinyGL/VFP matrix routines do not apply to
this SNES interpreter. No dependency on those engines or math libraries was added.

## Physical measurements

Three interleaved pairs of the first 600 Super Mario World frames after reset
(title/startup sequence) per presentation
mode. Values are median ARM `copy_us / challenges`, not wall-clock FPS. No
bridge polling during the timed playback. The initial/diagnostic readback and
startup are included in elapsed time, so elapsed average is not steady FPS.

| Change / mode | Before, µs per presented frame | After | Less stage time |
| --- | ---: | ---: | ---: |
| Contiguous spans, scanlines Off | 2366.80 | 1684.92 | 28.81% |
| Contiguous spans, Soft | 2534.76 | 1849.04 | 27.05% |
| Contiguous spans, Thin | 3732.84 | 2943.25 | 21.15% |
| O2 core compilation, µs per emulated frame | 6564.94 | 5590.46 | 14.84% |

The O2 row is a separate three-pair comparison against the span-optimised Os
build. Percentages must not be added. At the installed 4:3/Thin settings, two
final pairs measured 3765.27 → 2860.18 µs presentation time, a 24.04% reduction.
Playback elapsed time stayed around 14.2 seconds for 600 emulated frames;
three or four audio-gap events were still recorded. This is not a claim that
Thin now achieves 60 FPS or eliminates all audio stalls.

A separate scripted 1200-frame **F-Zero** comparison entered gameplay:

| Metric | Baseline | Optimised |
| --- | ---: | ---: |
| Elapsed seconds, including startup/readback | 21.112 | 21.129 |
| Emulated frames | 1200 | 1200 |
| Presented frames / acknowledged flips | 945 | 1153 |
| Average presented FPS across the whole run | 44.76 | 54.57 |
| Core time per emulated frame, µs | 11466.67 | 9482.90 |
| Audio gaps | 0 | 0 |

That is 22.0% more presented frames in this one workload. It is not a universal
performance rating; the emulation/audio timeline remains paced.

## Large-library cost on the real 68k

Two runs of the same helper with 20,000 synthetic entries. These are isolated
metadata operations, not complete MUI startup times and not cartridge tests.

| Operation | Previous median | Candidate median | Less time |
| --- | ---: | ---: | ---: |
| Sort | 16.73 s | 4.28 s | 74.4% |
| Write metadata | 11.70 s | 2.23 s | 80.9% |
| Read metadata | 11.20 s | 9.27 s | 17.2% |

The sort-title cache uses 128 bytes per entry temporarily (2.56 MB at 20,000)
and is freed after sorting. Allocation failure uses the original comparator.
No fixed ROM-count limit is introduced. The existing entry structures still
need memory proportional to the library size.

## Correctness and hardware checks

- ASan/UBSan: 1,658,880 sampled output pixels across view options, byte orders,
  masks, borders and PAL height; 20,000-entry library round trip, malformed
  records, numeric overflow, allocation failure and legacy preferences.
- Relocated ARM: pixel equivalence, Thin geometry, revoked mappings,
  cancellation and processor-state preservation.
- O2 versus Os: complete video bytes and PCM match for 600 frames each of
  Super Mario World, F-Zero, PAL Cannon Fodder, Doom and Star Fox 2, plus a
  second relocation. Input changes are included after the initial title sequence.
- Save-state replay and a fresh relocated instance pass with the original
  state fixture. This does not establish special-chip save-state completeness.
- Original FX3 fixture, Star Fox 2 and Doom each completed 600 physical frames
  with active framebuffer verification, Thin window output and clean teardown.
  Readback tests stall playback deliberately and are not FPS benchmarks.
- Successful bounded runs and the final lifecycle reported zero register restoration mismatches/cache witness
  faults, matching queued/submitted/completed audio counts and released memory.

The FX model check now calls the real `S9xReset` entry point; O2 can inline the
internal `S9xResetSuperFX` symbol. Both baseline and candidate pass. Existing
SA-1, expanded-mapping and special-chip state limitations remain; no new game
compatibility is claimed by changing compiler optimisation.

## Reproduce

```sh
make optimisation
make test-optimisation-native
make test-optimisation-arm MODEL_PYTHON=/path/to/unicorn-python
# Supplemental FX player:
python3 scripts/build_current.py --arm --optimisation --core-o2 --superfx
```

Use the pinned toolchain and `ARM_BUILD_COMMAND`/`ARM_CONTAINER_ROOT` described
in the development guide. `LLVM_NM` selects llvm-nm for model tests. Outputs
are `.context/optimisation-o2-build` and `.context/optimisation-o2-fx-build`.
For byte-exact core comparison run `tests/amiga/pocketsnes/check_optimisation_core.py
BASELINE_ELF CANDIDATE_ELF OWN_ROM 600` with Unicorn Python.

[Raw measurements and summaries](../amiga/records/2026-10-18/optimisation/).
Device paths in the public logs are replaced with `TEST:`. ROMs and private
settings are excluded. The archived input manifest changes only the two edited
build/test scripts; archived beta binaries retain their original checksums.

## Interactive checks and recorded failures

Final hardware checks passed pause, save/load, manual snapshot, window/fullscreen
switching and F10 exit. A fresh process then resumed each saved state for 120
frames. NES one- and three-frame runs also completed every queued PCM sample,
exercising shutdown before the new four-frame prebuffer is filled.

An initial SNES interactive attempt exited with code 20 after 14 frames and
pending AHI data; processor restoration and memory release still succeeded.
Quiet window tests (baseline and candidate, FPS on/off) and the later focused,
fully-started interactive test passed. The isolated early failure is retained
in `snes-lifecycle.log` in the SNES evidence; no production fix or definitive
root cause is claimed for it. Hardware tests therefore do not establish that
all startup/audio stalls have been eliminated.

A separate NES test-harness assertion was false: AmigaDOS would not open the
still-redirected log because it was in use. Reading the log after exit confirmed
that save and cleanup had succeeded. The harness now checks the completed state
file instead, and waits for playback before injecting controls. These test
harness corrections do not change the emulator's state format or core.
