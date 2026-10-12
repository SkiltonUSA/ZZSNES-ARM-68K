# ZZNes 0.3.2-dev: accuracy-first player

The first player extends the accepted fullscreen preview with Mesen's APU,
common cartridge mappers, a ROM requester, battery RAM, pause/mute and full
9-bit palette presentation. It is a development candidate, not a complete
Mesen port or a full-speed NES emulator. The original fullscreen installation
remains available separately.

## Core and frontend

The pinned upstream CPU/PPU now run with Mesen's two pulse channels, triangle,
noise, DMC DMA, frame counter and IRQ scheduling. Upstream NROM, MMC1, UNROM,
CNROM, MMC3, AXROM and JyCompany classes use a bounded memory-mapping adapter. Optional
accuracy-reducing flags remain off; no emulated frames are skipped, and no
CPU/PPU/APU clock ratios are changed to improve reported FPS.

Supported input is clean **NTSC iNES mapper 0/1/2/3/4/7/90**, with explicit
per-mapper PRG/CHR/RAM limits in `cartridge.h`. NES 2.0, trainers, PAL/Dendy,
VS, MMC6, expansion sound, cartridge-database board overrides and other
mappers fail closed. Standard MMC3B/C IRQ behavior is selected. Compatibility
with every game using these mapper numbers is not established.

The custom mono mixer preserves the upstream nonlinear pulse/TND DAC
formulae using generated integer lookup tables and upstream `blip_buf`'s
band-limited resampling at 32 kHz. It removes desktop sound devices, filters,
threading and allocation. The five channels retain upstream timing. A bounded
arena replaces desktop ownership. A generated patch replaces a negative
signed left shift in blip_buf's high-pass filter with equivalent defined
multiplication; ASan/UBSan exposed it during audible-channel tests. Audio
changes at the exact end-of-chunk timestamp are processed before advancing
blip_buf's frame, as in the upstream timestamp-based mixer.

AHI unit 0 receives signed, big-endian, 16-bit mono buffers through two
asynchronous requests. Existing AHI preferences select the output hardware;
the program does not change them. AHI failure opens a muted session while APU
emulation continues. Below real-time speed audio gaps are expected; pitch and
emulated clock ratios are not altered to disguise slow emulation. The gap
counter measures completed previous requests before the next submission,
not a hardware underrun interrupt count.

No-argument launch opens an ASL `.nes` requester. Keyboard: arrows, Z/X,
Return, left Shift; P pauses, M mutes, Q/Esc/mouse click/focus loss exits.
Pause stops advancing emulation and lets outstanding sound drain. No joystick,
second controller, save states, reset menu, cheats or archive loading yet.

Battery RAM loads from `<ROM>.sav`. Wrong-sized or unreadable saves stop the
session rather than replace existing data. Clean exit writes changed RAM to
`.sav.tmp`, checks the written bytes, renames the old file to `.sav.bak`, then
renames the temporary file. This retains a recovery copy but is not a
power-failure-atomic transaction. Failed/timeout core sessions do not write
back potentially incomplete state. There is no periodic autosave yet.

## Memory and display

The player reserves **4 MiB of Exec-owned ZZ9000 Fast RAM**, found at runtime
and proved by the mandatory eight-word bootstrap mapping probe. No fixed DDR
address is allocated. The accepted preview still uses 1 MiB.

| Relative range | Purpose |
| --- | --- |
| `000000–03ffff` | ARM image and static data; 255,728 bytes in 0.3.2-dev |
| `040000–04ffff` | Dynamically aligned L1/L2 tables and temporary framebuffer map |
| `060000` | Cache witness and guards |
| `080000–083fff` | Noncacheable mailbox, stop, saved MMU state and reports |
| `090000–0adfff`, `0b0000–0cdfff` | Private 16-bit PPU frame buffers |
| `0d0000–0d07ff` | Big-endian published PCM, up to 1,024 samples |
| `0d1000–0d8fff` | Published/imported battery RAM, up to 32 KiB |
| `0e0000–0fdfff` | Big-endian 9-bit pixel values for diagnostic readback |
| `100000–20000f` | Bounded ROM transport reservation |
| End of final wholly owned page | ARM stack |

