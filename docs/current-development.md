# Current beta development

## Architecture and transferred optimisations

- Keep the accepted console CPU/PPU/APU cores, PAL/NTSC cadence, AHI batching and
  deferred framebuffer handoff. DOSBox's x86 JIT and TinyGL matrix routines do
  not apply to these console cores.
- Present into an allocation-owned ZZ9000 bitmap. Fullscreen scanout consumes
  completed slots without sending full pictures through 68k RAM. Workbench
  windows use an OS blit from the private bitmap.
- Transfer ZZDosBox's guarded SIMD state handling and contiguous NEON row stores.
  Check existing firmware permission, preserve D0–D31/FPSCR, and retain scalar
  fallback. No CPACR/FPEXC changes or floating-point emulation shortcut.
- Fuse software scanline byte dimming with NEON stores. Classic black rows skip
  unnecessary colour conversion; core CPU/APU frames are still emulated.
- Retain cached scaling/palette work and reduce routine gameplay file logging
  from every 60 to every 600 frames.
- Change displays only after the pending frame is acknowledged; keep ownership,
  bounded mappings, cancellation, cache witnesses and return/reset-before-free.

SNES selection: the original STMIA row copy won the no-scanline comparison.
A NEON-copy-only trial was about 2.4% slower in ARM presentation, so that change
was rejected. NEON remains enabled for Soft/Gradient dimming, where it helps.
The rejected trial logs are retained beside the accepted results.

## Measurements

Physical A4000TX / TF4060 / ZZ9000 Cortex-A9, XX19c / XACP 1.7, AmigaOS 3.2.3.
Three interleaved runs per setting, 600 startup/title frames per run. Compare
`DIRECT_END.copy_us / challenges` (mean ARM presentation time per submitted
picture), then take the median across runs. This includes startup conversion
work. It excludes core emulation, host scheduling, AHI and scanout wait time.
Do not equate its percentage change with an FPS increase.

Raw logs and a machine-readable table are in `amiga/records/2026-10-15/refresh/`.
The older 59.893/59.920 FPS README entries are established steady emulation
measurements, not new claims derived from these presentation comparisons.

| Presentation | Baseline µs | Optimised µs | Reduction |
| --- | ---: | ---: | ---: |
| Plain output | 2976.97 | 2977.10 | Unchanged |
| Soft scanlines | 3787.29 | 3148.88 | 16.86% |

## Verification

- Native ASan/UBSan: settings migration, path deduplication, scanline arithmetic
  and FPS glyph bounds.
- Relocated Cortex-A9 instruction model: byte-exact pixels against the previous
  ARM payload, both native formats, borders/masks, all scanline styles, NEON
  permission/fallback, SIMD restoration, mapping revocation and cancellation.
- Physical bounded display runs: fullscreen/window, FPS, each scanline style;
  register/cache witnesses and owned-memory release checked.
- Physical interactive runs: paused save/load/capture, window/fullscreen return,
  normal exit. Capture while paused: an earlier whole-Workbench live capture
  caused a request timeout with clean teardown; that failure is retained.
- MUI pages, launch/return, title rename, deletion confirmation and mounted-drive
  discovery are checked separately from instruction-model tests.

These checks do not establish tear-free scanout on every monitor, universal ROM
compatibility, visual/audio acceptance by a person, or other firmware support.

## Rebuild

`make verify-current` checks archived source/artifact hashes. `make current`
rebuilds the 68k player, MUI library and icon helper around the tested ARM image.
`make current-arm` also recompiles the ARM payload. Neither deploys or launches
hardware. Player sources/inputs remain in `development/beta-20261015/`. Current launcher
sources/inputs are in `development/library-20261016/`; historical snapshots stay
unchanged. The launcher update does not change the ARM player or its measurements.

Requirements: Python 3, the pinned `amigadev/crosstools` image recorded in
`docs/current-build.json`, and Podman (or Docker via `BETA_CONTAINER_COMMAND`).
For model tests install Unicorn 2.1.4 and provide `llvm-nm`; run
`make test-current MODEL_PYTHON=/path/to/python`, setting `LLVM_NM` if necessary.

ARM rebuild: GCC 15.3.1/binutils for `arm-none-eabi`, soft-float ARM mode.
The prepared PIC picolibc archive and its sources/licences are included in the
repository. A local toolchain is used by default; `ARM_BUILD_COMMAND` can be a
JSON container command prefix with the repository mounted at `/work`.
Set `ARM_CONTAINER_ROOT` if that container mount uses another path.

Full standalone ARM rebuilds were compared with the tested payload after
packing and relocation extraction; payload bytes and relocation lists matched.
Debug paths can differ in rebuilt 68k files, so published artifacts retain their
own checksums.

## Large libraries

The launcher now grows entry storage and the visible-row index dynamically.
Case-insensitive hash indexes replace repeated linear duplicate searches, and
`qsort` replaces insertion sorting. MUI copies row strings, so the old fixed
row-text array is removed. List refreshes suppress intermediate redraws. Search polls the edited string
with a 200 ms typing delay because this MUI version does not notify Contents for
each keystroke; Return also applies the filter.
There is no configured ROM-count ceiling; allocations are checked against
available memory and the platform's address/index representation.

20,000 synthetic entries passed native ASan/UBSan checks for persistence,
search/filter/sorting, rename, duplicate lookup, removal and allocation failure.
Both physical MUI launchers loaded and saved 20,000 entries unchanged, including
selection beyond the old cap. This exercises library metadata, not cartridge
compatibility for 20,000 games. First opening took about 27 seconds on the test
machine; this is not an instantaneous-loading claim.

Run `make test-library-large`. New evidence is in
`amiga/records/2026-10-16/large-library/`. The v1/v2/v3 settings formats and existing
save keys remain compatible. Allocation failure does not silently replace an
existing settings file. Scanning retains accepted entries if memory runs out.
