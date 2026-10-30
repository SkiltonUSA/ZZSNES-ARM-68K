# Audio crackling and AX startup

**Historical investigation.** The initial source-core diagnosis below was
subsequently corrected: the recording followed ARM-to-68k PCM transfer.
See the [Cortex-A9 shared-audio fix](arm-audio-publication.md) for current
evidence and the 0.7.11 update. Earlier listening results remain recorded here.

Crackling is **not fully resolved**. HiFi output substantially improved Chrono
Trigger in listening tests, but slight crackling remained. The NES Super Mario
World title still had heavy pops; Contra also popped on its title screen.
A zero underrun counter is not a listening pass.

## AX after reboot

Initially, AX stopped advancing even in a standalone tone test. Chrono Trigger
exited after 21 frames with no callbacks. Requesting a 32,000 Hz device clock
instead of deriving it from the 32,040 Hz SNES source did not recover that session.
A startup callback check successfully fell back to configured AHI for 600 frames.
It checks startup only; it does not repair a later device stall.

After reboot, standalone mono and stereo AX playback both completed. The cause
of the earlier stalled state remains unknown. The following fullscreen runs
completed on the physical machine with all PCM delivered, zero reported
starvation, register/cache restoration and owned-memory release:

| Emulator / game | Output | Frames | Completed sample frames |
| --- | --- | ---: | ---: |
| ZZSNES / Chrono Trigger | AX 32,000 Hz | 3,000 | 1,604,146 |
| ZZSNES / Chrono Trigger | Paula 22,030 Hz | 3,000 | 1,604,146 |
| ZZNES / Super Mario World | AX 32,000 Hz | 3,000 | 1,597,247 |
| ZZNES / Super Mario World | Paula 22,030 Hz | 3,000 | 1,597,247 |

These are delivery/lifecycle results, not proof of clean analogue sound.

## HiFi comparison

AHI's HiFi mixer uses linear interpolation and a 32-bit mixing path. The
application still supplies 16-bit PCM, and Paula output remains 14-bit. No game
core, sample rate or source waveform was changed for this comparison.

| Test | Result |
| --- | --- |
| Chrono Trigger, HiFi 22,030 Hz | 7,200 frames; 3,849,952 sample frames completed; zero starvation; clean exit. Listening: much better, slight crackling remains. |
| Chrono Trigger, HiFi 27,928 Hz | 3,000 frames; zero starvation; clean exit. Listening: worse than 22 kHz. |
| NES Super Mario World, HiFi 22/28 kHz | 3,000 frames at each rate; zero starvation; clean exits. Listening still reports heavy pops. |
| NES Contra, HiFi 22 kHz | 3,000 frames; all 1,597,254 sample frames completed; zero starvation; clean exit. Title pops; gameplay effects may mask the noise. |

One earlier Chrono listening run was deliberately stopped to restart it for the
listener. Its clean shutdown is recorded, but it is not a full 7,200-frame pass.

The candidate now prefers the registered Paula HiFi 14-bit stereo mode for the
existing Paula choices, with regular 14-bit fallback when HiFi is absent.
Configured AHI and explicit AX retain their routes. No mode ID is hard-coded.
Native tests cover preference order, missing HiFi, invalid capabilities,
32,000/32,040 Hz sources, startup callback timeout, failures and cleanup.
The recorded ARM image, mapping probe and relocations are preserved.

## Separating feeding gaps from output conversion

A task-fed three-buffer triangle probe could starve when its producer was late.
Its mono Paula run had nine starvation transitions and 16 interior sample jumps
above 512. That demonstrates a feeding-delay failure, not that the emulator
suffers the same delay. A stereo run had no interior jumps above 512.

A second probe puts the entire four-second waveform in memory before playback.
It compares a single sound, 80 contiguous segments and, for HiFi, 80 separately
registered dynamic sounds. There is no producer deadline in this test.

- AX and regular Paula, mono/stereo, whole/segmented: no interior jumps above 512.
- HiFi Paula, mono/stereo, whole/segmented/separate sounds: no interior jumps
  above 512; largest normalized steps were 93 for mono and 148 for stereo.
- Listening feedback described the tones as clean.

