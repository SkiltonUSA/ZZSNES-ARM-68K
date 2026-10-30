# ZZSNES 0.7.8 Beta validation

Matching 0.7.8-dev executables; [build command and hashes](release-build.json).
These are functional and scheduling tests, not a general FPS or audio-quality guarantee.

| Asset | Bytes | SHA256 |
| --- | ---: | --- |
| `ZZSNES-0.7.8-dev-beta-amipkg.lha` | 3045025 | `917e625dfd9eb34b8ae90071760fb5d1faa504d303e475d634c9cbde3450313c` |
| `ZZSNES-0.7.8-dev-beta.zip` | 1128159 | `83df478b03deb0a2203a5ac278f84dbe0631a75408bc1efeb865189616fb22d7` |
| `ZZSNES-release.json` | 1116 | `125b44dcc75f67bdc891258fb352774cddfe3e351e3f75ac4c043717d598e17c` |

## Hardware comparisons

| Test | Frames per emulator | Result |
| --- | ---: | --- |
| Nine-frame prime, NTSC fullscreen, Paula 22.05 kHz | 6,000 | Zero starvation transitions; before capture-drain addition. |
| Nine-frame prime, NTSC fullscreen, Paula 28 kHz | 1,800 | Zero starvation transitions; before capture-drain addition. |
| Nine-frame prime, NTSC 2x and 3x, Paula 22.05 kHz | 1,200 per size | Zero starvation transitions; before capture-drain addition. |
| Warm-preview PAL fullscreen | 3,000 | Zero starvation transitions. |
| Final build, cold-preview PAL fullscreen | 3,000 | Zero starvation transitions; capture deliberately pauses playback. |
| Final build, cold-preview PAL 3x | 600 | Zero starvation transitions; capture deliberately pauses playback. |
| Final Auto / configured AHI / explicit AX | 180 per route | Correct route, all PCM completed, clean teardown. |

The earlier six-frame per-app prime still recorded shortages. Nine-frame priming
was retained with twelve producer slots. The original configured-unit-0 path
uses a different linked-request counter, so it is not compared numerically with
the callback starvation counter. Cold-capture silence totals are not zero:
queued PCM is drained before capture and playback re-primes afterwards.

Final interactive tests cover pause, screenshot, state save/load, 3x-window to
fullscreen and back, resume and F10 exit. Both complete all PCM and restore
register/cache/MMU witnesses before freeing owned memory. The interactive
sequence records five starvation transitions on NES and one on SNES; it does
not prove uninterrupted sound while switching or pausing.

## Native, model and interface checks

- Sanitizers cover the actual audio worker/API adapter, FIFO order and reuse,
  partial drains, mode capabilities, settings corruption, device failure,
  cancellation and teardown ordering.
- Capture drain is bounded and tested for worker failure, disabled audio and timeout.
- Relocated ARM tests check scalar-reference pixels, formats, NEON/fallback,
  enlarged windows, guards and register/MMU/SIMD restoration.
- Legacy settings, scanning, large libraries, borders and layout tests pass.
- Physical compact MUI checks exercise all seven pages, five audio choices,
  fullscreen/1x/2x/3x selection, resizing and the loading panel.
- ZIP and nested LHA payloads match; signed catalogues, corruption rejection,
  dependency checks, icons and private-data exclusions pass.

## Installation and limits

The exact offline installer completed on the A4000TX. Installed executable
hashes match the release build. Existing library/session settings, ROMs and
saves were preserved. First launcher exit creates Audio.cfg with Auto selected;
other absent settings stay absent. Workbench icon
positions were retained. Original executables have same-disk rollback copies
and a local backup. Global AHI and saved screenmode checksums remain unchanged.

An early standalone AHI probe queried the API before opening its device and
raised a software failure. It was suspended; the ordering was corrected and
regression-tested. It was never installed as an emulator. Reboot the test
machine once to clear that suspended diagnostic before normal use.

Listening remains pending. No eight-hour soak or tear-free physical scanout
claim is made. The exact Chrono Trigger scrolling scene still needs visual
confirmation. SNES now preserves every active source row; that regression is
covered by moving-pattern and relocated ARM pixel tests. Heavy Stunt Race FX
race execution remains below real-time audio speed even with tested frameskip.

[Investigation and upstream comparisons](output-quality.md) ·
[Raw evidence and run/binary mapping](../amiga/records/2026-10-30/output-quality/)
