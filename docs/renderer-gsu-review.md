# Renderer and Super FX optimisation review

The `rows` renderer remains an opt-in development build. Repeated physical
Star Fox 2 tests confirm a saving in ARM core time, but the tested scene still
starves the audio output. This is not a general FPS or sound-quality claim.

In matched ABBA runs, rows reduced core time by 5.709%; the separate GSU counter
change reduced it by another 0.209% relative to rows. Chrono Trigger's opening
sequence showed no material renderer gain. These are workload-specific results.

## Changes

`--ppu-test rows` combines the ordinary and clipped RGB565 row paths and sends
complete eight-pixel spans to `DrawTile16`. It preserves palette, transparency,
flip and depth behaviour; it does not discard logical image lines or audio.

The separate `--gsu-counter-test` experiment keeps the legacy Super FX
instruction budget in a local variable across opcode-handler calls. The A9
compiler holds it in `r9`, removing a counter load and store from each loop
iteration. STOP still clears the global counter and G bit; exhausted budgets
retain the original unsigned wrap. Instruction order, ROM reads, cycle budgets,
interrupts and the saved-state format are unchanged. This is not a Super FX
overclock or a new translator.

`prepare_gsu_counter.py` checks the known counter access sites before applying
the transformation. A future handler that observes or modifies the counter
must be reviewed before using this optimisation. Snapshots are taken outside
the execution loop.

## Checks

- Renderer: synthetic raster cases and 1,200-frame video/stereo-PCM comparisons
  for Chrono Trigger, Kirby Super Star, Pilotwings, Final Fantasy III, Doom and
  Contra III passed with ASan/UBSan. These are bounded opening sequences, not
  complete-game compatibility tests.
- Super FX counter: 33,280 loop-boundary cases cover STOP, zero/exhausted
  budgets, inactive GSU, unsigned wrap, ROM-read count and return values.
- Star Fox 2, Stunt Race FX and Doom: 3,600-frame native comparisons match video,
  stereo PCM and the complete serialized GSU state on every frame.
- Relocated ARM: 30 frames each from Star Fox 2 and Stunt Race FX checkpoints match video, PCM and
  serialized GSU state at two model addresses. Model addresses are not
  physical ZZ9000 allocations.
- `make test-native` and `make verify` pass. These checks do not replace hardware
  restoration, display or audio tests.

## Reproduction

From this repository, using the configured ARM/68k toolchains:

```sh
python3 scripts/build_current.py --release-ax --ppu-test rows
python3 scripts/build_current.py --release-ax --ppu-test rows --gsu-counter-test
python3 scripts/test_gsu_counter.py --rom /path/starfox2.sfc \
  --rom /path/stunt.sfc --rom /path/doom.sfc --frames 3600
python3 scripts/test_ppu_arm.py /path/starfox2.sfc /path/starfox2.state \
  --baseline .context/ppu-rows-build \
  --candidate .context/ppu-rows-build-gsu-counter --frames 30 --gsu-state
```

The game inputs and checkpoint are private test inputs, not included here.
`profile_renderer_arm.py` records executed ARM block instruction slots and
function entries. Predicated instructions count as slots; the results are not
Cortex-A9 cycle timings, cache measurements or FPS.

The installed release and default build flags are unchanged. See the
[test record](../amiga/records/2026-11-11/renderer-gsu/README.md) for individual
hardware runs, build hashes and limitations.