`MB_ALLOCATION` parameterizes the coarse-table reserve: up to five tables
cover the 4 MiB allocation across section boundaries; a sixth maps the timer.
The presenter starts its two temporary tables after this reserve. Private
pages remain inner WB/WA, outer noncacheable; shared control/table pages remain
noncacheable. PCM/save output is cleaned before ACK, and incoming save RAM is
invalidated before consumption. The host accesses published data only while
ARM is idle. Firmware/MMU control registers are restored at exit.

XANI's XX19c deferred-PAN/ACK design remains the fullscreen presentation path:
three slots in an OS-owned P96 bitmap, ownership challenges under bitmap lock,
ARM colour conversion directly into DDR, checked flip completion and bounded
teardown. The palette now includes all emphasis combinations using Mesen's
`EmulationSettings::GenerateFullColorPalette` calculation. No framebuffer
addresses are hard-coded. No XANI/PicoDrive source was copied.

## Validation and limits

### Original 0.3-dev player validation

Records: `../records/2026-10-09/mesen-player/`.

- Original 120-frame native regression remains **3,571,159 CPU cycles**, raw
  PPU hash **ed354ee5**; ten Right frames yield **9bbb3e9b**.
- **13 independent pinned timing ROMs pass ASan/UBSan**: eight blargg APU
  tests (including length/reload timing, frame IRQ and DMC rates), five MMC3
  tests including A12/scanline timing. MMC3 fixtures leave WRAM disabled, so
  those results are read from their PPU nametable text after 600 frames;
  they are not claimed as `$6000` status results.
- Synthetic tests check bank selection for the original six supported mappers, MMC1
  serial/CHR mapping, MMC3 RAM protection, malformed-header rejection and
  battery RAM round-trip.
- Both 1 MiB and 4 MiB page-table layouts pass **16,384 allocation alignments**
  under ASan/UBSan, including permissions and unmapped unrelated memory.
- Relocated ARM instruction-model tests match six frame hashes and validate
  aligned accesses, four display formats, guards, cancellation and revocation.
  They do not establish physical cache behavior.
- Physical six-frame CORE run: matching hashes, mapping PASS, RET1, zero MMU
  register mismatches, intact witness/canaries, exit 0 and owned-memory release.
- Physical sound/save demo: **120 frames / 7.873175 s = 15.24 FPS**;
  **120/120 acknowledged flips**, three matching framebuffer readbacks,
  **63,850 AHI samples**, **119 detected gaps**. Mean CPU/PPU/APU work was
  **29.54 ms/frame**, ARM colour/copy **3.69 ms/frame**. End-to-end time also
  includes polling, vblank waits, host audio, diagnostics and save/exit work.
  This is a generated demo, not a representative commercial-game benchmark.
  Logs establish successful AHI requests, not audible output at the speakers.
- Physical save/reload: 8,192 bytes; counter 117 reloads and advances to 120;
  `.sav.bak` exactly matches the previous save.
- Physical Q exit: 433 frames, 433 flips and eight readbacks; exits before the
  1,200-frame limit, RET1/exit 0, complete MMU restoration. Workbench, Roadie
  and Amiga-Bridge remain available; Core1 ownership is released.
- The owner missed the brief test; physical picture/speaker acceptance remains
  pending. No screenshot capture was used.

The previous **30.40 FPS** accepted fullscreen preview was silent, NROM-only,
and lacked APU timing/DMC/mapper work. It is not a like-for-like speed claim for
this player. The new result prioritizes correct core behavior and features;
it does **not** meet the approximately 60.10 FPS NTSC target. Compatibility,
sustained play, ROM-requester UI and pause/mute still need owner acceptance.

## Build and run

```sh
python3 scripts/build_mesen_nes.py --arm --player \
  --container-command '["podman","--connection","nuflix-converter-root"]'
python3 scripts/test_mesen_player.py
.context/amiga/mesen-bench/venv/bin/python tests/amiga/mesen_nes/check_arm.py \
  .context/amiga/mesen-player
```

Generated output: `.context/amiga/mesen-player/`. Build verifies source hashes,
runs the original native regression, emits auditable CPU/PPU/APU/mapper
integration patches, then builds freestanding Cortex-A9 and Amiga Hunk code.
The legacy `--fullscreen` build still compiles and passes its native regression.

