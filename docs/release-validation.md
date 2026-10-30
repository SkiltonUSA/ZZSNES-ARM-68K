# ZZSNES 0.7.11 Beta validation

Matching launcher, standard player and FX player; [build hashes](release-build.json).
The two player executables are identical. This update changes shared PCM mapping
and audio delivery, without changing the SNES mixer or save-state format.

## Shared PCM

Chrono Trigger, 600 frames: **641,658 individual samples match the exact ARM
reference**, after byte-order conversion. Previous cached publication produced
46,129 differences; 46,101 matched the preceding frame. Both runs reported zero
starvation, showing why delivery counters were insufficient.
[Technical explanation and reproducible comparison](arm-audio-publication.md).

## Release-build hardware tests

Results are listed in the [audio-publication records](../amiga/records/2026-10-30/audio-publication/).
The Chrono Trigger pause/save/load run and fresh-process resume use Paula HiFi
22,030 Hz. Super Mario World uses AX 32,000 Hz in a 2× Workbench window. An
older version-3 Super Mario World state is tested separately. These runs use
the release executable, without diagnostic PCM capture.

## Local checks

- All 64 allocation alignments: shared PCM pages, adjacent private pages,
  mailbox/ROM policy and bounds.
- Actual audio worker/API adapter: ordered stereo buffers, pause/re-prime,
  drains, device startup fallback, cancellation and teardown.
- Fresh relocated ARM instances: exact 30-frame stereo/video replay and corrupt
  state rejection, including a previously saved version-3 state.
- Native core and expanded-cartridge/state regressions, controller tests,
  compact launcher layout and the existing current-model suite.
- ZIP/LHA equality, executable versions/hashes, signed offline catalogue,
  dependency-first installation and private-data exclusions.

The first test staging attempt hit the bridge's checksum timeout on an
uncompressed 4 MiB ROM. The bridge remained responsive; retrying with the
original ZIP passed transfer verification. This was not counted as a game test.
A stale ELF from a previous host-only build was rejected by the model packer;
the accepted reference uses the newly rebuilt release ELF.

## Limits

These results prove the tested digital PCM transfer and lifecycle. They do not
measure the physical DAC or establish clean sound for every cartridge. Listening
to the new build remains to be confirmed. No FPS improvement is claimed, no
firmware/global AHI changes are required, and the NES-specific HQ synthesis
optimisations are not part of this SNES update.

[Previous 0.7.8 validation](release-validation-0.7.8.md).

## Release lifecycle results

| Test | Frames | Completed stereo sample frames | Starvation |
| --- | ---: | ---: | ---: |
| Chrono Trigger: fresh-process resume | 120 | 64,166 | 0 |
| Chrono Trigger: Paula fullscreen, pause/save/load | 1,800 | 962,488 | 0 |
| Super Mario World: older v3 state, Paula | 120 | 64,166 | 0 |
| Super Mario World: AX, 2× window | 1,800 | 962,488 | 0 |

All runs completed queued PCM and passed register/MMU/cache restoration and
owned-memory release. Silence during deliberate pauses is not an underrun.

## Release assets

| File | Bytes | SHA256 |
| --- | ---: | --- |
| `ZZSNES-0.7.11-dev-beta-amipkg.lha` | 3116973 | `845112366fcf29d685a696c816f64938cd9e382b7a6ed925d0497fba1f2ed7a2` |
| `ZZSNES-0.7.11-dev-beta.zip` | 1165460 | `db7130d7c8dd0f71b3da357a95c15e31013d98da4e308c43c64e0156680bb36a` |
| `ZZSNES-release.json` | 1117 | `191f332f651e6fd7d865b0bf08dfa7a4a19f1a0be7237663d488f50de2ce1500` |

## Installed verification

The offline installer completed on the A4000TX. All three installed executable
checksums match the release build. Existing Library.cfg, Library.bak,
Session.cfg, Audio.cfg, Controls.cfg and Controller.cfg checksums were preserved.
Previous executables, icon, documentation and settings were backed up and
verified first. Global AHI preferences, firmware, ROMs and save files were not
modified by installation.
