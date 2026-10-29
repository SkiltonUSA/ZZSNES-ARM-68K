# Fullscreen acknowledgement polling

The experiment reduces pending-flip register reads substantially, but does **not
establish a useful FPS improvement**. The original polling remains the default.

## What the profile measures

The existing host already runs the next ARM emulation request while the previous
deferred PAN is pending. `full_poll()` checks the firmware acknowledgement during
that work, and `full_drain()` waits for any remainder before audio preparation and
pixel conversion. Adding another emulation-overlap step would duplicate existing
behavior.

Queue-to-ACK age is **not blocked host time**: it includes work overlapped with the
ARM. The profile therefore separates acknowledgement observations during ARM
requests, explicit draining, presentation requests, and loop/pacing polls. Counts
cover flips queued after the first 60 warmup frames. Stage 1 frame timing measures
the explicit blocking drain separately.

A firmware ACK remains the authority for recycling the previous front buffer.
These are host-observed timestamps, not an FPGA scanout trace or proof of tear-free
output. The experiment uses the existing XX19c/XACP contract; no firmware changes.

## Candidate

Read the acknowledgement register at most once per 250 microseconds while a flip
is pending, with the first poll after queueing immediately eligible. This is a
host read interval, not a delayed display command. The host still busy-polls the
clock; it does not sleep under the bitmap lock or change task priority.

The bitmap stays locked until acknowledgement. Existing counter checks, nominal
100 ms timeout, cancellation, framebuffer allocation/bounds and reset-before-free
remain. Timer wrap uses unsigned subtraction. Windowed operation takes its
original early-return path. ARM code, audio scheduling, buffer sizes and shared
memory layouts are unchanged.

## Method and evidence

For each console, run reference / candidate / candidate / reference on the
A4000TX/ZZ9000. Use the same Mario Bros. or Super Mario World ROM and 660 completed
presentations, excluding 60 warmup frames from timing. No adaptive omissions,
recurring pixel verification, automatic snapshot, scanlines or FPS overlay.
The mandatory first-frame check remains; SNES reports 660 rendered, zero skipped.

One pass measures poll phases/read counts. A separate pass removes those counters
from **both** hosts before comparing frame-loop times. Byte-identical ARM payloads
and the same original audio schedule are used within each pair. Timing includes
pacing and is not a game's internal animation FPS.

The first NES profiling logs were overwritten by a test-harness filename
collision during the counter-free pass. Their initial summary was retained
privately, the output directories were separated, and that entire profiling ABBA
pair was repeated. Only the repeated NES profile is used below. SNES profiling
logs and all counter-free logs were preserved. This was a logging error, not an
Amiga execution failure.

## Profile results

Means across the two accepted runs, per 600 measured flips:

| Emulator / polling | Register reads | Observed ACK age | Blocking drain per frame |
| --- | ---: | ---: | ---: |
| ZZNES, original | 195,890.0 | 12.410 ms | 4.610 ms |
| ZZNES, 250 µs | 20,233.5 | 12.219 ms | 4.508 ms |
| ZZSNES, original | 215,438.0 | 12.518 ms | 4.237 ms |
| ZZSNES, 250 µs | 21,942.0 | 12.360 ms | 3.611 ms |

Pending-flip register reads fall **89.67% for NES** and **89.82% for SNES**.
These counts exclude the one counter read when queueing each flip. They are
register transactions, not bulk framebuffer copies or a measured bandwidth gain.
The host remains busy; no CPU-utilisation reduction has been established.

In the reference, 82.8% of NES and 75.6% of SNES acknowledgements are observed in
the explicit drain after emulation. That leaves a significant blocking interval
before conversion. A shorter drain counter alone is not a speed gain: observation
can move into another phase. The complete-frame results below are decisive.

## Counter-free results

| Emulator | Reference mean, 600 frames | 250 µs mean | Difference |
| --- | ---: | ---: | --- |
| ZZNES | 10.607100 s | 10.555073 s | 0.49% less time |
| ZZSNES | 10.936556 s | 10.985259 s | 0.45% more time |

Reference/candidate late audio links are NES 137/118 versus 132/125, SNES 113/69
versus 93/95. These complete-run counters show no dependable audio improvement;
queue completion is not a listening test. The small timing differences do not
justify changing release defaults.

## Validation and use

Native tests exercise the actual staged host retirement code against mocked
registers: ACK wrap, delayed observation, retained lock, unexpected counter change
and timeout cancellation. Policy tests cover clock wrap and interval boundaries.
All archived hardware runs complete 660 flips and every PCM sample, restore
register/MMU/cache witnesses, exit zero and release their allocation. These tests
do not establish long-session stability or resolve the previously reported NES
freeze. Installed players, ROMs, saves and system preferences are unchanged.

`--ack-poll-us 250` is available only with the isolated register experiment;
omission keeps the original host. The separate output drawer ends in `-ack250`.
[Build commands and native tests](../development/ack-poll-20261029/README.md).
No release or installer is replaced.

## Next target

The profile identifies an explicit ACK drain before conversion. A candidate worth
investigating is conversion into a free slot while the previous flip is pending.
This needs a separate render-in-progress state: an old flip's ACK must **not**
unlock a bitmap that the ARM is still writing. PCM metadata must also survive
presentation replies, and error/cancel paths must retain reset-before-free.
Simply deleting the drain or unlocking early would violate the current contract.

[Raw runs, patches, build hashes and analysis](../amiga/records/2026-10-29/ack-polling/).
