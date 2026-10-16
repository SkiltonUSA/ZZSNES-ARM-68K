# Guidance from Xanxi's ZZPicoDriveMD 1.1

Reviewed 9 October 2026 at XACP-ZZ9000 commit
`8bb998e4f9cac88d519ccdf3559c8a404fb6381d`. This is a source review, not a
hardware test or installation of ZZPicoDriveMD. Credit: **Xanxi** for
the ZZ9000 integration; **notaz and contributors** for PicoDrive.

[Upstream application and requirements](https://github.com/Xanxi-Amiga/XACP-ZZ9000/tree/8bb998e4f9cac88d519ccdf3559c8a404fb6381d/applications/emulators/ZZPicodrive/ZZPicodriveMD).
The retained launcher and Core1 source SHA-256s match upstream's
`SOURCE_PACKAGE_SHA256.txt`:

- `ZZPicoDriveMD_v1.1.c`: `174f0c628b5dafd07929e06a8d228a4da2180c8bd1322cc3e6e7eed7f5681c59`
- `zzpico_core1.c`: `9cf49e8ab7acc54563732fd93df2e9a5ef9b5a8ed36020335673cd28a5332584`

Subsequent installation and physical testing are recorded in the
[ZZPicoDrive comparison](../../docs/amiga-zzpicodrive-comparison.md), including
a stock silence-mode stall that qualifies the source-only pacing discussion below.

## What the implementation teaches us

1. **Keep pixel conversion on ARM.** Core1 renders RGB16, uses a palette
   conversion table to write directly into an RTG backbuffer, cleans that
   buffer, then publishes a ready sequence. The 68k publishes the address and
   pitch; it does not transport every pixel through Zorro III. Expensive CRCs
   and scans are omitted from the fast path.
2. **Make buffer ownership explicit.** The fullscreen launcher manages three
   buffers: displayed, pending firmware flip, and available to render. It
   waits for the deferred-PAN acknowledgment before recycling the old front
   buffer. `AllocScreenBuffer` has a `p96AllocBitMap` VRAM fallback.
3. **Separate emulation from presentation.** Core1 may continue emulating and
   generating sound while skipping drawing when no free buffer is available.
   With audio enabled, PCM ring drainage governs pacing. The AHI backend uses
   a separate task; Paula is also supported. Without audio, the ARM counter
   and display handshake provide pacing.
4. **Teardown is part of correctness.** The source records a previous exit
   corruption caused by freeing buffers while ARM was still writing. Its
   normal exit restores scanout and stops Core1 before releasing buffers.
   Our implementation must handle timeout/error exits as well as normal exit.

Sources:
[ARM core](https://github.com/Xanxi-Amiga/XACP-ZZ9000/blob/8bb998e4f9cac88d519ccdf3559c8a404fb6381d/applications/emulators/ZZPicodrive/ZZPicodriveMD/source/arm/zzpico_core1.c),
[Amiga launcher](https://github.com/Xanxi-Amiga/XACP-ZZ9000/blob/8bb998e4f9cac88d519ccdf3559c8a404fb6381d/applications/emulators/ZZPicodrive/ZZPicodriveMD/source/amiga/ZZPicoDriveMD_v1.1.c).

## Adaptation to ZZNes

The immediate experimental `--direct` build applies the first principle to a
Workbench window: ARM colours a completed frame into an OS-allocated P96
off-screen bitmap; the 68k requests a graphics-library blit into the window.
This removes the per-pixel 68k conversion/transfer, but **is not fullscreen
page flipping or direct scanout**. Picasso96 still owns window clipping and
drawing. No PAN registers are changed. The regular frontend remains available.

The subsequent **`--fullscreen` build** now uses the verified XX19c deferred-PAN
interface, three frame slots in the actual OS screen bitmap, ACK-gated ownership
and CPU work overlapping the vertical-blank wait. It passed physical flip,
pixel-readback, teardown and owner picture/control checks. See the fullscreen
milestone in [the implementation README](README.md). This is a separate path
from the windowed experiment described above, with no new firmware or copied
PicoDrive implementation.

Our bitmap address is obtained under a short relocation lock and independently
challenged at both ends on ARM before stores. Only overlapping pages are mapped,
as normal noncacheable/XN; the mapping is revoked before completion is published.
Framebuffer pages are not made cacheable. Our existing bounded private L1 policy
remains active for emulation. Full bitmap snapshots on frame 1/every 60 frames
are compared after unlocking, and error paths retain ownership until Core1 reset.

Xanxi's MMU implementation uses fixed firmware-reserved regions and cacheable
1 MiB sections for video. Those addresses and attributes must **not** be copied
into our Exec-owned allocation model. The README specifies XX19a/XACP 1.6 or
an explicitly compatible later firmware and a matching card driver. Our XX19c
integration needs its own contract checks; a newer version number alone does
not validate every deferred-PAN interface. No firmware update follows from this
review. The retained source's warning about bitmap locking on some P96 versions
also means local tests are necessary; unlocked pointers are not generally safe
against relocation.

PicoDrive/Cyclone performance does not predict Mesen CPU/PPU performance. Mesen's
measured 21.8 ms/core frame still exceeds the 16.64 ms NTSC budget. Presentation
optimization cannot on its own establish full-speed emulation. Audio, safe
asynchronous buffering and CPU/PPU optimization remain separate milestones.

## Licensing and reuse

This is architectural guidance, with no PicoDrive or ZZPicoDrive source copied
into the GPL Mesen implementation. The published PicoDrive revision has a
historical non-commercial license; it must not be assumed GPL-compatible.
Source reuse would require reviewing each component's license separately.
[Upstream licensing notice](https://github.com/Xanxi-Amiga/XACP-ZZ9000/blob/8bb998e4f9cac88d519ccdf3559c8a404fb6381d/applications/emulators/ZZPicodrive/ZZPicodriveMD/LICENSE_NOTICE.md).
