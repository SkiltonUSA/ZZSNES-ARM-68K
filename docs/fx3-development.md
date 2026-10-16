# FX3 compatibility work

The supplemental `ZZSnes-FX` player now has an experimental FX3 implementation
inside the existing PocketSNES/Snes9x2002 C core. It does not replace that core
with MesenCE. FX3 cartridge notices remain advisory and Play remains available.

Behaviour was reviewed against [MesenCE's FX3 change](https://github.com/nesdev-org/MesenCE/pull/222)
and its source at commit `4b8a196e299b3de297b204fd1208d2981562c89b`:
`Gsu.cpp`, `Gsu.Instructions.cpp`, `Gsu.h`, `GsuTypes.h` and `BaseCartridge.cpp`.
Credit to Randal Linden and the MesenCE contributors for documenting and
implementing this extension. See also [the FX3 project](https://github.com/LimitedRunGames-Tech/snes-fx3).
The local C changes adapt the observed behaviour to PocketSNES's existing
registers, interpreter and planar renderer; MesenCE source is not vendored into
the PocketSNES overlay. Existing Snes9x notices and licence remain in place.

## Implemented behaviour

- Cartridge types `$17` and `$18` select FX3; standard Super FX remains separate.
- CPU register window at `$7000–$7fff`, its mirrors and holes, VCR `$52`, and
  restricted register access while the GSU runs.
- Two 64 KiB RAM banks, with battery-backed exposure only for type `$18`.
- Full ROM mapping up to the existing 4 MiB cartridge limit, without copying
  low-bank mirrors over the upper ROM data.
- Low-bank 32 KiB fetch wrapping, extended ROM and RAM execution, and execution
  without the ordinary RON/RAN gate.
- FX3 MERGE commands 3–5 clear the specified planar tile regions; 0–2 are no-ops
  because this core already draws in planar format. Unknown commands do nothing.
- STOP clears R15 and does not raise a GSU IRQ. This follows the hardware intent
  described in the reference comments, rather than reproducing its IRQ side effect.
- Bounded 1,400/2,800-instruction slices for the two clock selections. This is
  an approximation within the older core, **not cycle-exact FX3 timing**.
- A post-PPU-reset FX3 reset prevents the PPU's FillRAM initialization from
  overwriting the new register state.

## Validation and limits

`make test-fx3` runs native sanitizer tests through the real CPU mapper/MMIO and
GSU interpreter. Tests cover both chip types, all CPU bank timings, ROM integrity,
RAM banks, register mirrors, MERGE commands 0–6, exact tile clear bytes, STOP,
execution from ROM/RAM and bounded looping/cancellation.

`tests/amiga/pocketsnes/check_fx3_arm.py` repeats instruction/MMIO tests on the
Cortex-A9 build at two relocation bases. These are instruction-model results,
not physical cache validation. `scripts/make_fx3_fixture.py` creates an original
SNES ROM that executes MERGE and checks its RAM output before showing a
checkerboard. The old player remains blank; the new core matches the original
checkerboard's video hash after six frames.

The combined player also completed the original fixture on the physical A9 for
600 frames, with the same final checkerboard hash, clean register/cache witnesses
and owned-memory release. See [physical records](../amiga/records/2026-10-16/fx3-thin/).

Native 600-frame runs plus 30-frame save/replay diagnostics for supplied Star
Fox, Star Fox 2, Doom and Super Mario World match the preceding core byte for
byte in video/audio signatures and replay outcomes. Existing Super FX state
replay problems are **not fixed** by this extension.

No commercial FX3 cartridge has been tested. Original Star Fox 2 and original
Doom are ordinary Super FX games; they do not establish FX3 compatibility.
The standard player and published beta are separate from this development build.

Build with `make current-superfx`. The FX3 overlay is applied after the earlier
compatibility overlay. `development/fx3-20261016/core.patch` records the delta
against those inputs with line endings normalized for review.