Installed **0.3.2-dev**: `SD032G:Dev/ZZNesPlayer/ZZNes` (420,832 bytes),
MD5 `d54d086434df49e7f5ece528316a6f29`. Native Amiga MD5Sum verified the
executable, build metadata, readme and checksum manifest. The accepted 0.3.1-dev
executable is retained as `ZZNes.before-optimization` with its metadata/readme/
checksums; MD5 `a1697c90b4f9427ca6b670fe73d69385`. The prior 408,972-byte
0.3-dev executable is retained as `ZZNes.before-mapper90`, with its metadata,
readme and checksum manifest backed up alongside it.

Launch `Execute SD032G:Dev/ZZNesPlayer/Start-ZZNesPlayer` for the requester,
or `Execute SD032G:Dev/ZZNesPlayer/Start-SuperMarioWorld` for the user's
separately supplied ROM. The app is standalone; the bridge is used for
development/deployment, not normal play.

## Mapper 90 update (0.3.1-dev)

The unchanged pinned Mesen `JyCompany.h` now uses the bounded adapter:
PRG/CHR banking, register reads and multiplier at $5000-$5fff, read-only
PRG mapping at $6000, and CPU/PPU IRQ callbacks. Mapper debug reads bypass
IRQ/latch side effects. Supported mapper 90 input is 32–512 KiB PRG and
8–256 KiB CHR ROM, power-of-two sizes, no battery/four-screen flag and zero
header RAM byte. Related mapper 209/211 remain disabled. This is not a
claim of compatibility with every JY board variant.

Records: `../records/2026-10-09/mesen-mapper90/`.

- New synthetic mapper tests exercise bank modes, multiplier, ROM protection,
  mirroring, all four IRQ sources, prescalers, acknowledgement and debug reads.
  All 13 existing timing ROMs and the ARM instruction-model presentation tests
  still pass; both allocation alignment suites pass ASan/UBSan.
- The user's unchanged 786,448-byte mapper 90 ROM completed **900 native
  frames**, scripted Start/A/direction input and cancellation under ASan/UBSan,
  with 327 changing frames. A native preview reached the world map. The ROM
  and preview are private local files, excluded from source/distribution.
- Physical A4000TX: **120 frames / 8.168770 s = 14.69 FPS**, 120 acknowledged
  flips, three checked framebuffer readbacks, matching native raw hashes and
  CPU cycle counts at frames 1/60/120. CPU/PPU/APU work averaged 32.48 ms/frame.
  There were 63,850 AHI samples and 119 detected submission gaps. This bounded
  intro run is not a sustained-play or audible-output acceptance test.
- Clean exit: RET1, zero MMU register mismatches, intact cache witness,
  restored firmware registers and released owned memory. Workbench, Roadie
  and Amiga-Bridge remained available. No physical screenshot was captured.

The approximately 60.10 FPS NTSC target remains unmet. The mapper update adds
cartridge support, not a speed improvement. Owner subsequently confirmed the mapper 90 game works; detailed speaker-quality
and sustained-play measurements remain unreported.

