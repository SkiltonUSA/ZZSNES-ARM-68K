# ZZNes: Mesen on ZZ9000

**New accuracy-first player:** see [player.md](player.md) for the CPU/PPU/APU,
mapper 0/1/2/3/4/7, ROM requester and battery-save build, installation, physical
results and limits. The sections below preserve the earlier silent preview
experiments and their measured results. They do not describe player support.

## Earlier CPU/PPU integration preview

Development work in progress; **working silent demo, not a full NES release**. The earlier
CPU-only throughput result is not a full-emulator performance guarantee.

Uses Mesen's CPU and PPU at commit `e1180957b31979d4b82d613af9263fcb9442a685`:
https://github.com/SourMesen/Mesen (GPL-3.0-or-later).
Unchanged CPU source/license is in `../mesen_bench/vendor/`; unchanged PPU
source and hashes are in `vendor/`. Build-generated patches document removal
of desktop ownership, debugger, snapshot and threaded video dependencies.
The CPU instruction and PPU rendering algorithms remain upstream code. Settings
select deterministic NTSC 2C02 with optional hacks off and zero-initialized RAM.
This port does not include the desktop UI, save states, HD packs or audio.

## Implemented

- CPU + PPU scheduling, background/sprite rendering, vblank/NMI and sprite DMA.
- Narrow iNES NROM loader: 16/32 KiB PRG, 8 KiB CHR ROM or 8 KiB CHR RAM,
  horizontal/vertical mirroring; unsupported formats fail closed.
- Controller 1: arrows, Z=A, X=B, Enter=Start, left Shift=Select. Q/Esc or
  the window close gadget requests shutdown. Focus loss releases buttons.
- An original generated demo ROM: patterned background and movable sprite.
  No commercial game files are bundled. Source: `scripts/make_nes_demo.py`.
- ARM-private cached execution using the verified bounded memory policy;
  separate uncached mailbox, frame publication checksum and teardown checks.
- 256x240 Workbench preview through Picasso96 ARGB on true-colour screens;
  indexed-pen fallback exists but is not physically tested. The 68k copies
  and converts published pixel indices. **This is not direct ARM scanout.**
- Optional `--direct` experiment: ARM converts into a locked, OS-owned P96
  bitmap, then the host requests a window blit. Verified separately below;
  no fullscreen PAN/page flip, and the installed preview is unchanged.
- Optional `--fullscreen`: three screen-bitmap slots, XX19c deferred-PAN/ACK
  flipping following Xanxi's approach, and CPU work overlapping the flip wait.
  Separate installed fullscreen preview; see the fullscreen milestone below.

No APU, sound, DMC, audio IRQ timing, PAL, mappers beyond NROM, battery saves,
trainers, NES 2.0, colour-emphasis presentation, scaling or game compatibility
claim. Games depending on omitted audio timing can malfunction even silently.
ROM input is capped and strictly validated. Sessions end after five minutes.

## Build

```sh
python3 scripts/build_mesen_nes.py
python3 scripts/build_mesen_nes.py --arm \
  --container-command '["podman","--connection","nuflix-converter-root"]'
.context/amiga/mesen-bench/venv/bin/python tests/amiga/mesen_nes/check_arm.py
```

Outputs under `.context/amiga/mesen-nes/`: standalone Amiga Hunk `ZZNes`,
ARM ELF, demo ROM, native preview PPM, integration patches and build hashes.
The optional ARM instruction-model check requires Unicorn 2.1.4. It does not
model physical MMU/cache, firmware or graphics-driver behavior.

CLI after checking Core1 is idle on **XX19c/XACP 1.7 only**, with 65536 stack:

```text
ZZNes <fresh-nonzero-hex-nonce> RUN DEMO 120
ZZNes <fresh-nonzero-hex-nonce> RUN CORE 6
ZZNes <fresh-nonzero-hex-nonce> RUN path:game.nes
```

`CORE` performs six complete frames with no application window/drawing; the
shared launcher's brief P96 mapping probe still runs. It is a diagnostic, not
a performance benchmark. The full display/capture sequence remains under investigation after an
unexpected physical-machine restart; do not treat this as ready for release.

## Evidence and incident, 9 October 2026

