# Super FX timing investigation

Candidate core only; the published 0.6.1 Beta and normal installation are unchanged.

## Finding and fix

The Super FX mapper left the ROM half of banks 00–3F without initialized speed
entries. The observed first-load speed was zero. The candidate extends the
existing timing loop from eight to all sixteen blocks per bank, leaving I/O at
six master clocks and normal ROM/RAM at eight.

An instruction trace compared the old core with Snes9x2005 revision
`a79dfe9047e7fec58808aefe48ad2bf499c7af11`. The first 23,739 CPU-to-APU writes
matched in address/value. At the last command, the old core sent $0F before
SPC initialization cleared the input ports; the reference sent it afterward.
The mapped CMP/BNE wait loop took only 6+6 clocks in the old trace, compared
with 30+22 in the reference. The ROM-timing fix resolves the lost command
without a game-specific workaround. `reference.c` is the independent native
libretro harness; the upstream core stays external.

## Reproduce

From the repository root, `make test-superfx-timing` builds the original mapper
and CPU-instruction fixture with ASan/UBSan. `--baseline` deliberately fails on
the first uninitialized ROM speed. The recorded failure is a negative control.
Native test summaries include the historical core fixtures; the new candidate
is covered by the dedicated timing test and cartridge surveys.

For each provided cartridge, run `scripts/probe_cartridges.py` with
`--allow-experimental-chips --frames 600`, then `--frames 2400`. Each writes a
state, compares thirty subsequent video/PCM hashes, and restores in a fresh
process. Exit 3 denotes replay divergence, not a passed compatibility check.
Logs and JSON for all eleven cartridges are in the numbered directories.
No cartridge, save-state or commercial screenshot data is included here.

Six non-SuperFX cartridges at both checkpoints retained identical execution
and replay results from the previous candidate. Star Fox and Star Fox 2 now
produce changing video and run GSU code. Both still fail video state replay.
Doom's two checkpoints pass; other remaining failures are documented in the
compatibility guide. No complete-game or performance claim follows.

## A9 checks

The candidate rebuild passed the existing two-base presenter model, covering
pixels, scanlines, NEON/fallback, SIMD preservation and cancellation. The
state model used the original 32 KiB fixture with only cartridge type changed
to $13 (Super FX), verifying six startup frames and thirty replay frames at
two relocation bases, plus corrupt/wrong-ROM rejection. This fixture does not
execute GSU instructions and does not prove special-chip state completeness.

Build/source hashes are in `build.json`. Native instruction models do not
verify physical cache maintenance or display behaviour.

## Physical A4000TX checks

Star Fox 2, Star Fox and Doom each completed 1,200 frames with the new player
in windowed mode, FPS overlay enabled, scanlines off and no scripted game input.
All reported exit 0, no register mismatches, intact cache witnesses and release
of owned RAM. These are startup/display/audio lifecycle runs, not full gameplay
or a save-state compatibility pass. Audio completion does not verify listening
quality. Logs are included; the main emulator installation was not replaced.

A second 1,200-frame Star Fox 2 run also exited cleanly after a window capture
showed its starfield intro. The private capture is not a rendering-accuracy
or sustained-FPS benchmark. No Core1 owner remained after the runs.
