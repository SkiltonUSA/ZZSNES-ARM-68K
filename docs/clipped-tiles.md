# Horizontally flipped tile clipping

**Development correction; not included in 0.7.12 Beta.** This is a rendering
correctness fix, not an audio or FPS optimisation.

The portable `SWAP_DWORD` macro returned a reversed value, but clipping callers
used it as an in-place operation. Its result was discarded. For horizontally
flipped tiles, partial four-pixel masks therefore affected the wrong pixels.
The ARM assembly version already writes its argument. The candidate makes the
portable version obey the same contract; it does not alter emulated PPU timing.

The independent clipping oracle first draws a complete tile, then checks that a
clipped draw changes only the requested span and matches full-draw colours and
depth inside it. It includes all flip combinations, partial rows/widths,
transparent pixels and depth rejection. The baseline fails at horizontal flip,
start 0, width 1. The correction passes 25,344 native ASan/UBSan cases and 6,336
Cortex-A9 instruction-model cases. Original synthetic tile data only.

```sh
python3 scripts/test_clipped_tiles.py --baseline  # expected failure
python3 scripts/test_clipped_tiles.py
python3 scripts/build_current.py --release-ax --clipped-flip-test
python3 scripts/test_clipped_tiles_arm.py
```

The ARM test requires Unicorn and the ARM compiler/libc headers. Candidate
output is isolated in `.context/clipped-flip-build`; recorded core snapshots
are unchanged. Both ARM and 68k builds complete. A physical 120-frame run of
the original System-Test fixture at 800 MHz completed with three framebuffer
checks, zero display faults, register/cache-witness restoration and owned-memory
release. All 64,165 generated audio samples were delivered without a device
error; four queue starvations were recorded. This is a startup/rendering/cleanup
smoke test, not an audio-quality or game-compatibility result. Released binaries
are unchanged. A further 900-frame startup/intro pass completes Chrono Trigger,
Contra III, Super Mario World, Stunt Race FX and Doom, including windowed and
fullscreen output and saved previews. Queue starvations remain, and gameplay,
scrolling/race scenes and listening acceptance are still pending.
[Game startup checks and limits](validation/clipped-tiles/games/README.md).

Fresh builds include the previously validated 666/800 MHz measured-timer gate.
It does not change the ARM clock, guest timing or configured PCM output.

This does not establish that every reported scrolling artifact had this cause.
It also does not solve Super FX games that exceed the audio production budget.
[Recorded checks](validation/clipped-tiles/results.txt).

[Physical smoke test](validation/clipped-tiles/hardware.txt).