Native ASan/UBSan: 120 complete frames, 3,571,159 CPU cycles, final raw-PPU
hash `ed354ee5`. Ten frames of right input change the hash to `9bbb3e9b`.
Cancellation and unsupported-mapper rejection pass. ARM instruction modeling
matches raw frame hashes `48ef4dc5`, `48ef4dc5`, `d8cc1f38`, then `ed354ee5`
for frames 4–6, with alignment checks enabled.

The initial physical launch initialized the real CPU/PPU, then deliberately
rejected a true-colour Workbench bitmap. It restored MMU/cache registers,
reached RET1 and released memory with exit 20. Roadie/bridge survived that run.
The bridge's legacy screen report of 8bpp did not describe the actual bitmap.

After adding the Picasso96 true-colour path, a second launch was followed by
an unexpected restart. The user confirmed it returned to Workbench by itself;
Roadie and the bridge had closed. RAM files disappeared, so its application
log was lost. No physical frame or exit success was captured. A subsequent
bridge screenshot request used an incomplete window title; upstream falls
back to a full-screen capture when a title is not found. The exact failure
point and whether the core, frontend or capture contributed are **unknown**.
Do not characterize this as a normal emulator exit or an established ARM bug.
Further diagnosis separates core work and display, avoids screenshot capture,
and keeps diagnostic logs on SD so a restart does not erase them.

A subsequent **six-frame physical CORE run passed**: all six raw-PPU and
published-output hashes match the local reference, RET1/exit 0, exact control
register restoration and cache witness/canaries pass. Network apps survived.
Rendered frames 4–6 took 23.314–23.354 ms of CPU+PPU execution; this exceeds
the approximately 16.64 ms NTSC frame budget before audio/display overhead.
The CPU-only 3.35 ms result therefore does not establish full-speed NES games.
This short correctness diagnostic is not a representative game benchmark.

A subsequent **six-frame display run without screenshot capture also passed**
with the same frame hashes, output-transfer checks, RET1, exit 0 and restoration
checks. `GetBitMapAttr` reports **24-bit Workbench**, using Picasso96 ARGB
conversion. Roadie/bridge survived both isolated runs. This narrows the failure
but does not prove a screenshot caused the restart. At that stage, monitor
appearance and controls were pending; the later interactive acceptance below
confirmed both. Extended stability and external ROM compatibility remain open. Screenshot capture is intentionally excluded from further
preview acceptance until its format handling is independently reviewed.

The display run's 601,740 us for six frames includes polling, validation,
68k transfer/conversion and drawing, and includes startup/blank emulated
frames. It is **not a game FPS benchmark**. The current frontend is deliberately
simple and synchronous. Rendering/scheduling optimization is separate work.

Records: `../records/2026-10-09/mesen-nes/`.

## Interactive acceptance and installed launcher

The owner confirmed the blue background, orange sprite and arrow-key movement
on the physical monitor. A **1,104-frame, 114.369-second** interactive run then
exited on injected Q before its 1,200-frame limit. All published-frame checks
passed; RET1, register/cache restoration and allocation release passed. Roadie
and Amiga-Bridge stayed running. No screenshot command was used.

Observed end-to-end demo rate: **9.65 displayed frames/s**. Average CPU+PPU work
was **23.48 ms/frame**; the rest includes publication, per-frame checksums,
20 ms host polling, transfers, 68k conversion and P96 drawing. This is one
simple interactive demo, not a game compatibility/performance benchmark.
Neither core nor presentation currently meets full-speed NTSC emulation.
The separate earlier restart's cause remains unresolved.

Installed, with all five native Amiga MD5s matching the Mac:
`SD032G:Dev/ZZNesPreview/ZZNes`, `Start-ZZNes`, `ReadMe-Amiga.txt`, `COPYING`
and `build.json`. Binary unchanged from the six-frame acceptance: 187,596
bytes, MD5 `176807e1edc3ae7ef9663b169c2dc5c9`. Start from Shell:

```text
Execute SD032G:Dev/ZZNesPreview/Start-ZZNes
```

The script selects the drawer, sets a 65536-byte stack and logs to
`SD032G:Dev/ZZNes-Diagnostic/preview.log`; the app generates a session nonce.
It opens the built-in demo; arrows move, Q/Esc closes it. Only one Core1 app
at a time. Automatic exit after five minutes is retained. No auto-start,
firmware, library or network settings were modified. This is an installed
development preview; no public release or GitHub upload was made this turn.

