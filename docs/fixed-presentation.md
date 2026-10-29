# Fixed-presentation audio comparison

Earlier AHI notification does **not establish a repeatable performance gain** in
this controlled comparison. It stays opt-in; installed players and releases are
unchanged.

## Why repeat the measurements?

The previous 600-frame timing runs enabled `VERIFY`: the 68k checked pixels on
frame 1 and every 60th frame. They also allowed an automatic snapshot at frame
300. The logs record roughly ten seconds of presentation post-processing in some
runs. That diagnostic work changes scheduling, audio servicing and the number of
presentations skipped. Those elapsed-time reductions must not be interpreted as
normal gameplay speed improvements.

The earlier logs remain useful correctness evidence. This follow-up supersedes
their scheduling interpretation; it does not invalidate separately measured ARM
core-time comparisons.

## Controlled method

A4000TX / ZZ9000, Mario Bros. on ZZNES and Super Mario World on ZZSNES. Each mode
uses reference / early / early / reference order. The pair uses byte-identical ARM
payloads and differs only in when the 68k wakes the AHI worker:

- Reference: notify when posting the next emulation request.
- Early: notify as soon as three PCM frames are queued, before presentation.
- Present all 660 completed frames; no adaptive presentation skipping.
- SNES core options are zero; all 660 frames are rendered, none skipped.
- Retain the mandatory first-frame pixel check, disable recurring verification
  and automatic snapshots. Exclude 60 warmup frames from the timing samples.
- Report the sum of the remaining 600 frame-loop durations, including pacing.
- Keep existing buffer sizes, prebuffer, audio format and ARM code unchanged.
- Scanlines and FPS overlays are off. Test window and fullscreen independently.

These are diagnostic hosts, not a proposed change to release frame-skipping
policy. Presenting every frame can itself prevent the core keeping up with its
normal region cadence. The table measures completed emulator-frame loops, not
a game's internal animation FPS. Audio counters cover the complete 660-frame run,
including warmup and shutdown; they are not a listening test.

## Results

| Emulator / mode | Reference mean, 600 frames | Early mean | Reference late links | Early late links |
| --- | ---: | ---: | --- | --- |
| ZZNES window | 9.961706 s | 9.962186 s | 0 / 0 | 0 / 0 |
| ZZNES fullscreen | 10.575047 s | 10.578673 s | 132 / 130 | 135 / 131 |
| ZZSNES window | 10.012394 s | 10.021272 s | 0 / 0 | 0 / 0 |
| ZZSNES fullscreen | 11.194146 s | 11.075491 s | 157 / 76 | 106 / 114 |

NES differences are below 0.04%. Windowed SNES is 0.09% slower. Fullscreen SNES
averages 1.06% less time, but reference runs span 10.807134–11.581159 s; both early
runs sit inside that range. This pair does not establish a dependable improvement.

All 16 runs complete their 660 presentation requests. Fullscreen runs acknowledge
all 660 flips. Every queued PCM sample completes: 351,395 per NES run and 352,912
per SNES run. Initial frame verification, register/MMU/cache witnesses, zero exit
and allocation release pass. No new freeze occurred during these runs; they do
not explain the previously reported NES freeze or establish long-session stability.

## Decision and next investigation

Keep the original release scheduling. A more useful fullscreen investigation is
to measure display acknowledgement latency and its interaction with emulation
cadence, then test one scheduling change at a time with this fixed-work control.
Simply waking the audio worker earlier does not resolve sustained production
shortfalls. Do not trade skipped frames for an unqualified speed claim.

[Raw logs, host patches, build hashes and analysis](../amiga/records/2026-10-29/fixed-presentation/).
No ROMs, saves, installed executables or system preferences were changed.
