# ZZSNES 0.7.13 Beta validation

Release build and physical checks are recorded below. Listening acceptance remains separate.

The release build corrects horizontally flipped partial-tile masks. The portable
byte-swap macro now changes its argument as its callers expect. The combined row renderer also avoids repeated tile setup and clipping work for
complete eight-pixel spans. Guest timing, source picture rows, audio synthesis
and Super FX budgets are unchanged.

With corrected clipping enabled, baseline and row rendering match all video/PCM
frames in five 3,600-frame native runs: Stunt Race FX, Star Fox 2, Super Mario
World, Chrono Trigger and Contra III. Previous hardware row-renderer measurements
showed a scene-specific 5.709% core-time saving; this is not a new measurement
of this combined candidate or a claim that audio starvation is resolved.

The independent clipping oracle passes 25,344 native sanitizer cases and 6,336
Cortex-A9 model cases. Race-state model comparisons validate the current build
against the previously tested corrected candidate. Physical startup tests of
that earlier candidate are recorded separately. Final-build startup and matched
row-rendering checks are linked below; listening confirmation remains pending.

Super FX games including Star Fox 2, Stunt Race FX and Doom can still exceed the single-core audio-production budget. Existing
frame skipping reduces work but trades away pictures and does not eliminate
all starvation. No default frame-skipping, pitch or playback-rate change is made.

Both launcher and player are rebuilt with matching versions. Fresh output hashes
are recorded after success, stale snapshot manifests are excluded, and the
measured 666/800 MHz timer policy is retained. No firmware or clock is changed.

[Recorded candidate checks](validation/release-0.7.13/README.md).
