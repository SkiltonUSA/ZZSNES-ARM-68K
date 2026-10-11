# frank-snes ARM reference review — 10 October 2026

Reviewed [rh1tech/frank-snes](https://github.com/rh1tech/frank-snes) at
`a88025e6a28b996cf2aaf4f6243804a5e7310b0c`. Research checkout is local and
unmodified. No frank-snes source has been copied into the Mesen2 integration.

It is a SNES application for RP2350/Pico 2 boards using Snes9x/snes9x2010,
not a Mesen2 port. Its Cortex-M33-specific assembly, PIO/HDMI, PSRAM placement,
I2S and optional second-chip sound transport need different implementations
on the ZZ9000. The current XACP app owns Core1 only; the C2 design is not
permission to commandeer the ZZ9000's firmware core.

## Useful techniques

- [Tile rendering](https://github.com/rh1tech/frank-snes/blob/a88025e6a28b996cf2aaf4f6243804a5e7310b0c/src/snes9x/tile.c)
  has lookup-based planar conversion, batched four/eight-pixel drawing and
  opaque-row paths. This supports testing wider scalar operations and less
  repeated per-pixel work, as in our NES palette packing optimisation.
- [Tile conversion interfaces](https://github.com/rh1tech/frank-snes/blob/a88025e6a28b996cf2aaf4f6243804a5e7310b0c/src/tile_convert_opt.h)
  expose caching and explicit VRAM-range invalidation. Any Mesen adaptation
  needs to preserve partial scanline fetches, VRAM writes, scroll/mosaic/flip
  effects and internal PPU state; a static checkerboard alone cannot validate
  a general decoded-tile cache.
- [CPU assembly](https://github.com/rh1tech/frank-snes/blob/a88025e6a28b996cf2aaf4f6243804a5e7310b0c/src/snes9x/cpu_asm.S)
  optimises mapped-memory accesses and places hot code in a time-critical
  section. The transferable idea is reducing dispatch/lookup overhead; its
  hard-coded Snes9x structure offsets and M33 Thumb code are not drop-in A9
  routines. We already use the verified private-L1 cache policy on the A9.
- [C2 sound design](https://github.com/rh1tech/frank-snes/blob/a88025e6a28b996cf2aaf4f6243804a5e7310b0c/docs/C2_SOUND_SPLIT.md)
  overlaps work and measures exchange cost. Our accepted NES host/AHI batching
  is relevant once the SNES presentation/audio pipeline exists. The current
  headless test has no AHI or flip waiting to remove.

## Comparison limits

The [build](https://github.com/rh1tech/frank-snes/blob/a88025e6a28b996cf2aaf4f6243804a5e7310b0c/CMakeLists.txt)
turns its reduced-quality fast mode on by default. The
[settings](https://github.com/rh1tech/frank-snes/blob/a88025e6a28b996cf2aaf4f6243804a5e7310b0c/src/settings.c)
default to frameskip level 2; runtime settings can override it. Consequently
an emulated 60 Hz rate cannot automatically be read as 60 distinct displayed
frames. The C2 document's reported game results are on two RP2350 chips at
504 MHz and are upstream measurements, not our measurements or an A9 prediction.
Our current comparisons render every fixture frame and retain SPC/DSP execution.

The [license file](https://github.com/rh1tech/frank-snes/blob/a88025e6a28b996cf2aaf4f6243804a5e7310b0c/LICENSE)
separately identifies GPL-3-or-later application code and the Snes9x core's
noncommercial terms. A future Snes9x comparison should remain a separately
attributed integration with its actual license; do not label copied core code
as Mesen2 GPL code.

## Practical next steps

The first carried-over NES technique is an independently written scalar
32-bit memcpy/memset implementation in `system/memory.c`, with byte handling
for unaligned heads/tails and mismatched copy alignment. It is not a copy of
frank-snes and does not introduce ARM assembly, NEON or a tile-cache shortcut.

Next profile finer PPU stages before changing tile decoding. Expand fixtures
for scroll, transparency, mid-scanline writes, sprites and active sound before
accepting more aggressive paths. A separate Snes9x headless comparison is also
worth considering, similar to our NES Mesen-versus-FCEUX core comparison, but
has not been built or measured here. This reference does not establish
full-speed SNES on the ZZ9000.
