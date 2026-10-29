# Timed Super FX experiment

Opt-in **0.7.5-exp** launcher/player pair. Normal release builds remain 0.7.4;
Legacy is also the default in the candidate. This is a timing experiment, not
an overclock or a claim of better game compatibility or faster gameplay.

```sh
make test-timed-superfx
make current-timed-superfx
```

Use the established ARM toolchain environment described in the build guide.
Output: `.context/timed-superfx-build/ZZSNES`, `ZZSnes-ui`, `ZZSnes-FX`.
Keep the matching launcher and player together in a separate writable drawer.
In Display → Emulation → Super FX choose **Legacy** or **Timed (experimental)**.
The selection uses existing global/per-game settings. FX3 retains Legacy timing.

## Execution model

- Legacy retains the original opcode implementation and instruction budgeting.
  A second generated opcode table accounts for cycles only in Timed mode.
- Timed runs a nominal 1,364 SNES master-clock budget at each end-of-scanline
  event. GO/R15 writes arm execution, without granting another full slice.
- Fetches distinguish ROM/RAM, loaded cache lines and cache fills. Loads/stores,
  plotting, ROM-buffer reads and multiplies add instruction-specific costs.
  CLSR and CFGR select the guest chip's clock/multiply costs.
- An instruction completes atomically. Its excess cycles are deducted from the
  next slice. STOP preserves IRQ handling and discards unused execution credit.
- This is scanline-granular timing, **not cycle-exact bus arbitration**. Pixel
  costs are approximate. The legacy cache data implementation is retained; the
  new cache mask models timing. No clock multiplier is exposed yet.
- The ARM/68k split, 96 MiB Exec-owned runtime and 32 MiB dispatch reserve are
  unchanged. The actual Cortex-A9 frequency is not changed.

## Save states

The existing version-3 payload layout and pointer-reconstruction logic stay
unchanged. Option bit 23 selects Timed mode; reserved header bytes 72–83 store
`FXT1`, cycle debt and timing cache mask. Loading a state restores its timing
mode. Invalid timing metadata is rejected before modifying the live core.
Legacy candidate states leave these bytes zero and retain the old option word.
Older players reject Timed states; use this matching experimental player to
resume them. Keep experimental saves separate from released installations.

## Provenance

Instruction costs and the timing-only cache-mask approach are adapted from
[Snes9x](https://github.com/snes9xgit/snes9x/tree/1bcc369e89f08243e0a462882fb1f3e42e51de3a),
commit `1bcc369e89f08243e0a462882fb1f3e42e51de3a`, `fxinst.cpp`, `fxinst.h`
and `fxemu.cpp`. See [LICENSE.Snes9x](LICENSE.Snes9x). This does not import the
modern Snes9x core wholesale. The generated interpreter retains the existing
PocketSNES/Snes9x notices. Budget debt, event gating, state extension and launcher
integration are the ZZSNES adaptation.

## Development log

1. Read the legacy scheduler, GSU pipeline/cache, state format and pinned Snes9x
   timing implementation. Identified extra slices from GO/R15 writes.
2. Added an original synthetic cartridge/timing fixture before implementation;
   initial run failed because the new overlay did not exist.
3. Added isolated timed tables, bounded slices, debt and state metadata.
4. Cycle-cost tests caught plotting still selecting the hard-wired legacy table;
   switched plot selection to the active table. Legacy continues selecting its
   original functions.
5. Added launcher selection and profile persistence; built matching 0.7.5-exp
   components. Normal build inputs and release version stay unchanged.
6. Validated native synthetic timing, four-game Legacy equivalence and timed
   video/PCM save-state replay.
7. Relocated ARM replay and backward Legacy-state loading pass at two addresses.
8. Ran four bounded physical tests and the compact MUI/profile probe. Verified
   teardown and restored the test environment by removing only the private drawer.
   Installed 0.7.4 and global preferences remain unchanged.
9. Recorded hashes/results and backed up the source and documentation to GitHub.
   Visual/listening acceptance and timing accuracy remain explicitly unclaimed.

[Validation report](../../docs/timed-superfx.md).
