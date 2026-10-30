# Audio quality and scrolling

## Audio output

The reported muffled sound had a concrete cause: configured AHI unit 0 used
**Paula Fast 8-bit mono at 8,000 Hz**. Supplying 32 kHz PCM to that unit does not
raise its mixing rate. There is no lossy audio compression in this path.

Display now includes **Audio output (all games)** ([interface](current-interface.md#audio-output)):

| Choice | Behaviour |
| --- | --- |
| Auto | If unit 0 names a registered Paula mode, request per-app 14-bit stereo at 22.05 kHz; otherwise retain unit 0. |
| Configured AHI unit 0 | Use the system's existing driver, rate and mixing configuration. |
| Paula 14-bit / 22.05 kHz | Select the registered uncalibrated 14-bit stereo mode, at the closest available rate. |
| Paula 14-bit / 28 kHz | Higher-rate alternative with greater 68k mixing cost. |
| ZZ9000AX / 32 kHz | Explicit AX output for speakers connected to that hardware. |

The choice is saved in the application's `Audio.cfg`, independently of ROM
profiles. Missing/invalid settings mean Auto. No global AHI preferences, firmware
or Workbench screen modes are changed. If the selected low-level controller is
unavailable, playback falls back to unit 0 and reports that route in the player
log. A busy device can therefore retain the old lower-quality output.

The core still produces 16-bit PCM: NES 32 kHz mono; SNES 32,040 Hz stereo.
AHI performs output-rate conversion. The tested Paula rates are **22,030 Hz**
and **27,928 Hz**. These are hardware rounding, not changes to game speed.

A dedicated 68k process keeps three immutable PCM buffers ready. The AHI
callback retires/schedules buffers and signals the process; it does not allocate,
copy from ARM, perform disk I/O or draw. Buffer sizes respect the driver's
reported minimum. Pause/state operations drain queued PCM; controller teardown
precedes releasing samples and process code. Per-app output primes nine frames (three three-frame batches) in a twelve-slot
producer ring: about 150 ms at NTSC or 180 ms at PAL. The configured unit-0
fallback retains its six-frame startup threshold. Per-app output acquires the audio hardware while
playing; configured unit 0 remains available for system-managed mixing.
Higher-quality mixing costs more 68k time; slower machines can retain their
configured output instead.

`AUDIO_OUTPUT` reports the mode, actual mix rate, callbacks, silence schedules
and starvation transitions outside a declared drain. Silence totals include
startup/end/pause/capture; legacy `AUDIO_END gaps` counts late linked device requests
and is **not comparable** to the callback counter. A zero gap counter alone
never establishes good sound. Listening acceptance remains a separate test.

## Upstream comparison

- [FCEUX SDL audio](https://github.com/Rezzy-dev/fceux/blob/4d01530b6689b06a6d7cf0679b63625e3270ae9a/src/drivers/sdl/sdl-sound.cpp)
  requests signed 16-bit mono PCM, uses a ring buffer and feeds an audio callback.
  It supplies silence on underrun; it does not compress the audio.
- [Snes9x SDL audio](https://github.com/snes9xgit/snes9x/blob/1bcc369e89f08243e0a462882fb1f3e42e51de3a/common/audio/s9x_sound_driver_sdl.cpp)
  uses signed 16-bit stereo and a producer/callback ring. Its
  [APU resampler](https://github.com/snes9xgit/snes9x/blob/1bcc369e89f08243e0a462882fb1f3e42e51de3a/apu/apu.cpp)
  can adjust output ratio from buffer occupancy. That modern APU/resampler was
  reviewed, not transplanted into PocketSNES; doing so needs separate core/state
  compatibility work.
- [Xanxi's PicoDrive framework](https://github.com/Xanxi-Amiga/XACP-ZZ9000/tree/8bb998e4f9cac88d519ccdf3559c8a404fb6381d/applications/emulators/ZZPicodrive/ZZPicodriveMD)
  separates AHI servicing from video, uses stereo PCM blocks and preserves active
  source rows. Its direct Paula backend is distinct from this AHI implementation.
- [AHI controller allocation](https://www.lysator.liu.se/ahi/v4-site/files/ahi/doc/ahidev_8.html)
  supports an app-selected mode/rate; the hardware chooses the nearest frequency.
  [Dynamic sample documentation](https://www.lysator.liu.se/ahi/v4-site/files/ahi/doc/ahidev_9.html)
  specifies big-endian signed PCM and the minimum backing-buffer calculation.

No modern Snes9x/FCEUX/PicoDrive core was replaced in this release. The changes
are in the output adapter, scheduling and MUI settings.

## Validation and limitations

Native sanitizer checks cover routing, malformed settings, preference bounds,
real worker PCM order, ring reuse, partial drains, missing/busy devices, failures
and cleanup. Relocated ARM pixel tests cover formats, integer window sizes,
scanlines, padding, cancellation and MMU/SIMD restoration. These complement,
but do not replace, physical tests or listening.

An initial diagnostic queried AHI before opening its device and raised a software
failure. The probe was suspended, the ordering corrected, and a test now requires
a valid AHI base at every query. Corrected mono/stereo probes complete and release
the controller. The failed diagnostic was never installed as an emulator.

Hardware measurements and final acceptance checks are recorded in
[release validation](release-validation.md).

## Vertical-scroll bands

The previous SNES bezel mapped **224 source rows into 216 output rows** (239
into 216 in overscan mode), dropping regularly spaced rows. The horizontal
Smooth option did not filter that vertical reduction. Doubling the already
composed 320×240 image cannot recover those rows. That explains a reproducible
source of horizontal bands specifically during vertical motion; it does not
prove every visible monitor artifact has the same cause.

Normal output now centres all 224 rows within 240, one-to-one. Overscan retains
all 239 rows. Explicit user edge masks still hide their selected margins. The
active width is 298 pixels for a 4:3 monitor (224 for the widescreen compensation
option), keeping the content's shape close to the original while preserving
rows. High-detail mapping follows the same row alignment; 2×/3× windows duplicate
the corrected composition. Horizontal Sharp/Smooth choices remain available.

The old mapping fails a consecutive-source-row regression. The corrected scalar
and optimized paths agree for every pixel across 32 vertical scroll offsets,
NTSC/overscan, masks and both byte orders. Relocated A9 tests compare against an
independent scalar reference, including NEON and fallback with register restoration.
No core PPU scrolling, timing, ROM, firmware or PAN-acknowledgement policy changed.

Physical scanout tearing remains a distinct possibility: a framebuffer capture
cannot prove tear-free monitor output. Visual confirmation of the reported
Chrono Trigger scene remains pending. See the earlier
[fullscreen investigation](chrono-trigger-display.md).

## Capture and PAL testing

First-time automatic library capture exposed another interruption: bitmap
readback, encoding and disk writes ran while audio needed servicing. Capture now
uses the state-operation drain at an acknowledged, unlocked frame boundary.
The worker finishes queued PCM and re-primes after capture. This is a deliberate
pause, not uninterrupted playback; silence counters include that pause.

Warm-preview PAL tests completed 3,000 frames per emulator with no starvation
transitions. Final cold-preview PAL runs completed 3,000 fullscreen frames and
600 frames in 3× windows per emulator with zero starvation transitions and all
produced PCM delivered. Capture still scheduled silence. A zero counter must
not be interpreted as proof that no audible pause occurred.

Asterix's legacy NES header lacks a PAL flag. Its `(Europe)` filename supplies
the existing region fallback; renaming a private test copy initially lost that
hint. Corrected PAL tests assert the actual runtime region. ROM data was not
patched. Another reused private SNES filename retained SRAM from a different
cartridge; the player correctly refused the mismatched save. The test fixture
was separated and rerun; user saves were unaffected.

## Super FX limit

Stunt Race FX's race checkpoint remains slower than real time on this core.
Across 1,800 emulated NTSC frames (about 30.04 seconds of audio), core work took
41.60 seconds with frameskip Off, 40.37 with Auto, and 36.93 with Fixed 1.
Auto rendered 1,562 frames; Fixed 1 rendered 900. All three still starved audio.
Larger buffers cannot repair a sustained core-time deficit, and these options
do not establish acceptable sound for that race. Default core settings remain
unchanged. Early trials restored Off from the checkpoint; only the reruns with
verified checkpoint options support this comparison.