Next engineering work: reduce host polling/transfer/drawing overhead, then
optimize CPU+PPU scheduling with correctness retained. Audio and broader ROM
support follow separately. Original ROM/source and current implementation are
kept reproducible; no commercial ROM is needed for this milestone.

## Performance and ARM-to-P96 experiment, 9 October 2026

The next implementation milestone passed native/model checks and bounded
physical tests. Xanxi's **ZZPicoDriveMD 1.1** provides relevant guidance on ARM
pixel conversion, buffer ownership, pacing and teardown. See the
[source review and attribution](zzpicodrive-review.md). No upstream PicoDrive
code or fixed firmware arena was copied into this GPL Mesen implementation.

Same generated original ROM, 120 frames, 3,571,159 emulated CPU cycles; each
physical run returned through RET1, restored the recorded register/cache
state and released memory cleanly:

| Configuration | End-to-end demo fps | CPU+PPU ms/frame |
| --- | ---: | ---: |
| Original frontend, profiling added | 9.67 | 23.38 |
| 1 ms polling and bulk host copy | 9.88 | 23.33 |
| Above plus CPU/PPU compiled together at `-O3` | 10.00 | 21.80 |
| ARM colour/copy into P96 bitmap, window blit | **28.44** | **21.54** |

The last run took 4.219675 seconds, **2.94x** the baseline throughput. ARM
colouring/copying itself averaged **3.75 ms/frame**. The combined presentation
measurement averaged 10.54 ms including command/polling, periodic readback and
P96 drawing. The original frontend spent about 30.43 ms on host validation/
colour conversion and 41.90 ms on drawing. These are single bounded runs of a
simple **silent demo**, not statistical game benchmarks or full-speed emulation.
The optimized core still exceeds the 16.64 ms NTSC budget; APU is absent.

**Validation differs:** the three ordinary runs independently checksum the
published indices on the 68k every frame. The direct run keeps ARM frame hashes
and both-end address challenges every frame, but fully reads back and compares
the P96 bitmap only on frames 1, 60 and 120. All three comparisons passed. This
throughput improvement includes reducing diagnostic traffic, as well as moving
colour conversion/writes to ARM and replacing `p96WritePixelArray` with a
bitmap blit. It is not an isolated measurement of one optimization. No SDL
library changes are involved.

The actual P96 format was `RGBFB_B8G8R8A8` (9): four storage bytes per pixel,
despite `GetBitMapAttr` reporting depth 24. Format/pitch are obtained from the
allocated bitmap; the code does not infer storage size from colour depth.
The application no longer fetches/converts every frame on the 68060. The P96
driver handles the window blit; no bus-traffic measurement or direct-scanout
claim is made for that driver operation.

Build and reproduce:

```sh
python3 scripts/build_mesen_nes.py --arm --direct \
  --container-command '["podman","--connection","nuflix-converter-root"]'
clang -std=c99 -O2 -g -fsanitize=address,undefined -Iamiga/mesen_nes \
  tests/amiga/mesen_nes/direct_plan.c -o .context/amiga/test-nes-direct-plan
.context/amiga/test-nes-direct-plan
.context/amiga/mesen-bench/venv/bin/python tests/amiga/mesen_nes/check_arm.py \
  .context/amiga/mesen-nes-direct
python3 scripts/analyze_mesen_performance.py
```

`--legacy-host` and `--optimize` build the comparison variants. `--direct`
implies `--optimize`; the default still uses the regular host presentation.
Native tests passed 12,816 bitmap mappings, boundary rejection, no clobber,
pixel formats and guards. Actual ARM instruction modeling matched six frames
and checked all four output formats, incorrect challenges, cancellation,
padding/guard bytes and mapping revocation. Models do not establish physical
MMU/cache behavior. Required SDK/MCP regressions and ARM/68k builds passed.

Physical direct tests: six frames, then 120 frames, both clean. No screenshot
capture was used. Pixel memory was validated; owner confirmation of this new
path's on-monitor appearance and extended interactive stability remains pending.
The earlier restart's cause remains unresolved. Temporary candidate:
`RAM:ZZNesPerf/Direct`, 200,828 bytes, MD5
`647efc2b45ebd0ae1c3159beb5baea75`. Local standalone executable:
`.context/amiga/mesen-nes-direct/ZZNes`. The installed `ZZNesPreview` binary
remains the previously accepted version; no firmware/driver/startup changes
or release were made. Records: `../records/2026-10-09/mesen-performance/`.

