# FX3 and thin-line checks

Native FX3 instruction/MMIO tests passed under ASan/UBSan. The new A9 image
passes the original instruction tests at two relocation bases. Ordinary Star
Fox, Star Fox 2, Doom and SMW retain the preceding native video/audio signatures
and save-replay outcomes after 600 frames; existing replay failures remain.

The original FX3-instruction ROM is reproducible with
`scripts/make_fx3_fixture.py`. It shows a checkerboard only after checking VCR,
executing MERGE and inspecting the resulting RAM/STOP state. Native pre-fix
control stays blank (`b87d5dc5`); the candidate and ordinary checkerboard match
(`08285dc5`). The physical A9 candidate also reports `08285dc5` at frame 600.
This is real extension execution, not just an FX3 cartridge-header test.
It is not evidence of commercial FX3 game compatibility.

The 640×480 Thin presenter passes full pixel comparisons against intact doubled
rows, both pixel formats, two relocation bases, optimisation on/off, row-padding
guards, three-section MMU revocation and cancellation. Existing modes retain
their exact previous pixels. Native bounded-map tests also pass.

Physical runs:

- Thin fullscreen SMW: 600 frames, followed by 1,200 frames using the existing
  snapshot. Correct 640×480 allocation, pixel readback, acknowledgements and exit.
- Thin window → fullscreen → window, F9 snapshot and F10 exit passed.
- Combined Thin/FX3 player: original fixture 600 frames; Star Fox 2 and Doom
  120-frame startup/lifecycle checks. These short runs are not gameplay validation.
- All successful runs report zero register mismatches, zero cache-witness
  failures and owned-memory release. No Core1 owner remains after each run.

## Timing scope

The 1,200-frame Thin fullscreen run recorded 1,106 completed flips and
24.275389 seconds overall: 49.43 emulated frames/s and 45.56 displayed frames/s
over the entire run. The post-warm-up loop mean was 16,653 µs (about 60.05 loops/s).
These are different measures. Startup/readback and occasional picture omission
are included in whole-run figures. No general 60 displayed FPS claim is made.

The first 600-frame runs included automatic snapshots: Classic 11.346403 seconds,
Thin 15.877898 seconds. Their ARM presentation totals were 918,601 µs / 576 frames
and 2,943,454 µs / 557 frames respectively. Thin writes four times as many output
pixels and costs more. The purpose is text fidelity, not an optimisation claim.
An idle launcher with automatic drive scanning was present during these checks.

## Test/deployment recovery

The first interactive capture ran before the game window appeared, so it captured
Workbench. An input call then failed argument validation before injecting any key.
The corrected run waited for the window, focused it and passed both switches,
capture and clean exit; the failed attempt is not counted as a switch test.

Deployment verification caught the bridge's large file-write truncation while
updating Library.cfg. The launcher was still closed. The complete preserved file
was restored via HTTP, with only the selected scanline value changed to Thin,
and compared byte-for-byte after transfer. `installed.json` records the hashes.
ROMs and save data were not modified by installation. Previous files/settings
are retained in the application's Rollback-thin-1016 drawer.
