# 0.7.13 candidate validation

The final candidate combines corrected portable flipped-tile masks and the row
renderer. It does not change timing, audio synthesis, Super FX budgets or skip
pictures. The GSU counter experiment remains opt-in.

- Native ASan/UBSan: synthetic raster/depth/window checks and 3,600 matching
  video/PCM frames for each of five supplied games, with corrected clipping on
  both comparison sides. ROM order/hashes are in rom-identities.json.
- Exact candidate Cortex-A9 model: 120 frames each from Stunt Race FX race and
  Star Fox 2 checkpoints match video, stereo PCM and serialized GSU state against
  the corrected baseline at two candidate relocation addresses.
- Independent pixel/depth oracle compiled from this candidate: 6,336 ARM cases.
- Fresh ARM/68k build and artifact SHA-256 checks pass. See build.json.

Reproduce native comparisons with `scripts/test_ppu_experiment.py --clip-fix
--mode rows --frames 3600 --rom <supplied-rom>`. Use test_ppu_arm.py and
 test_clipped_tiles_arm.py with the recorded candidate directory for ARM checks.
No ROMs, audio recordings or save states are distributed in these records.

A prior corrected baseline had passed physical startup and lifecycle checks.
The initial new physical run was bounded to 18,000 frames, but its screenshot
request timed out. Cleanup could not be queried after the gateway paused, so
that run is NOT a pass and the lease remains held pending target inspection.
It used the correction-only build, before the final row integration.

Final physical gameplay/listening, installation, clean exit and publication remain
pending. Public 0.7.12 and installed production binaries are unchanged. Existing
Star Fox 2 audio starvation remains an explicit limitation; neither these model
checks nor the historical row-renderer saving establishes that it is resolved.
