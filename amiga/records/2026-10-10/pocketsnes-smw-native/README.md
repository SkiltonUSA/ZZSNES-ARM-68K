# Super Mario World native reference — 10 October 2026

The owner supplied a 512 KiB ordinary LoROM/SRAM cartridge. Its bytes remain
in the ignored local attachment/test directories; they are not in this backup.
The native PocketSNES test completed 600 frames with ASan/UBSan, changing
256x224 pictures and active PCM. Start is pressed for eight frames at 360 and
480; no directional input is supplied. This is a startup/menu workload, not
a sustained gameplay test or an A9 timing measurement.

The first sanitizer run stopped in `soundux.c` at a left shift of a negative
BRR sample. The test builder applies the recorded experimental patch: signed
nibble scaling and filter doubling use multiplication instead. The rerun
passes through frame 600 with 320,813 stereo sample pairs and 327,934 nonzero
interleaved sample values. Final hashes: video `edf1acde`, cumulative PCM
`a2053b75`. No sound was sent to an audio device.

This patch is confined to the game-test builder. The saved hardware-tested
checkerboard executable and its 4.6166 ms result are unchanged. The next step
is bounded ARM execution with matching graphics/audio checkpoints before
adding fullscreen presentation and AHI playback.

Run `python3 scripts/test_pocketsnes_game.py /private/path/game.sfc`.
The harness emits no cartridge bytes, only checkpoint hashes. See result.json,
game-native.log and active-audio.patch for evidence.
