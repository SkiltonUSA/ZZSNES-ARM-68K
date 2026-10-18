# Fullscreen FPGA / windowed Soft scanlines

This development update retains the verified ARM payload and changes the 68k
presentation policy. Fullscreen requests plain pixels from ARM, then writes the
mode/parity controls documented by Xanxi. A Workbench window disables that global
hardware effect and requests Soft software filtering (or Off). F7 changes apply
only after outstanding presentation work is drained. Alt+Return reopens the
display at the existing safe boundary. Normal cleanup disables hardware filtering.

The runtime-discovered ZZ9000 board base is used with UWORD mode/parity registers
at offsets 0x100c/0x100e. Parity is zero. These controls are write-only: an installed
utility and a successful write do not prove firmware support or visual output.
FPGA filtering acts on scanout after framebuffer readback; screenshots cannot
validate it. Scanline spacing follows the FPGA's low-resolution line doubling,
not a hard-coded count of 224 dark rows. Saved Thin mode maps to Soft.

The bundled controls are unmodified Xanxi sources at
1707c356056a3a5c3aeb9d46d38b461e62526035, GPL-3.0-or-later.
[Upstream](https://github.com/Xanxi-Amiga/zz9000-scanlines).
The installer checks both canonical and C: locations, installs missing programs,
and keeps existing ones. It never flashes firmware or edits startup scripts.
The emulator talks directly to the registers, so the standalone control program
is not launched for each frame or mode change.

## Build and package

Build the optimisation target normally; it also builds the controls and installer.
To package an existing build, run `scripts/package_current.py --build BUILD_DIR`.
ZZSNES additionally requires `--fx-build FX_BUILD_DIR`. Output is a complete ZIP
under `.context/packages`, with the installer icon, controls, corresponding tool
sources and license. User ROMs, states and Library.cfg are excluded.

## Validation

Native tests cover exclusive backend selection, key repeat/release handling,
fullscreen cycling, windowed toggling, and legacy Thin migration. The unchanged
ARM image is checked by the existing relocated presentation tests. Hardware and
installer results are recorded separately below after testing.

Physical checks passed for NES, standard SNES and Super FX host paths using
private test ROMs: F7 while running and paused, window/fullscreen transitions,
backend isolation, sampled pixel readback and F10 exit. All returned exit zero,
released owned memory, reported zero register mismatches and clean cache witnesses.
These tests establish command/lifecycle behaviour, not optical scanline spacing.
The Super FX player test used the standard SMW fixture to exercise its host;
it is not new evidence for Super FX game compatibility or speed.

The first test script incorrectly expected a final mode of 2 while deliberately
exiting at mode 0; its assertion was corrected. The recorded NES log passes the
correct backend and lifecycle assertions. Native/model tests passed without a
player change. Diagnostic pauses/switches/readback affect timing; no FPS gain or
uninterrupted-audio claim is made for these tests.

Installer tests on the Amiga passed: fresh destination (two installed), repeat
run (zero changed), missing GUI with an existing different CLI (one installed,
CLI preserved), invalid directory in place of a tool (error), and default system
installation (both existing tools kept, hashes unchanged). The first harness
expected `RC=0`; Amiga Echo prints `RC 0`. The harness was corrected and all cases
were rerun in a fresh scratch drawer. Installer code did not need changing.

Installed five executables with checksums verified and previous executables in
`Rollback-fpga-1018`. Library preferences, ROMs and saves were preserved. Versions
remain 0.7.2-dev / 0.6.2-dev. Existing tagged releases are unchanged.
