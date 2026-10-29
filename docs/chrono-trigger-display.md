# Chrono Trigger fullscreen tearing review

The reported symptom is a brief horizontal tear in **fullscreen**. The saved
session uses normal-resolution output, FPGA Soft scanlines, left/right and
vertical edge masks, and core options 0. The new 2×/3× Workbench scaling path
is not active in that session. Its scaling changes therefore do not explain
this report.

## What the code establishes

- Core1 completes the PPU frame before conversion. ARM writes only the locked,
  allocated render bitmap and revokes its temporary mapping before acknowledging.
- Fullscreen uses four slots and retains the previous logical front for another
  PAN acknowledgement before reusing it.
- In the local XX19c reference `main.c`, a blanking edge updates the pending
  pan offset and increments the acknowledgement. `video.c:isr_video` separately
  configures VDMA from that offset. The software acknowledgement is not a
  documented physical scanout-completion fence.
- Observing an ACK during active scanout does not prove the flip happened then:
  the host may simply have polled after blanking ended.

Earlier post-ACK blanking waits were withdrawn after bounded timeouts; the
extra-cycle variant also substantially slowed playback. They remain disabled.
No firmware, global screen mode or unverified DMA-register changes were made.

## Current hardware comparison

Chrono Trigger ran for 1,800 emulated frames per case, using the same fullscreen
settings and ROM. Existing thumbnail data avoided automatic capture pauses.
The first-frame pixel verification remains enabled.

| Setting | Run time | Late AHI links | ACK observed in blank / active |
| --- | ---: | ---: | ---: |
| FPGA Soft scanlines | 32.737 s | 0 | 1381 / 35 |
| Scanlines off | 32.783 s | 0 | 1375 / 40 |

Both runs completed 962,488 PCM frames without a device error, passed the
first-frame pixel comparison and restored registers/MMU/cache state before
releasing owned memory. Observed refresh was approximately 59.52 Hz. These
counters do **not** establish tear-free output or identify the visible cause.
The reported scene has not yet been reproduced and visually confirmed here.

Next useful evidence is the same scrolling scene with scanlines on/off, and a
comparison of an F9 capture with what the monitor shows. A framebuffer capture
cannot record a physical mid-scanout tear; an artifact present in that capture
would instead direct investigation toward PPU output or conversion. Keep this
separate from audio scheduling and integer-window scaling.

Reference source digests (local XX19c review snapshot):

- `main.c`: `b15d3a9101960268a2223aa5c8c82dadb33127d8980a62906b8d82f6bc76db35`
- `video.c`: `ac54096007a658d88ba6f3a02ced91e8c29106e129fa3a4c04061c5975c49b12`

[Hardware logs](../amiga/records/2026-10-29/audio-window/).