Upstream: [Sour/Mesen](https://github.com/SourMesen/Mesen), commit
`e1180957b31979d4b82d613af9263fcb9442a685` (GPL-3.0-or-later), Shay Green's
blip_buf (LGPL-2.1-or-later), and
[XANI's XACP](https://github.com/Xanxi-Amiga/XACP-ZZ9000).
Pinned original files, licenses and hashes are in `vendor/player/`; fixture
hashes and upstream commit are in `tests/amiga/mesen_nes/player-fixtures.json`.
No commercial ROMs are distributed.

## Optimization pass (0.3.2-dev, 9 October 2026)

Owner confirmed the mapper 90 game works and requested optimization. The first
change replaces the mixer's five-channel scan at every CPU timestamp with a
bounded timestamp bitmap. Deltas are interleaved by timestamp for locality;
events remain chronological, simultaneous channel changes use the same nonlinear
DAC calculation, and end-of-chunk events retain their existing semantics.
No CPU/PPU/APU clocks, frame skipping, sample rate or compatibility settings change.

The 900-frame game trace is identical before/after: combined frame hash
`51e910ee`, PCM hash `ce72c34a`, 479,166 samples. A separate dense-reference
mixer test compares samples across 100 chunks, including simultaneous,
out-of-order, cancelled, word-boundary and end-timestamp events. All 13 timing
ROMs, mapper tests, sanitizer allocation checks and ARM model tests pass.

Matched physical 120-frame intro runs (single short runs, not general benchmarks):

| Build | End-to-end FPS | Mean ARM CPU/PPU/APU time |
| --- | ---: | ---: |
| Installed 0.3.1-dev baseline | 14.95 | 32.51 ms/frame |
| Sparse timestamp mixer | 15.39 | 30.77 ms/frame |

ARM emulation time fell **5.34%**, overall FPS rose **2.97%**. Both runs have
matching frame hashes/cycles, 120 acknowledged flips, three verified readbacks,
63,850 AHI samples / 119 gaps, RET1, clean MMU restoration and exit 0.

A further 0.3.2-dev candidate allows the pending display flip to overlap the
next CPU/PPU/APU frame. It polls ACK and processes Intuition only when the
bitmap is unlocked, drains any remaining flip before AHI/input handling, and
retains the existing pause/exit and bounded flip timeout paths. Additional
host counters separate audio handling and explicit flip draining.

The initial transfer was followed by a network outage before the launch command
could be delivered. Owner reported responsive Workbench and running network
services. After reconnection, native checksum and Core1 ownership checks passed,
and the scheduling candidate completed its physical tests. The outage cause is
unknown; it is not evidence of a candidate execution failure.

### Overlap AHI submission with ARM work

Profiling showed PCM copy/cache maintenance took **0.239 ms/frame**, while
`SendIO` occupied **18.244 ms/frame** of host wall time, including time the host
may be descheduled. This is not a measurement of CPU utilization. The stream
is only 64,000 bytes per emulated second (32 kHz mono, 16-bit), so the measured
copy is a small cost in this setup.

The new path copies completed PCM to a free host-owned buffer while ARM is
idle. After starting the next `NES_FRAME`, the 68k drains the previous display
flip and submits that prepared buffer through AHI while Core1 continues running.
It reads the next shared PCM only after the matching ARM acknowledgement.
An active AHI buffer is never overwritten; the last prepared buffer is submitted
once on clean exit. Audio is scheduled one frame later, and no samples are
resampled, dropped or duplicated to claim speed. The final audio request is
aborted/drained on app teardown as before; submitted counts do not establish
that every sample was heard. This follows the request/buffer ownership rules
in the [AHI device documentation](https://wiki.amigaos.net/wiki/AHI_Device).

| Physical intro run | Frames | Baseline FPS | 0.3.2 FPS | Gain |
| --- | ---: | ---: | ---: | ---: |
| Short comparison | 120 | 14.95 | 22.08 | 47.69% |
| Longer repeat | 300 | 15.06 | 22.53 | 49.56% |

The 300-frame pair produced identical sampled raw/output hashes and CPU cycles,
300 acknowledged flips, six verified bitmap readbacks and 159,692 submitted
samples each. Both restored all MMU registers and cache witnesses and released
owned memory. The interactive candidate was given pause/resume and Q input,
then exited at 429 frames (below its 1,200-frame cap), with 429 flips, eight
readbacks and clean restoration. Its elapsed time includes the pause and is
not a performance benchmark. Roadie and Amiga-Bridge survived; Workbench is back.

The largest gain comes from overlapping AHI submission with emulation; the
sparse mixer contributes the smaller ARM-core improvement. No firmware,
SDL, global AHI preferences, NTSC clocks or emulated-frame skipping changed.
These are bounded intro measurements, not whole-game compatibility/full-speed
claims. There are still audio gaps (299 completed-previous-buffer observations
in the 300-frame test); approximately 60.10 FPS remains the NTSC target.

Owner confirms **ZZ9000AX is physically installed**. This build still opens
AHI unit 0, configured for Paula in the saved inventory. The Music Unit is
configured for ZZ9000AX. Direct Core1-to-AX playback and an HDMI PCM service
are not implemented or verified here. The pinned XX19c source has AX/I2S audio
support, but its register/buffer ownership needs a separate integration review;
do not write into firmware-owned audio buffers or change firmware speculatively.

Evidence: `../records/2026-10-09/mesen-optimization/`.

## Independent audio process experiment — 9 October 2026

Current source builds **0.3.3-dev**, an optional scheduling experiment. Installed
default and normal launcher remain **0.3.2-dev**: the experiment did not improve
performance. No PicoDrive implementation code was copied.

A separate AmigaDOS process owns AHI, its reply port and two playback buffers.
The main task copies acknowledged ARM PCM into an eight-slot bounded host queue;
the worker releases slots only after copying them into its own buffers. Three
blocks prime startup. Pause and shutdown flush short queues; errors propagate
to the main task. Shutdown drains, then aborts if necessary, never unloading
live worker code. Queue publication uses short Forbid/Permit sections on the
single 68k CPU. Worker does not access ARM memory, graphics or the main timer.
No CPU/PPU/APU timing, frame skipping, firmware or global AHI preference changes.

| Matched 300-frame mapper-90 intro | Installed 0.3.2-dev | Audio process |
| --- | ---: | ---: |
| Elapsed time including frontend cleanup | 13.283617 s | 13.622638 s |
| Emulated/displayed FPS, every frame presented | **22.584** | **22.022** |
| Mean ARM core time | 31.141 ms | 31.180 ms |
| Main-task AHI submission wall time | 3.385256 s | 0 (moved to worker) |
| Completed-previous-request gap observations | 299 | 299 |
| Submitted PCM samples | 159,692 | 159,692 |
| Completed PCM samples | not instrumented | 159,692 |

Candidate measured **2.49% slower** in this pair. This is not a statistical claim
about every workload, but gives no basis for promotion. Moving AHI calls does
not remove their underlying CPU cost or guarantee useful overlap. The unchanged
ARM core averages about 31 ms, already above the approximately 16.64 ms NTSC
budget. The ARM ELF is byte-identical to the previous build. Both runs matched
sampled hashes/cycles, 300 flips and six readbacks. Gap observations are not
hardware underruns or proof of audible quality. No owner picture/speaker
acceptance is claimed for this optional candidate.

Additional physical checks:

- `audio_probe.c` independently exercises the worker with no ARM or RTG access:
  all 17,056 silent samples submitted/completed, clean shutdown. AHI rewrites
  io_Length; completion counters now use separately retained sample counts.
- Two-frame game: all 1,020 samples flushed below startup prebuffer threshold.
- Pause/resume, mute/unmute and Q: exit at 193 frames, all 102,719 samples completed.
- Corrected runs: worker error=0/stopped=1, RET1, no restored-register mismatches,
  intact witnesses, all flips acknowledged and Workbench/network restored.

The first Exec-Task candidate was followed by **Guru Meditation**, requiring
owner reboot. Recovered log stops during ARM clock calibration, before the audio
startup message; exact cause remains unproven. Correction uses a DOS Process
because AHI initialization may need DOS services. Passing tests establish the
corrected candidate's tested behavior, not proof of the earlier fault's cause.
Initial executable is quarantined with execute permission removed at
`SD032G:Dev/ZZNes-Diagnostic/ZZNes-audio-task.failed`; do not rerun it.

Corrected optional executable: `SD032G:Dev/ZZNesPlayer/ZZNes-audio-process`,
426,584 bytes, MD5 `d5a4bd29ff0a6158a777a8e8312d9785`. Default remains MD5
`d54d086434df49e7f5ece528316a6f29`. AHI unit 0 still selects Paula, not AX.

`audio_worker.c` tests the actual worker with fake Exec/AHI: 1,200 ordered
variable-size blocks, wrap, bursts/starvation, short flush, allocation/open
failure, cancellation, I/O error, modified request lengths and resource cleanup
under ASan/UBSan. Integrated into `scripts/test_mesen_player.py`; native tests
cannot model AmigaOS scheduling. All 13 timing ROMs, mapper/mixer regression,
ARM model tests and legacy fullscreen build also passed.

Evidence: `amiga/records/2026-10-09/mesen-audio-task/`, including comparison
`summary.json`, both 300-frame logs, failed-run log, probe, controls and checksums.
Temporary transfer server stopped; final hardware state is idle Workbench.

## Additional ROM check: Mario Bros — 9 October 2026

Owner-supplied `011 Mario Bros.nes` (mapper 0, 16 KiB PRG / 8 KiB CHR) was
installed unchanged under `SD032G:Dev/ZZNesPlayer/ROMs/`. Native MD5 matched
`8d8e0b86dd870302bf4a994e76c55c83`. `Start-MarioBros` uses installed 0.3.2-dev.
A 300-frame no-input startup run measured **23.7536 FPS**, mean core time
**28.9605 ms/frame**. All 300 flips, six readbacks and teardown checks passed;
159,692 audio samples submitted, 299 gap observations. This is a separate ROM
workload, not an optimization result or full-speed claim. Owner subsequently confirmed picture, controls and sound all work during the
interactive session. The timed benchmark itself did not test active gameplay.
Evidence: `amiga/records/2026-10-09/mario-bros/`. ROM bytes are not committed.
