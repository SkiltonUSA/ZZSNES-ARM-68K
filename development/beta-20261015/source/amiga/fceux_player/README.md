# ZZFceux — experimental ZZ9000 NES player

FCEUX CPU/PPU/APU on ZZ9000 Cortex-A9 Core1, with the existing ZZNes 68k
frontend: fullscreen ARM framebuffer conversion, XACP XX19c deferred-PAN
triple buffering, keyboard input, pause/mute and AHI audio. This is a separate
candidate; installed Mesen-based ZZNes and the FCEUX headless benchmark remain
available.

Upstream: [Rezzy-dev/fceux](https://github.com/Rezzy-dev/fceux), pinned commit
`4d01530b6689b06a6d7cf0679b63625e3270ae9a`, GPL-2.0-or-later. Selected original
sources and license are in `../fceux_bench/vendor/`. Our integration is
GPL-3.0-or-later. Credit to [Xanxi's XACP platform and PicoDrive example](https://github.com/Xanxi-Amiga/XACP-ZZ9000)
for the documented XX19c firmware interface and fullscreen handoff approach.
No PicoDrive source was copied. SDL is not used.

## Shared game-library interface (0.4.0-ui-preview)

The launcher now shares its MUI layout with ZZSNES: searchable titles,
visible library filters, local artwork, Play/Continue actions and a five-card
save gallery. Existing NES saves remain usable and show "No screenshot".
See [shared UI design, build, migration and test status](../../docs/amiga-game-library-ui.md).
Build the launcher alone with `python3 scripts/build_game_libraries.py`.
The snapshot-enabled 68k host captures a library preview after 300 frames, or
on F9 (also while paused). Pictures are separate from save states; no download
is needed. Both systems passed bounded physical capture/cleanup tests on the
A4000TX. The preview is installed separately; see the guide for remaining tests.

## Player integration and earlier 0.3.0-dev library

Build with `--scheduling release-ui`. This produces **ZZNesLibrary**, the
native MUI 3.8 launcher, and its matching **ZZFceux** player (install as
`ZZFceux-ui` beside the launcher). Previous accepted build modes are unchanged.

The resizable, tabbed interface follows the xSysInfo MUI application structure:
ROM picker and folder import, recent history, favourites, five save slots per
ROM, a live physical-port-2 joystick test, fire-button assignment, mute and
monitor/border settings. It closes its window while the synchronous player
runs, then returns to the library. Library removal never deletes a ROM file.
Settings/history live in `Library.cfg`, with `Library.bak` recovery; no global
Workbench, AHI, firmware or startup preferences are changed.

The Display tab offers 4:3 / 16:9 monitor correction and independent 0/8/16 NES
pixel masks on each axis, with black, white and three original procedural
border designs, plus two optional user-supplied Nintendo side-panel themes
([provenance](../borders/README.md)). See [overscan explanation and the requested reference
images](overscan.md). This masks pixels without zooming the remaining picture.

The active backend is **FCEUX**, not Mesen. Upstream Mesen has snapshots, but
our old extracted Mesen build stripped that facility. This candidate instead
serializes the active CPU/PPU/APU/mapper/controller state, including LQ audio
filter history. `F5` saves the selected slot; `F8` restores it; **Resume slot**
starts a fresh player process from the chosen file. The previous slot remains
as `.bak`. States are private `.zzs` files, tied to ROM content and a state ABI,
not compatible with desktop Mesen `.mst` or FCEUX `.fcs` files. CRC-like FNV
checks catch accidental corruption; these files are not an authenticated or
hardened untrusted-input format.

The bridge transfers at most 32 KiB in the existing owned save area; no ARM,
OS, function pointers or framebuffer state is serialized. Fullscreen ACK/lock
and audio completion precede save/load. Upstream mapper90's state table saved
only four of eight CHR low registers; the opt-in preparation patch saves all
eight. Pinned vendor files are unchanged. See `scripts/fceux_states.py`.

MUI patterns are adapted from [xSysInfo](https://github.com/SkiltonUSA/xSysInfo),
BSD-2-Clause, with its notice retained in `LICENSE-xSysInfo`. Emulation remains
pinned FCEUX GPL-2.0-or-later; this integration is GPL-3.0-or-later.

## 0.2.0-rc1: input, profiling and Workbench package

New joystick support through lowlevel.library, controller artwork and a
Workbench ROM requester defaulting to the adjacent ROMs drawer. Release
package includes an original controller/tone test cartridge only.
Build with `--scheduling release-profile`; standalone `SCHEDULING=release-profile`.
The old build modes and accepted executable remain available.

Physical Mario scripted-Start run: **55.47 FPS / 1,800 frames**, 31 native state
checks and all 958,349 audio samples completed; SMW startup: **54.26 FPS**.
Flip-only scheduling did not improve performance. This release does not claim
a further speed gain or sustained 60 FPS. AX mode allocation passed, but the
player still uses AHI unit 0; physical joystick and new audio acceptance are
pending. See [full findings](../records/2026-10-10/fceux-release/) and
[distribution instructions](ReadMe.txt). Earlier sections below are historical.

## ARM presentation optimisation — later on 10 October 2026

A separate `ZZFceux-fast` candidate uses direct 32-bit palette words for BGRA
and byte-swapped palette words for ARGB, retaining the generic 24-bit fallback.
ARM presentation drops from **3.32 ms to 1.15 ms per frame** (about 65% less
work time, 2.9× throughput for that stage). Colours, emphasis, alpha and every
output pixel match the generic implementation. No NEON or SDL is involved.

| 300-frame startup workload, AHI enabled | Mario Bros | Super Mario World |
|---|---:|---:|
| Accepted scheduling build | 53.71 FPS | 51.92 FPS |
| Faster ARM presentation | **53.99 FPS** | **53.80 FPS** |

Mario's FPS difference is too small to claim a meaningful improvement from a
single run. SMW improves by about 3.6%. Much of the ARM time saved becomes flip
waiting; **60 FPS is still unproven**. The accepted build remains available.

The proposed combined emulation/draw command passed correctness and cleanup
checks, but reduced Mario performance to 50.02 FPS (47.99 with fast packing).
Moving pacing before the flip also failed to improve throughput (52.98 FPS).
Neither experiment is selected. The fast candidate retains the accepted
separate commands, scheduling, audio batching and ownership/cache safeguards.

Build with `--scheduling optimized-fast`. Candidate: **270,212 bytes**, MD5
`4398f22b8de76db50aad845a384b8749`. Separate Shell launchers are
`Start-MarioBros-Fast`, `Start-SuperMarioWorld-Fast`, `Start-ZZFceux-Fast` under
`SD032G:Dev/ZZFceuxPlayer`; invoke with `Execute`. On 10 October 2026, the owner confirmed option 1: Mario Bros Fast
picture and sound are good. This acceptance applies to the binary above; it
does not establish SMW acceptance, measured latency, exact A/V sync or 60 FPS.

The prior accepted live session returned cleanly after 37,191 frames / 677.5 s
(54.89 FPS), with all 19,801,079 audio samples completed. Its inputs and pauses
were not recorded, so it is not a controlled throughput comparison. See the
[pipeline experiment record](../records/2026-10-10/fceux-pipeline/) for long-run
validation, models and failures. The fast build subsequently completed 18,000
frames in 318.48 s (**56.52 FPS**), with all 9,583,486 samples and clean teardown.
Start was injected; subsequent gameplay progression was not supervised, so this
is a stability result rather than a controlled gameplay comparison.
[SNES is on the future roadmap](../../docs/nes-roadmap.md).

## Scheduling optimisation — 10 October 2026

A separate **ZZFceux-optimized** candidate now reaches **53.71 / 51.92 FPS**
with AHI enabled on the same hardware. Fresh 300-frame comparisons:

| Player | Mario Bros (mapper 0) | Super Mario World (mapper 90) |
|---|---:|---:|
| Unchanged installed baseline, measured today | 42.79 FPS | 43.89 FPS |
| Optimised scheduling + three-frame audio requests | **53.71 FPS** | **51.92 FPS** |
| Improvement | **25.5%** | **18.3%** |

Every frame is displayed, the APU remains enabled and all 159,725 / 159,724
samples complete. Timing includes initial framebuffer verification and final
audio drain. These remain short, no-input startup tests; **60 FPS is not yet
achieved**. The ARM loaded image, relocations and entry are unchanged. Gains
come from bounded host polling, overlapping audio delivery with the next ARM
command, batching three PCM blocks per AHI request, and recovering small frame
pacing delays. No sample dropping/resampling or frameskip was introduced.

The three-frame audio batches span about 50 ms at full NTSC speed and can add
buffering latency. The producer retains its eight-slot queue. Pause and exit
flush partial batches; pacing resets after pauses/long stalls. Global AHI
preferences are unchanged: unit 0 still selects Paula Fast 8-bit mono at 8 kHz;
the app supplies 32 kHz signed 16-bit mono to AHI. This is not ZZ9000AX playback.

Both ROMs passed six sampled native frame/cycle comparisons, all-frame flip
checks, first-frame readback and RET1/MMU/register/cache-witness cleanup.
A separate 432-frame interactive test exercised Start, pause/resume, M and Q;
all 230,003 samples completed and Workbench returned cleanly. The owner then
confirmed that the optimised Mario Bros picture and sound are good. Latency
was not separately measured; this acceptance does not cover SMW. Gap-counter
observations were 63/73, down from 299/299, but batching changes the number of
requests: these counters are **not a comparable audible dropout rate**.

Installed alongside the previous default (which is unchanged):

```text
Execute SD032G:Dev/ZZFceuxPlayer/Start-MarioBros-Optimized
Execute SD032G:Dev/ZZFceuxPlayer/Start-SuperMarioWorld-Optimized
Execute SD032G:Dev/ZZFceuxPlayer/Start-ZZFceux-Optimized
```

Candidate MD5: `bf14d8e7dc20970d95fcd110cc9e6754`.
Build it explicitly, then test the generated host worker:

```sh
python3 scripts/build_fceux_player.py --scheduling optimized
python3 tests/amiga/fceux_player/check_host.py
```

Omitting `--scheduling` retains the baseline. Intermediate experiment modes
are preserved for reproduction, including unsuccessful ones; do not install
them as defaults. See the [experiment record](../records/2026-10-10/fceux-optimisation/)
for timings, failed approaches and validation limits.

## Physical results — 9 October 2026

Final candidate: **265,660 bytes**, MD5 `ce77a7b5fd6a453d50a9a697b86d3ae9`.
300 no-input startup frames, AHI enabled, every frame presented:

| ROM | Displayed FPS | Mean ARM emulation + adapter | ARM presentation |
|---|---:|---:|---:|
| Mario Bros (mapper 0) | **44.19** | 5.813 ms | 3.324 ms |
| Super Mario World (mapper 90) | **44.30** | 7.084 ms | 3.323 ms |

Elapsed time includes initial framebuffer verification and final audio drain.
These are actual full-player rates, not the headless core's 164–202 FPS
throughput. **60 FPS has not been achieved.** Startup workload only; no
whole-game compatibility claim. Earlier Mesen player tests measured 23.75 and
22.58 FPS, but they performed six framebuffer readbacks versus one in normal
ZZFceux play, so that is not an identical instrumentation comparison.

Both final runs displayed all 300 frames, matched six sampled native FCEUX
frame hashes/cycle counts, completed all 159,725 / 159,724 audio samples, and
passed RET1, MMU/register restoration, cache witness and memory-release checks.
There were **299 completed-predecessor audio gap observations** in each run.
Zero AHI errors and completed samples do not establish smooth audible playback;
**The owner subsequently confirmed that the picture and sound work** in the
live Mario Bros test. The counter remains a diagnostic observation, not proof
of an audible gap. This acceptance does not establish full-speed playback or
compatibility of other games.

A controls run accepted Start (frame hash changed from the no-input title),
paused beyond the normal duration of its 600-frame limit, resumed and exited
with Q after 360 frames. All flips and 191,669 audio samples completed and
cleanup passed. Mute/unmute keys were dispatched, but audible muting was not
independently confirmed. Live log reads failed while the app held the file
open; final validation used the closed log. No crash/reboot occurred.

Left Mario Bros running for owner testing, `ZZFceux fullscreen` plus Core1
owner port verified. Q/Esc exits. Original ZZNes is unchanged.
Evidence: [logs, provenance and comparisons](../records/2026-10-09/fceux-player/).

### Frontend findings

- Initial integration: Mario Bros 36.41 FPS with periodic full readbacks.
- Sparse diagnostic publication plus first-frame-only readback: 42.50 FPS.
- Shorter 250 microsecond timer waits: 41.78 FPS; reverted.
- Profiling separated ARM presentation (~3.9 ms including reply work) from
  the ~10 ms the 68k observed while yielding with the bitmap locked.
- Bounded polling for `PRESENT` reduced the observed handoff to ~4.4 ms.
  Other requests and frame pacing still yield; the 50 ms presentation deadline
  and reset-before-unlock cleanup remain. Final rates are the table above.

Remaining overhead involves host scheduling, flip pacing and audio delivery.
Do not attribute all of it to decoding or Zorro pixel bandwidth. Further
profiling is needed before claiming a specific AHI/OS cause or a 60 FPS fix.

## Launch and controls

Installed directory: `SD032G:Dev/ZZFceuxPlayer`.

```text
Execute SD032G:Dev/ZZFceuxPlayer/Start-MarioBros
Execute SD032G:Dev/ZZFceuxPlayer/Start-SuperMarioWorld
Execute SD032G:Dev/ZZFceuxPlayer/Start-ZZFceux
```

The last command opens a ROM requester. The first two use the owner's existing
ROMs in `SD032G:Dev/ZZNesPlayer/ROMs`; no commercial ROMs are distributed.
Launchers set a 65536-byte Shell stack and save a log in the player directory.
Close other Core1 applications before launch.

| Key | Action |
|---|---|
| Arrows | NES directions |
| Z / X | A / B |
| Return | Start |
| Left Shift | Select |
| P | Pause/resume |
| M | Mute/unmute |
| Q / Escape | Exit to Workbench |

Current scope: NTSC iNES mapper 0 or 90, CHR ROM, no battery/trainer/four-screen.
Old PPU, hardware eight-sprite limit, quality-0 32 kHz mono audio, every emulated
frame presented. Uses the existing player's RGB palette/emphasis approximation.
This has accuracy tradeoffs compared with Mesen and is not a general FCEUX port.
No battery saves, save states, rewind, joystick support or arbitrary mapper support.

AHI uses **unit 0**, currently Paula on this A4000TX. The fitted ZZ9000AX is not
selected by this player; no HDMI/AX audio or global AHI configuration changes.
The AHI owner is the previously tested DOS Process, with an eight-block queue,
three-frame prebuffer, ordered immutable PCM slots and explicit drain/cleanup.

## Presentation and validation

The adapter packs FCEUX's separate pixel/emphasis buffers into the existing
9-bit pixel-index format and converts 32-bit PCM container values to signed
16-bit samples. The ARM converts completed pixels directly into the owned P96
screen bitmap. The 68k coordinates commands/flips and feeds AHI; it does not
copy each displayed RGB frame across Zorro.

Normal play checks the complete first framebuffer. Every frame still uses
bounded temporary mappings, both-end nonce challenges, mailbox sequencing,
private-frame hash consistency and deferred-PAN acknowledgement. Raw diagnostic
pixel publication/readback is skipped after the initial verified frame.
Diagnostic `VERIFY` restores full readback on frame 1 and every 60th:

```text
Stack 65536
SD032G:Dev/ZZFceuxPlayer/ZZFceux 9fc10110 RUN "SD032G:Dev/ZZNesPlayer/ROMs/011 Mario Bros.nes" 300 VERIFY >SD032G:Dev/ZZFceuxPlayer/audit.log
```

The initial integration copied diagnostic indices every frame and checked the
complete framebuffer every 60 frames. It measured 36.41 FPS for 300 Mario Bros
startup frames, with six successful readbacks and all audio samples completed.
This exposed frontend overhead after the much faster core was integrated.

## Memory and cleanup

Uses the existing runtime mapping probe and private L1/outer-NC MMU contract.
The entire 4 MiB block belongs to a real Exec allocation. FCEUX BSS stays at
allocation-relative `0x220000` below the stack reserve. Frontend pixels/PCM and
ROM occupy the existing disjoint player ranges below that. No firmware shared
DDR allocation is invented. The strict FCEUX ELF packer validates relocations.

A locked bitmap outlives outstanding ARM writes; framebuffer mappings are
revoked before ACK. Core1 RET1, synchronous reset, register restoration and cache
witness checks precede memory release. AHI drains and its DOS Process exits
before unloading code. Default ZZNes, firmware, drivers and Roadie are untouched.

## Build and checks

```sh
python3 scripts/build_fceux_player.py \
  --container-command '["podman","--connection","nuflix-converter-root"]'
python3 tests/amiga/fceux_player/check_native.py /private/game.nes 300
.context/amiga/mesen-bench/venv/bin/python tests/amiga/fceux_player/check_arm.py /private/game.nes 120
make test-amiga-arm test-amiga
```

Outputs live in `.context/amiga/fceux-player`. `frontend.patch` records generated
adaptations of the existing frontend/worker; shared production sources remain
unchanged. Native tests compare every pixel/emphasis and PCM sample, scripted
Start input and cancellation. ASan/UBSan exclusions for upstream biased bank
pointers are documented in `../fceux_bench/README.md`.

ARM instruction-model checks compare native frame hashes/cycles and verify four
framebuffer byte formats, challenges, padding guards, stop handling and mapping
revocation. Host tests cover triple-buffer handoff, AHI queue ownership and
shutdown. Model/native tests do not establish physical display, audible quality
or physical cache coherence; hardware results are recorded separately.
