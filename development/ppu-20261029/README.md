# Raster experiments

Generated source changes live in `scripts/prepare_ppu_experiment.py`; archived
core sources stay unchanged. See [methods and results](../../docs/ppu-profiling.md).

`tests/raster_fixture.c` generates synthetic sprite and PPU-window cases.
`tests/game_trace.c` records per-frame video and stereo PCM from a supplied ROM.
Neither contains game assets. Build variants remain separate from release output.

## Work log

1. Inspected the corrected 0.7.4 renderer and the Snes9x-Z reference. Found
   existing tile-row batching and colour-table caching; narrowed the candidates
   to repeated setup and renderer selection rather than a renderer replacement.
2. Added an instrumented baseline. Checked its video/PCM output against the
   original before interpreting operation counts.
3. Added deterministic sprite/window fixtures before the candidate edits.
   Derived fixed-colour full-strength functions from the existing core routines;
   reused horizontal sprite clipping; narrowed clipping invalidation and reused
   sprite renderer selection only inside one draw call.
4. Compared each isolated candidate and the combination under sanitizers:
   8,192 window writes, 2,048 sprite scenes, and 3,600 frames in each of four games.
   Re-ran cartridge, Super FX colour/state and option regression checks.
5. Built five ARM variants; confirmed the baseline packed ARM payload equals
   the released payload. Compared resumed ARM video and stereo PCM at two
   relocation addresses. Increased only the model's per-call instruction budget
   for GSU-heavy Star Fox 2 frames; emulation settings were unchanged.
6. Measured bounded, sequential physical runs from private checkpoints, using
   the existing runtime mapping, cache/MMU and audio teardown checks. Corrected
   an initial log-parser expectation (`STATE load OK`); the completed run itself
   had passed. No installed player, firmware or system preference was changed.

The optional UB complexity/duplication/mutation tools were unavailable. Actual
renderer output comparisons and sanitizer/model/hardware checks provide the
verification recorded here.
