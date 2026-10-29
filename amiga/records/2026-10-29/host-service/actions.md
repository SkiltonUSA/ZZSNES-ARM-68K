# AHI servicing and bitmap locks — 29 October 2026

- Read current handoffs, console agent guides, player/audio overlays and previous
  conversion-overlap evidence. Retained original serial presentation and ACK policy.
- Added call-elapsed histograms for P96 lock/unlock, AHI SendIO (locked/unlocked
  at entry), WaitIO and worker PCM copies. Sample after 60 warmup frames; no logging
  inside the timed loop. These are elapsed intervals, not task CPU measurements.
- Native histogram tests cover timer wrap, bin boundaries and sample totals.
- Built isolated profiled hosts around existing ARM/MMU images; preflight found
  bridge available, Core1 idle, screenmode CRC 9C80FBC5 unchanged.
- Six profiling runs completed. Initial SNES instrumentation missed its stereo
  CopyMem expression, so its zero-copy counts were invalid. Added an exact-match
  check and repeated both SNES runs; final report uses NES 1/2 and SNES 3/4 only.
- Read ENV:Sys/ahi.prefs without modifying it. A read-only AHI mode query confirms
  the SDK structure layout and mode names: unit 0 is configured for Paula Fast
  8-bit mono, 8000 Hz; music unit 255 is ZZ9000AX 16-bit stereo, 32000 Hz.
  No sound allocation, preferences changes or listening test in that probe.
- Added a four-frame batch candidate with six-frame prebuffer. NES AHI submission buffers
  are 8192 bytes; SNES stereo buffers are 16384 bytes. SNES feedback capacity
  includes two four-frame requests. Core sample rates/formats remain unchanged.
- Extended actual-worker native tests: 600 blocks require 150 submissions rather
  than 200 (old worker fails that assertion), preserving ordered PCM, short/forced
  flushes, state drains, error/abort handling and allocation cleanup. A missing
  unused-function suppression and stereo fixture definitions were fixed locally
  before hardware candidate testing. Both reference/candidate fixtures pass.
- Corrected a format warning in the read-only mode probe before deployment.
- Matched hardware timing uses original presentation, 660 frames and 60-frame
  warmup, initial pixel verification only, no automatic captures, private saves.
- Eight ABBA timing runs completed. Four-frame batching is 1.07% slower NES and
  0.57% slower SNES on mean 600-frame time. Keep the three-frame release default.
  NES late-link counts fall, but fewer longer submissions are not directly
  comparable and no audible-quality benefit is claimed.
- Fresh integrated batch builds pass; ARM payload/MMU headers are byte-identical
  to the recorded base. Host/fullscreen source matches the tested relinks, and
  audio preprocessing is identical with early wake disabled. Public profiling
  generation reproduces the exact tested instrumentation for both consoles.
- Both four-frame candidates pass physical pause/resume, fullscreen-window-
  fullscreen and clean quit after a fresh Player.ready marker. Total: 16 player
  runs, all completing PCM/restoration/release, including the two superseded
  SNES profiling runs. No-audit final hash fields are not used as proof of pixel
  equivalence; timing checks retain only the exact initial pixel readback.
- Postflight: Core1 idle, bridge alive. AHI ENV preferences CRC 725CE7DB matches
  the initial read-only snapshot; saved screenmode CRC remains 9C80FBC5.
  Removed only SD032G:Dev/ZZCost1029. Installed applications, ROM libraries/saves,
  firmware and global preferences remain unchanged. Local MCP left running.
