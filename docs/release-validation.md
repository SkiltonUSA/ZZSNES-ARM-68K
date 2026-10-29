# ZZSNES 0.7.4 Beta validation

Matching **0.7.4-dev** launcher, normal player and FX player. This release fixes
Super FX race geometry and legacy PPU colour-window handling. Normal regional
pacing, register dispatch and automatic per-app AX audio are retained.

[Build command and executable hashes](release-build.json) ·
[Investigation and limits](stunt-race-fx.md).

| Asset | Bytes | SHA256 |
| --- | ---: | --- |
| `ZZSNES-0.7.4-dev-beta.zip` | 1117808 | `658c88afc4d0627c05cffb75cd138b9fda489dd4090c6d266c66578a9ef41750` |
| `ZZSNES-0.7.4-dev-beta-amipkg.lha` | 3024442 | `7f15624087664eb68fa6cc26117e324280948fd10683fcf2ba15c5d10c8b2496` |
| `ZZSNES-release.json` | 1116 | `624ed5aaf4ebca56257f1decc81f5e09f52b4ce9e11f4cde29f7a309e9d89eb9` |

## Checks

- Original synthetic cartridge: 8,192 pixel-transparency cases, adjacent pixels,
  two-bank drawing, mirrored execution and live PPU main/sub colour/window masks.
  Failing cases were reproduced before the fixes; native sanitizers pass.
- Expanded-cartridge, FX3, preference and battery tests pass.
- Stunt Race FX, Star Fox 2, Doom and Super Mario World each replay 30 frames with
  exact video and stereo-PCM results after save/load, including a fresh process.
- The built ARM core resumes a physical race checkpoint at two runtime relocation
  addresses with identical corrected video/PCM output.
- A fresh A4000TX race run completes 3,940 frames and passes window/fullscreen
  captures, pause/save/load/resume/quit. All 2,106,779 PCM frames complete;
  the log reports zero audio gaps/errors, register/MMU/cache-witness restoration
  and owned-memory release.
- ZIP and signed LHA contain identical tested binaries. Payload/member hashes,
  catalogue authentication, signature-corruption rejection, prerequisites, icons
  and ROM/save/config/private-key exclusions pass.
- The exact offline installer is installed on the A4000TX. Executable hashes
  match the tested build; Library/Controls/Session settings remain byte-identical.
  Original executables and settings are backed up privately.

Core1 is idle and the bridge responds. The private test drawer was removed;
live AHI and saved Workbench screenmode checksums are unchanged. A final rebuild
after trimming inherited comment whitespace matches all tested executable hashes.

These are bounded correctness/lifecycle checks. No new FPS improvement,
full-game completion or listening/long-session audio-quality result is claimed.
The earlier unexplained NES freeze is outside this SNES fix.

[Evidence and action log](../amiga/records/2026-10-29/stunt-race/).