### Additional direct-mode memory contract

The host allocates a P96 bitmap, locks it against relocation, checks its format,
pitch and ZZ9000 aperture, writes nonce markers at both ends, and asks ARM to
copy an **already completed** frame. ARM installs at most two additional coarse
tables in the existing private table reserve. Only pages overlapping the owned
bitmap are mapped, normal inner/outer NC, privileged RW and XN. Partially owned
page bytes are addressable but never touched. No cacheable framebuffer alias,
PL310 maintenance or fixed DDR allocation is introduced.

ARM validates both markers before writing, checks cancellation per row, completes
its stores, revokes the mapping and invalidates its local TLB **before** ACK.
The host then unlocks and requests a blit. Diagnostic frames use a bulk snapshot
under the lock and compare after unlocking. Intuition events are serviced outside
the lock. Presentation has a 50 ms deadline; errors keep the bitmap allocated
and locked until synchronous Core1 reset, with at most two 20 ms stop-wait ticks
before reset (firmware reset/flush latency additional). Normal shutdown keeps
the bitmap allocated until ARM is stopped/reset too. Both tested launches
verified restored state, cache witness/canaries and exit 0.

## Fullscreen deferred flipping, 9 October 2026

Implemented `--fullscreen`, which implies direct presentation and unified `-O3`.
This is an independent adaptation of Xanxi's **XX19c deferred-PAN/ACK handoff**,
not copied PicoDrive source. The pinned firmware source confirms one-shot arm at
USER3, PAN_HI/LO submission, USER4 acknowledgment at a vertical-blank edge, and
clear/disarm on exit. No firmware/card-driver change was needed. The exact source
pin, hash and register contract are recorded in `mesen-fullscreen/firmware-contract.json`.

### Display and ownership

A 320x240 true-colour RTG screen displays the original 256x240 image with black
32-pixel side borders. A 320x720 bitmap provides three frame slots. On this P96
installation, `OpenScreenTags` creates a separate screen bitmap in VRAM from
our supplied bitmap. We use the **actual screen bitmap** for locks, geometry,
ARM writes and scanout; the supplied bitmap is retained only for caller-owned
cleanup. Both structure identity and memory differed in the physical probe.
Two initial probes rejected the mismatch before any ARM pixel write or flip,
then returned cleanly. The final path checks screen dimensions, actual bitmap
capacity/format/pitch and the full scanout aperture before enabling output.

This differs from allocating three untracked off-screen bitmaps: the entire
scanout allocation belongs to the OS screen. Each ARM destination address is
refreshed under the P96 lock; no remembered pointer is used for later writes.
Only the NES rectangle in the free slot is mapped into Core1's existing private
translation tables, using the previous NC/XN, nonce-challenged mapping contract.

Ownership is explicit: front, pending, free/render. After ARM finishes a frame,
its destination mapping is revoked and stores are complete before the host
submits a deferred flip. The old front is not recycled until the expected
16-bit ACK advances by exactly one. No pending flip can be overwritten.
The next CPU/PPU command can run while the previous frame awaits vertical blank.
The bitmap stays locked through that pending flip (100 ms deadline), then is
unlocked before Intuition events. Counter wrap and unexpected ACKs are tested.
Diagnostic snapshots are compared after unlock and before the next CPU command.

There is **no per-frame window blit**. On error, pending PAN is cancelled. ARM
is stopped/reset before display resources are freed. Cleanup restores slot zero
while still on the application's screen, closes the screen through Intuition,
and frees the original caller bitmap separately. If another screen has become
foremost, it does not pan over that screen. Q/Esc, mouse click, loss of window
activation or the five-minute limit ends the preview. The earlier unexplained
screenshot/restart issue remains outside this work; no screenshots were taken.

### Physical result and installation

Six frames and then 120 frames passed. The longer run recorded **120 queued /
120 acknowledged flips**, no pending flip or fault, three complete NES-pixel
readbacks, matching frame hashes/cycles, RET1, exact register/cache restoration
and exit 0. Workbench returned and the bridge/Roadie remained available.
Mode `50031303`, BGRA32, pitch 1280 were observations, not hard-coded addresses
or mode IDs. The bridge's legacy screen listing still incorrectly says 8bpp.

