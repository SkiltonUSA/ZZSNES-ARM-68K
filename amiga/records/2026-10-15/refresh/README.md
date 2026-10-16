# Presentation and launcher verification

The accepted beta uses the final selected presenter. `*-bench-*` logs are three
interleaved 600-frame runs per setting. `measurements.json` records the calculation;
`analyze.py` reproduces the medians from logs in this directory.

`*-lifecycle.log`: physical paused save/load/capture, display changes and clean exit.
`*-pal.log`: 600-frame PAL run. `*-ui-player.log`: asynchronous MUI launch, save,
capture and parent-window return. Display logs cover window/fullscreen, FPS and
Classic/Soft/Gradient effects. Register/cache witnesses and owned allocation
release are recorded by the launcher. These are not subjective audio/tearing tests.

Earlier attempts are retained where available: the NES whole-Workbench capture
interrupted a live run and caused a request timeout with clean teardown. Test
harness focus and partial-log issues were corrected before the final lifecycle.
The SNES `trials/snes-neon-copy` directory is the rejected slower plain-copy trial.

The native/model checks cover settings migration, mounted-volume discovery,
alias deduplication, cancellation, pixel equivalence, SIMD state preservation and
mapping revocation. Physical scanner testing found a ROM on RAM Disk while a
mounted USB optical volume was also present; Stop scan returned control. ROMs,
saves and live preferences are excluded from this repository.
