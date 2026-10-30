# ZZSNES shared PCM publication — 0.7.11

[Technical explanation](../../../../docs/arm-audio-publication.md) and
[release validation](../../../../docs/release-validation.md).

`builds.json` identifies the private before/after capture executables and the
release player. `chrono-after.json` identifies the exact ARM-model reference.
The comparison JSON records full sample comparisons after endian conversion;
PCM, ROMs and save-state data remain private. Capture builds add recording
overhead and are not performance benchmarks. `hardware.json` and the named
logs cover the uninstrumented release player's lifecycle tests.

Native/current fixtures retain their historical scope. The new generated-page
policy, current stereo capture and fresh relocated save-state replay are tested
separately; a model pass is not physical cache validation.