The capture is AHI's mixed output before hardware conversion and the physical
DAC. It cannot exclude calibration/analogue artifacts. The HiFi test normalizes
signed 32-bit mixer output to its high 16 bits for comparison. These low-frequency
tones do not cover every game waveform or prove every buffer transition correct.

## Paula driver comparison

Initial software was ahi.device 4.180 (68060), paula.audio 4.23 and
zz9000ax.audio 4.19. No active or saved 14-bit calibration file was present.

The Aminet Paula 4.33 update includes revised uncalibrated defaults and a change
to the conversion table around negative values/zero crossings. Its source
explains why the old table could introduce audible analogue noise even with
correct digital samples. This is a relevant hypothesis, not a demonstrated fix
on this machine.

Testing 4.33 was explicitly approved. The original driver was backed up and
verified; a rollback script was prepared. Only paula.audio was replaced.
The on-disk version is 4.33, while the pre-reboot resident version was still
4.23. After reboot, a completed Chrono Trigger test confirmed resident 4.33.
All 1,604,146 sample frames completed over 3,000 frames, with zero starvation
and clean restoration/release. Listening: no popping, but some tones still
sounded slightly off. The post-update NES run also completed 3,000 frames with all PCM delivered
and zero starvation, but Super Mario World still popped/crackled in listening.
The driver update is therefore not a complete fix for both emulators. Both saved and active AHI preferences
retain their original checksums. ahi.device and the AX driver were not replaced.

The candidate emulator builds remain uninstalled and unpublished. The earlier
bridge-interrupted run is not counted; the post-reboot results above supersede
the incomplete test matrix without retroactively accepting it.

Evidence: [audio test records](../amiga/records/2026-10-30/audio-crackle/).

References: [AHI mixing and callback contract](https://www.lysator.liu.se/ahi/v4-site/files/ahi/doc/ahi.html),
[dynamic sample requirements](https://www.lysator.liu.se/ahi/v4-site/files/ahi/doc/ahidev_9.html),
[Paula 4.33 release notes](https://ftp.fau.de/aminet/driver/audio/AHI-Paula.readme),
[driver and source archive](https://ftp.fau.de/aminet/driver/audio/AHI-Paula.lha).

## NES source-audio isolation

A private NES build recorded the exact signed 16-bit, 32 kHz mono samples
before AHI conversion. It completed 1,200 frames, wrote 1,277,798 bytes after
audio stopped, delivered all 638,899 sample frames and shut down cleanly.
The recording is private game audio and is not included in this repository.
Listening to this recording on the Mac reproduced the popping/crackling.
The reported NES artifact is therefore present before AHI, Paula conversion
or physical audio delivery. This isolates the source-audio path; it does not
establish the cause of the remaining SNES tonal differences, or prove that
every audible NES effect is an emulation defect.

The NES adapter selects FCEUX sound quality 0. Isolated quality 0/1 ARM builds
are now being compared with identical CPU/video settings. Native checks passed
24 synthetic-tone frames and 120 Super Mario World frames with equal CPU/video
endpoints; both builds passed 20,480 relocated register-handler cases. These
checks establish consistency, not sound quality.

Higher-quality audio remains experimental: the current save-state adapter does
not preserve the HQ FIR position/history (`mrindex`, `WaveHi`). Save/load and
old-state compatibility need integration before enabling it in a release.


## Isolated higher-quality NES core result

Listening found the quality-1 Super Mario World test cleaner / popping reduced.
It is not ready for release: 3,000 emulated frames took 60.567 seconds, with
474 audio starvation transitions and 476 queued silence buffers. All 1,597,361
source sample frames were eventually completed, but that does not mean
continuous playback. There were 1,001 acknowledged presentations, not 3,000
displayed frames. Core time was 47.205 seconds. Restoration, cache witnesses,
and owned-memory release passed.

The short quality-0 reference completed 600 frames in 11.173 seconds with zero
starvation. The unequal run lengths are not a matched performance benchmark.
Native CPU/video endpoint comparisons are recorded separately. The quality-1
log retains a historical `quality0` player banner; the build receipt and
`compat_core.cpp` select `FSettings.soundq=1` in the rebuilt ARM image.

Next: profile the high-quality synthesis/filter path, reduce its cost without
altering sample timing, then repeat equal-length delivery/listening tests.
HQ filter history and old-state compatibility also need implementation before
promotion. Neither experimental player replaces the installed release.