| 120-frame original silent demo | FPS |
| --- | ---: |
| Original 68k conversion/drawing | 9.67 |
| ARM conversion + Workbench bitmap blit | 28.44 |
| ARM conversion + fullscreen deferred flip | **28.88** |

Fullscreen elapsed time: 4.155026 s; CPU/PPU 21.63 ms/frame, ARM conversion/copy
3.75 ms/frame. The difference from windowed direct is only **1.56% in these
single short runs**, not evidence of a large or statistically established speedup.
Both direct variants use the same sampled readback policy. Fullscreen removes
the window blit and enables overlapping CPU work with the flip wait, but core
execution still exceeds the 16.64 ms NTSC frame budget. No sound or game-FPS
claim follows from this result. ACK measures firmware flip commitment; monitor
appearance and interaction are separate acceptance checks.

The owner subsequently confirmed the fullscreen picture and arrow-key movement.
That interactive session completed **2,262 frames / 74.419783 s (30.40 fps)**,
2,262 acknowledged flips and 38 full NES-pixel readbacks, then exited cleanly.
Changed frame hashes confirm the controller affected the rendered image. This
longer moving-demo result is separate from the matched 120-frame comparison.
A fresh installed launch then exited on injected **Q** after confirming the
fullscreen window was foremost: 232 frames / 232 flips, clean restoration and
exit 0. Workbench and network apps survived. No screenshot capture was used.

Installed separately as **`SD032G:Dev/ZZNesFullscreen`** with matching native
Amiga MD5s for executable, script, readme, license and build manifest. Executable:
213,032 bytes, MD5 `ce06e0f7dc480332051af06e8e2b1253`. The accepted windowed
`SD032G:Dev/ZZNesPreview` remains unchanged. Start with:

```text
Execute SD032G:Dev/ZZNesFullscreen/Start-ZZNesFullscreen
```

The script sets a 65536 stack and logs to
`SD032G:Dev/ZZNes-Diagnostic/fullscreen-preview.log`. Only one Core1 app at a
time. This is a development installation, not a GitHub release.

```sh
python3 scripts/build_mesen_nes.py --arm --fullscreen \
  --container-command '["podman","--connection","nuflix-converter-root"]'
clang -std=c99 -O2 -g -fsanitize=address,undefined -Iamiga/mesen_nes \
  tests/amiga/mesen_nes/flip_plan.c -o .context/amiga/test-nes-flip
.context/amiga/test-nes-flip
python3 scripts/analyze_mesen_fullscreen.py
```

Validation: 200,000 native three-slot transitions including 16-bit wrap,
pending protection and unexpected-ACK rejection; native core/controller/cancel
checks; six-frame ARM model and four pixel formats; SDK/MCP regressions and
warning-clean fullscreen/windowed cross-builds. CPU/PPU/ARM pixel code and the
private-cache policy remain unchanged. Evidence:
`../records/2026-10-09/mesen-fullscreen/`.

## Memory and publication

No new DDR allocation or fixed physical address is invented. The existing
launcher owns and independently translates a 1 MiB Exec allocation; all offsets
below are relative to that validated allocation. Private MMU/L1 and teardown
are reused from `../mesen_bench/private-memory.md`.

| Offset | Use |
| --- | --- |
| 0..0x3ffff | Entry, hot code, constants and bounded BSS |
| 0x40000..0x4ffff | Private translation table pool |
| 0x50000..0x5efff | 61,440 published 8-bit pixel indices |
| 0x60000 | 1 KiB cache writeback witness plus canaries |
| 0x70000..0x7a00f | Read-only iNES staging, maximum 40,976 bytes |
| 0x80000..0x83fff | Uncached command/status and MMU restoration records |
| 0x90000..0xadfff | Private 16-bit PPU buffer A |
| 0xb0000..0xcdfff | Private 16-bit PPU buffer B |
| Top fully owned page | ARM stack descending through remaining private RAM |

The host publishes the ROM once before initialization; ARM does not read it
until the command. ARM packs a completed frame, cleans only its owned output
lines to PoC (outer NC mapping), then commits an uncached sequence/nonce ACK.
The 68k invalidates/pulls that range and verifies its checksum while ARM waits
for another command. It never writes the cached output or live PPU buffers.
No PL310 register writes or global L2 maintenance are added. The default
SDK/fractal cache policy is unchanged.
