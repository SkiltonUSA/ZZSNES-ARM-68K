# ZZSNES PAL and Contra III compatibility — 11 October 2026

The region build adds automatic PAL/NTSC selection to the PocketSNES-based
ZZSNES player and matching MUI library. It keeps the existing XACP/XX19c
fullscreen flip lifecycle, owned 32 MiB allocation, LUT/STMIA presenter,
stereo AHI output and per-game save slots. No ROM or firmware is patched.

## Regional cadence experiment — 12 October 2026

The next isolated host scheduler keeps a regional emulation deadline instead
of resetting it to each display handoff. Every CPU/PPU/APU frame still executes
and every PCM sample is submitted at the original 32,040 Hz. If presentation
would miss its budget, the host omits that completed picture, with at most two
consecutive omissions. The existing C0 four-slot ownership, previous-front
retention and previous-PAN drain remain intact. The ARM payload, fullscreen
code and AHI worker are byte-identical to the control. This reduces dependence
on display timing; it does not make the host pipeline asynchronous.

`amiga/pocketsnes/library/cadence.h` implements the bounded policy. The first
picture always displays; a stall exceeding 250 ms rebases the deadline rather
than allowing a long catch-up burst. Pause/thumbnail pacing reset is preserved.
There is no playback-rate shift or additional audio buffering: eight slots,
six-frame prebuffer, as in the original player.

Native ASan/UBSan checks cover PAL/NTSC, timer wrap, long stalls, bounded catch-up
and 200,000 handoffs using the actual C0 slot planner. Strict 68k builds pass
`-Werror`. These tests check policy and ownership, not physical scanout timing.

The automatic-thumbnail regression passed on hardware: capture at frame 300,
continued beyond frame 301, completed 600 frames, validated the private 128×112
thumbnail, and restored checked MMU/register/witness state before clean exit.
This closes the previously pending physical capture test for `pace_target.h`.

| 1,200-frame SMW comparison | Control | Cadence |
| --- | ---: | ---: |
| Emulation frames per second, including startup | 56.267 | 58.952 |
| Presented pictures per second | 56.267 | 57.135 |
| Omitted pictures | 0 | 37 (3.08%) |
| Audio gap indicators | 89 | 0 |
| Host ACK intervals over 20 ms | 84 | 22 |

Both runs completed the same 641,626 stereo sample pairs, identical sampled
game hashes, no audio/flip errors, checked restoration and clean exit.
One startup deadline rebase occurred in the candidate. Audio gap indicators
are not measurements of audible dropouts; host ACKs are not scanout timestamps.
The earlier horizontal game-image split remains unproven either way.

### Five-minute hardware comparison

Each variant completed 18,000 SMW frames using identical scripted input.
Steady rates use progress samples from frame 600 to frame 18,000, excluding
initialization and first-picture readback. The original core target is about
59.9227 Hz; the host uses a rounded 16,689 us period.

| Measurement | Control | Cadence |
| --- | ---: | ---: |
| Whole-loop duration, seconds | 311.848 | 300.720 |
| Whole-loop emulation fps | 57.720 | 59.856 |
| Steady emulation fps | 57.758 | 59.920 |
| Steady presented pictures/second | 57.758 | 58.053 |
| Omitted pictures | 0 | 561 / 18,000 (3.12%) |
| Audio gap indicators | 1,005 | 0 |
| Host ACK intervals over 20 ms | 998 / 17,941 | 390 / 17,384 |
| Maximum host ACK interval | 51.110 ms | 51.563 ms |
| Generated PCM duration minus elapsed steady wall time | −10.885 s | −0.014 s |

The candidate holds regional emulation/audio cadence in this run without
changing pitch or queue capacity. It does not eliminate all long display
intervals: maximum ACK latency is similar and picture omissions remain a
visible-motion tradeoff. The control processes the same number of frames but
takes longer, accounting for much of its audio starvation. PCM-versus-wall
time here measures production, not the hardware audio clock or acoustic sync.

Both runs queued, submitted and completed all 9,624,392 stereo sample pairs,
matched sampled game hashes, completed all requested flips and reported AHI
error 0, no flip fault, stopped worker, checked MMU/register/witness restoration
and clean exit with memory released. Candidate: one initial deadline rebase,
maximum measured lateness 18,524 us, at most two consecutive omitted pictures.
There is one long pair, not repeated statistical coverage across games.
The user subsequently played the interactive candidate and reported “so much
better!”. This supports retaining the policy; it does not separately establish
that every previously observed horizontal split has disappeared.

The short physical PAL check used Cannon Fodder (Europe), 1,200 frames each.
Both control and candidate presented all 1,200 pictures with no omissions,
zero gap indicators and all 764,073 stereo pairs completed. Sampled hashes
matched and both restored/exited cleanly. Whole-loop rates including startup
were 49.735/49.743 fps, using the core's 19,873 us PAL period. Host ACK
intervals over 20 ms were 209/215: no demonstrated PAL improvement in this
short comparison. A 20 ms threshold is much closer to the PAL frame period
than NTSC, so those counts cannot be compared directly between regions.

Interactive candidate, real joystick/keyboard with scripted input disabled:

```text
Execute SD032G:Dev/ZZSnes-Cadence/Start-manual-Interactive-18000
```

Return is Start; the standard joystick uses physical port 2 (lowlevel port 1).
Q/Esc exits, P pauses. The run is capped at 18,000 emulated frames (about five
minutes NTSC) and has a separate 385-second loop limit. It writes no SRAM,
save states or thumbnails; its log is `Visual-manual-Interactive-18000.log`
in the same drawer. This is a diagnostic candidate, not the normal app or a
release. User feedback: “so much better!” after the interactive test.

Build and run only one hardware stream at a time:

```sh
PYTHONPATH=. python3 scripts/build_pocketsnes_cadence.py --variant Baseline --variant Cadence --frames 18000
.tools/amiga-venv/bin/python scripts/run_pocketsnes_cadence.py --series unique-name Baseline-18000 Cadence-18000
python3 scripts/analyze_pocketsnes_cadence.py /path/to/completed.log
```

The build depends on the verified local C0/audio-game source cache and checks
its hashes; it is not a clean-checkout release build. The runner stages only
isolated diagnostics at `SD032G:Dev/ZZSnes-Cadence`, verifies upload hashes and
teardown, and stops the sequence on failure. It bounds both outer and pacing
waits. Private ROMs, snapshots and user saves are excluded from evidence.
The normal installed player/library have not been replaced.
Evidence: `amiga/records/2026-10-12/zzsnes-cadence/`.

## Audio scheduling isolation — 12 October 2026

Following the user's smooth Core / slightly jittery Audio observation, built
bounded C0-derived comparisons with unchanged ARM payload and fullscreen
handoff. Four immutable patterns isolate audio load from framebuffer writes.
All use 1,200 emulated frames, identical scripted input and a 45-second loop
limit, Q/Esc cancellation, and no SRAM/state/snapshot transactions. Per-frame
host timing and audio counters stay in bounded RAM arrays and print after
playback closes. ACK observations are not scanout timestamps; the AHI gap
counter means the preceding linked request was already complete, not a
measured audible dropout. Background OS scheduling remains a source of noise.

| Immutable-pattern variant | Runs | Host ACK intervals >20 ms per run | AHI gap indicators per run |
| --- | --- | --- | --- |
| Core, no PCM transfer/playback | 2 | 3, 7 | 0, 0 |
| PCM transfer only | 1 | 6 | 0 |
| Existing AHI scheduler | 3 | 22, 24, 26 | 20, 25, 19 |
| Wake worker immediately after publishing PCM | 2 | 61, 53 | 97, 80 |
| Extra reserve: 12 slots, 9-frame prebuffer | 2 | 13, 15 | 11, 10 |
| Extra reserve plus 31,828 Hz diagnostic playback | 2 | 17, 7 | 9, 0 |

PCM copying costs about 464 us/frame in the copy-only comparison. Core1 still
spends about 6.3 ms/frame emulating the same game. Immediate worker notification
worsens the measured cadence, so do not promote it. Extra buffering helps but
cannot eliminate drift: baseline startup submits both three-frame requests
from the six-frame prebuffer, leaving no queued reserve. Nine prebuffer frames
leave three extra frames (about 50 ms), at the cost of that extra audio latency
and 16,400 additional bytes for four queue slots. Gap events are delayed rather
than eliminated; this is a mitigation, not stable long-run synchronization.

The clock diagnostic keeps nine-frame prebuffering but asks AHI for 31,828 Hz
instead of 32,040 Hz: rounded 32040 * 16689 / 16800, specific to this NTSC test
and the mode's observed nominal 16.8 ms interval. It slows/pitch-shifts audio by
about 0.66%, so is not a production fix or a PAL setting. Some startup edge
samples miss refreshes; do not treat their averages as a mode change. The two
clock runs varied substantially (9 versus 0 gaps), and transient late frames
remain. A fixed rate correction cannot compensate arbitrary stalls.

All twelve pattern runs completed 1,200 frames and flips, 1,201 ACKs, zero
ARM PRESENT requests, unchanged ba3ce5c5 display hash, matching sampled game
hashes, checked MMU/register/witness restoration and clean exit. Every playback
run queued, submitted and completed exactly 641,626 stereo pairs, with AHI
error 0 and worker stopped. No full-ring waits. These results do not resolve
the earlier horizontal split in actual game rendering.

Reproduce from the verified C0 build cache:

```sh
PYTHONPATH=. python3 scripts/build_pocketsnes_loaded_pattern.py --audio-study
PYTHONPATH=. python3 scripts/build_pocketsnes_loaded_pattern.py --audio-study --variant Reserve --variant Clock
PYTHONPATH=. python3 scripts/build_pocketsnes_loaded_pattern.py --audio-study --game --variant Audio --variant Reserve
python3 scripts/analyze_pocketsnes_audio.py /path/to/completed-run.log
```

Only isolated diagnostics are installed at `SD032G:Dev/ZZSnes-AudioStudy`.
The normal Borders player and library are unchanged. All 68k builds use
`-Werror`; the bounded trace and enlarged queue pass ASan/UBSan tests, including
short flush, state drain/resume, ring wrap, allocation/device failures and
cancellation. Evidence: `amiga/records/2026-10-12/zzsnes-audio-study/`.
A log collection conflict during the first Core run was caused by concurrent
bridge file requests; that run exited cleanly but was excluded and repeated.
Use a single bridge client stream during log collection.

The next scheduling work should measure producer-versus-consumer drift over
longer runs and decouple emulation/audio timing from display acknowledgement
cadence with bounded queues and an explicit presentation policy. Do not just
keep enlarging the buffer or hard-code the diagnostic playback frequency.

### Actual-game comparison and thumbnail pacing fix

After preserving real C0 rendering, the first control captured an automatic
library thumbnail at frame 300. The snapshot reset `next_frame_us` to zero,
but the old pacing expression interpreted that sentinel as a distant future
time when the absolute timer's high bit was set. Its inner wait bypassed the
outer test deadline. The watchdog halted the sequence; Q exited normally at
300 frames, with full ARM restoration and owned memory released. It created
only the diagnostic `ZZSnes-PatternLoad/unused.zzp`, not a user save state.
This cancelled run is excluded from performance comparisons.

`pace_target.h` now treats zero as a reset, rejects implausible deadlines and
preserves the normal two-frame lead across timer wrap. The region source
generator uses it; native tests exercise PAL/NTSC and thumbnail resets during
frames at high/low/wrapping timestamps. Both game test builds use the fix,
clear the thumbnail path before initialization, and enforce the test deadline
inside pacing as well. The normal installed app is unchanged; fixed automatic
thumbnail capture was subsequently verified by the 600-frame cadence regression above.

| Actual-game variant (1,200 frames each) | ACK intervals >20 ms | AHI gap indicators | Mean host ACK interval |
| --- | --- | --- | --- |
| Existing audio queue | 72 | 74 | 17,447.9 us |
| Extra reserve, unchanged 32,040 Hz | 58 | 63 | 17,330.1 us |

The extra reserve reduced these counters by about 19% and 15% respectively in
this one pair. It did not eliminate jitter, and needs a subjective comparison
before promotion. Worst ACK intervals remain about 50.4 ms. Both runs pass
1,200 FRAME/PRESENT/flip operations, identical sampled game hashes, all 641,626
stereo pairs played, no AHI errors or full-ring waits, and clean restoration.
Game presentation costs about 2.95 ms/frame on Core1 in addition to roughly
6.3 ms emulation. No ARM kernel or firmware changes were made.

Isolated Amiga launchers (roughly 20 seconds plus startup/checks, scripted Start,
Q/Esc exits, saves disabled):

```text
Execute SD032G:Dev/ZZSnes-AudioGame/Start-Audio
Execute SD032G:Dev/ZZSnes-AudioGame/Start-Reserve
```

Run each after the previous exits. Evidence, including the cancelled first
control: `amiga/records/2026-10-12/zzsnes-audio-game/`. The visual result and
original horizontal-tearing symptom remain unverified by these counters.

## Isolated synchronization comparisons — 12 October 2026

`scripts/build_pocketsnes_tearing.py` builds four players from the verified
region-aware build cache. They are experiments, not replacements for the
installed Borders player:

| Variant | Vertical picture | Display slots |
| --- | --- | --- |
| A | Existing 224/239 to 216-row scaling | 3 |
| B | Native 224/239 rows | 3 |
| C | Existing 224/239 to 216-row scaling | 4, retaining the previous front |
| D | Native 224/239 rows | 4, retaining the previous front |

All use the same black, unmasked CLI default and unchanged horizontal sizing.
Native vertical output preserves every source row, centered at output row 8
for 224-line pictures and row 0 for 239-line pictures. The latter has a single
bottom border row. Source-coordinate overscan masks remain supported. Since
horizontal sizing is unchanged, native height slightly changes the displayed
aspect; this comparison does not introduce a final aspect-ratio policy.

The builder recompiles the control presenter and requires its packed ARM
image, relocations and entry point to match the accepted payload exactly.
Only the native-height presenter changes ARM code. A/C share one payload;
B/D share another. Four slots add 307,200 bytes of display storage at a
1,280-byte pitch. Core state, audio, clocks, MMU/cache policy, the 32 MiB core
allocation, and reset-before-free ownership remain unchanged. Presentation
remains serialized; unrestricted rendering overlap is not enabled.

The XACP PAN acknowledgement is a firmware observation, not an independently
verified DMA scanout fence. Retaining a buffer longer is an experiment, not
a proof of tear-free output. Trial counters distinguish ACKs observed during
blanking/active display, but cannot establish the instant the DMA latches its
address. Frame-interval histograms also timestamp host ACK observation, not
monitor refresh. Keep scaling shimmer, uneven frame pacing and tearing as
separate hypotheses.

Build and local checks:

```sh
python3 scripts/build_pocketsnes_tearing.py
python3 tests/amiga/pocketsnes/check_tearing_host.py .context/amiga/pocketsnes-tearing/D
.context/amiga/mesen-bench/venv/bin/python tests/amiga/pocketsnes/check_regions_arm.py \
  .context/amiga/pocketsnes-tearing/B /private/path/ntsc.sfc /private/path/pal.sfc
```

The native-height unit checks every surviving row exactly once for both
heights and all three mask sizes. The exhaustive presenter test covers both
heights, valid border/monitor/mask settings and both pixel formats under
ASan/UBSan. Generated host tests exercise 200,000 serial handoffs per variant,
ACK wrap, lock retention, timeout and unexpected-ACK cancellation. A9
instruction tests cover relocated state replay, framebuffer nonce/row guards
and temporary mapping revocation. These do not replace visual acceptance.

### Physical comparison results

Installed only in `SD032G:Dev/ZZSnes-Tearing`. `Execute Start-A`, `Start-B`,
`Start-C` and `Start-D` launch the respective SMW comparisons; Q/Esc exits.
Each has a separate save-state/snapshot path. Copies of the test ROMs and
their automatic SRAM files live in this drawer. The normal Borders player,
library, settings and user save states are not replaced.

All four variants completed 600-frame NTSC runs with 11 readbacks each;
variant D also completed a 600-frame PAL run with 11 readbacks. The 1,800-frame
timing runs use only the initial readback and the same scripted Start input:

| Variant | Measured FPS | Audio gap indicators |
| --- | ---: | ---: |
| A, first control | 57.050 | 91 |
| B, native rows | 56.718 | 104 |
| C, four slots | 56.773 | 104 |
| D, both | 56.765 | 103 |
| A, repeated control | 56.511 | 116 |

This is a bounded startup/Start-input sequence, not a gameplay-wide or
scrolling benchmark. The elapsed totals include startup readback and frontend
shutdown. Control variation spans the candidate results: there is no demonstrated
speed improvement or reliable ranking. ARM presentation averages about
2.944 ms for the scaled picture and 3.014 ms for native height. There is a
small extra native-row rendering cost, without a demonstrated significant
end-to-end regression in this sample.

All ten runs completed the requested frame/flip counts, verified their
readbacks, delivered every queued stereo sample, restored the checked ARM
registers/MMU and released ownership after RET1. The emulated frame hashes
match across like-for-like variants. Audio completion does not prove continuous
playback: gap indicators remain present. No run entered the full-audio-queue
wait path, so producer backpressure is not the observed bottleneck here.
Host frame/ACK intervals still contain longer outliers; actual display cadence
and presentation deadlines need separate investigation before adaptive audio
resampling is justified. Small resampling corrections cannot repair sustained
missed frame deadlines.

Variant D also passed paused save/load, snapshot capture, resume/quit and
fresh-process state resume, using an isolated slot and ROM copy. Original
Borders player/library checksums were rechecked unchanged afterward.

After these automated runs, the user reported that C was the best of the
variants. C is therefore the preferred visual candidate, retaining the existing
vertical scaling with four-slot delayed retirement. The user subsequently
clarified that horizontal tearing is less noticeable, but still present.
This subjective improvement does not establish the root cause; C provides
the preferred baseline for further presentation-timing comparisons.
**Tear-free scanout is not verified, and no variant is promoted to the normal
player.** The additional tests above were performed on D; subsequent
C-specific PAL and save/load checks passed in the buffer-release comparison below.
Hardware logs, artifact hashes and local test results are retained in
`amiga/records/2026-10-12/zzsnes-tearing/`. Private ROMs, states and screenshots
are excluded from those records.

## Buffer release timing — 12 October 2026

The user clarified that C is only slightly better and horizontal tearing
remains noticeable. The next comparison holds its four-slot plan, scaling,
audio and ARM payload constant while changing when the bitmap lock is released
after a deferred-PAN acknowledgement. It does not alter the firmware or write
VDMA registers directly. Sources: `scripts/build_pocketsnes_scanout.py` and
`amiga/pocketsnes/library/scanout_gate.h`, `scanout_measure.h`.

The pinned XX19c firmware source updates the requested pan state and increments
the ACK in its main loop. Its video ISR calls `init_vdma` using that state.
Those are distinct operations; neither the ACK nor the following experiment
proves that the DMA address has latched for a particular scanned frame.

| Trial | Release behavior | 1,800-frame timing result |
| --- | --- | --- |
| C0 | C behavior with startup blanking measurement | 57.171 FPS; repeat 57.109 |
| E | If ACK arrives during blanking, wait until active display | 56.504 FPS initially; later withdrawn |
| F | Observe a further active/blank/active cycle | Withdrawn: timeout after 547 frames |

E delayed 1,708 of 1,800 releases, averaging about 1.36 ms for those waits.
It costs about 0.6 FPS in this sample. The user subsequently reports that
the issue persists and identifies a horizontal split/offset, not simply
whole-picture jerking: E did not resolve the reported tearing.
Its audio gap indicators remain present (120 versus 90 in the first C0 run).
Three 600-frame NTSC checks (C0/E/F), three completed 1,800-frame timing checks
(C0/E/C0), and 600 PAL frames on the original C passed readback, expected
frame/flip counts, completed PCM, RET1, MMU/register restoration and memory
release. Like-for-like emulated frame hashes matched.

Original C and E also passed paused save/load, F9 snapshot capture, resume/Q,
and fresh-process state resume using private files. E-specific PAL was not
physically tested. Final checks found only Workbench, no Core1 task, Roadie
and bridge running, and unchanged normal player/library checksums.

F completed its initial 600-frame check but was much slower. In its longer
run, the 50 ms deadline expired before the required blanking sequence was
observed. It exited with status 20 at frame 547, cancelled the pending gate,
restored checked registers/MMU with no mismatches and released its allocation.
The exact reason for missing the sequence is unproven; do not interpret it as
a board failure. The deadline was not relaxed to hide the failure. F is not
a usable candidate: Start-F now only displays a withdrawal notice, and the
binary is renamed `ZZSnes-F.withdrawn` with execute protection removed. A slower picture could
also mask artifacts, so apparent improvement in F would be inconclusive.

The startup sampler reads the firmware's FPGA blank-status bit without a
bitmap lock and services window events. Clean runs observed approximately
16,800 microseconds between rising edges (~59.52 Hz). Other runs contain
intervals close to multiples of this period: one C0 sample spans 33,599 us.
Polling gaps can inflate the reported mean; these samples do not prove a
variable monitor refresh or give timestamps of DMA address latching. The
core's 59.922743 FPS target and these observed display intervals differ, but
that alone does not explain the remaining tearing or missed frame deadlines.

Native sanitizer tests cover 200,000 actual host handoffs per timing variant,
ACK wrap, locked gate rejection, required edges, stuck status, timeout,
unexpected ACK and cancellation. Sampler tests cover timer wrap, bounded
failure and event cancellation. All three 68k builds pass `-Werror`; their
embedded ARM image and ELF are byte-identical to C. Physical evidence and
hashes: `amiga/records/2026-10-12/zzsnes-scanout/`.

Trials live in `SD032G:Dev/ZZSnes-Scanout`, with private ROM/SRAM and
save-state paths. C0 remains available via `Execute Start-C0`; Q/Esc exits.
Existing Borders and earlier A/B/C/D players are unchanged. E/F are withdrawn.
E failed visual acceptance as a tearing fix. Its completed extended-session
log additionally records a gate timeout after 30,524 frames, exit 20 and
owned memory released. The 50 ms wait did not remain reliable over this longer
run, despite passing short checks. Start-E now prints a withdrawal notice;
`ZZSnes-E.withdrawn` has execute protection removed. The exact cause of the
missed observation is unproven. No adaptive audio resampling is introduced.

The supplied [NESdev DMA reference](https://www.nesdev.org/wiki/DMA) describes
NES sprite/audio transfers and CPU timing. That is a guest-emulation accuracy
reference; it does not describe the ZZ9000's display DMA. No NES DMA or SNES
core timing changes were made in this comparison.

## Immutable-image diagnostic — after E failed visual acceptance

`scripts/build_pocketsnes_pattern.py` builds `ZZScanout-Pattern` from the
verified C0 host. The diagnostic fills four owned screen-bitmap slots once
with low-contrast diagonal lines, holds one completed picture for five seconds,
then pans between the immutable images for twenty seconds. Adjacent phases
translate the repeating lines by eight pixels. There is no full-screen
brightness alternation. Q/Esc exits early; switching away cancels.

Core1 initialisation and its existing return/reset/ownership lifecycle remain,
but the comparison sends no SNES FRAME or ARM PRESENT requests and opens no
audio output. SRAM and save-state loading/storing are disabled. It uses the
existing private test ROM only for core initialisation. No new ARM payload,
firmware, peripheral register map or shared allocation is introduced.

Each flip re-locks and validates the actual P96 screen bitmap, including its
current address, stride and format. Pixel checksums before and after the test
check for persistent image changes. Only PAN requests occur during motion;
application pixel-writing never overlaps the comparison. The deferred-PAN ACK
still does not establish which image the monitor is scanning. Checksum equality
cannot exclude transient changes or prove tear-free scanout.

Interpretation:

- A horizontal break with these immutable patterns reproduces the issue in the
  display path without SNES rendering or concurrent application pixel writes.
  It would justify examining firmware/driver/mode/monitor timing, but would
  not identify any one of those as the cause.
- A clean pattern test does not exonerate that path under the game's CPU/audio
  load; the next comparison would reintroduce those loads separately.
- Uneven whole-image motion is a pacing observation, distinct from a diagonal
  line breaking across a horizontal boundary.

Native ASan/UBSan checks cover both pixel formats, every aligned supported
stride, all four image phases, padding guards, immutable buffers across mock
flips, bounded completion, quit, foreground loss, invalid stride and queue
failure. The 68k build passes `-Werror`; its embedded ARM image is identical to
C0. These checks do not substitute for physical scanout observation.

Installed separately at `SD032G:Dev/ZZSnes-Pattern`. Run
`Execute SD032G:Dev/ZZSnes-Pattern/Start-Pattern` with no other Core1 app active.
The normal emulator is unchanged. Logs and build records are retained under
`amiga/records/2026-10-12/zzsnes-pattern/`. Physical results are recorded there;
visual acceptance requires the user's observation of the monitor.

The first bounded physical run completed 1,129 moving flips (1,130 ACKs
including initial selection), with unchanged `ba3ce5c5` checksums and no
flip fault. Checked registers/MMU and witnesses restored; exit 0 released
owned memory and returned to Workbench. After initially missing the automatic
test, the user reports the moving diagonal patterns looked good, with no
horizontal split. The fault was not reproduced in this workload; this does
not prove that buffer switching remains correct under normal game load.
`FULLSCREEN_END queued=0` counts ARM presentation challenges, which this
diagnostic intentionally does not issue; it is not the PAN flip count.

### Loaded immutable-pattern comparisons — 12 October 2026

`scripts/build_pocketsnes_loaded_pattern.py` builds two separate players from
hash-verified C0. Both retain its ARM payload, exact fullscreen handoff and
normal FRAME/previous-flip-drain/audio/next-flip scheduling. Only the normal
presenter is replaced with selection of an already-filled pattern slot.

| Script | SNES CPU/PPU/SPC/DSP | PCM transfer and AHI output | ARM framebuffer presenter |
| --- | --- | --- | --- |
| Start-Idle | Initialises only | Off | Off |
| Start-Core | Runs each frame | Off | Off |
| Start-Audio | Runs each frame | On | Off |

Audio initialisation failure aborts the Audio test, rather than silently
converting it into a second muted control. Both loaded tests issue zero ARM
`NES_PRESENT` requests (an explicit guard rejects any attempted request), and
write the pattern pixels only during initialisation. The SNES core still
renders game pictures to its private output and generates PCM in both variants.
The original idle diagnostic is copied unchanged as the bracketing control.

Tests use the existing private SMW ROM and identical scripted Start input.
SRAM/state loading and saving, and F5/F8/F9 transactions, are disabled. Loaded
runs stop at 1,200 frames or a 45-second main-loop deadline, plus in-flight
request and cleanup time. Q/Esc exits; the deadline also applies while paused.
This is a bounded startup/input sequence, not coverage of every game scene.
No audio assembly, guest timing, firmware, extra blanking gate or normal
installed emulator is changed. E and F remain withdrawn.

The native ASan/UBSan helper test passes 500 handoffs with unchanged images,
retained locks, corruption detection, unexpected-present detection, foreground
loss and queue/stride failure paths. Both 68k builds pass `-Werror`; generated
ARM image, ELF and fullscreen header hashes match C0. These local checks do not
establish tear-free physical scanout. Build and hardware evidence is retained
in `amiga/records/2026-10-12/zzsnes-loaded-pattern/`.

The isolated drawer is `SD032G:Dev/ZZSnes-PatternLoad`. Run `Execute Start-Core`
or `Execute Start-Audio` there, one at a time. Watch for a horizontal break in
the diagonal lines separately from uneven whole-picture movement. A split in
a loaded pattern narrows the relevant workload but does not establish a
firmware defect. If both remain visually clean, isolate framebuffer writes,
publication/reuse and completed game-picture content next.

Physical Idle/Core/Audio/Idle runs all completed without a flip fault and
restored the checked MMU/register/witness state before exit 0/releasing owned
memory. Core and Audio each completed 1,200 frames and immutable-pattern
flips, plus one initial selection (1,201 ACKs). Each reports zero ARM PRESENT
requests and unchanged `ba3ce5c5` display checksums. Sampled game-frame hashes
match between Core and Audio. The idle controls completed 1,132 and 1,133
moving flips respectively.

The Audio worker queued, submitted and completed all 641,626 stereo sample
pairs and stopped with error 0. Twenty audio-gap indicators remain; completion
is not proof of uninterrupted playback. Neither loaded test entered a full
queue wait. The final state was Workbench, no Core1 task, and unchanged normal
Borders player/library checksums.

The user subsequently ran both tests manually: **Core was “super smooth”;
Audio had slight jitter twice in about twenty seconds.** The user clarified
that the whole pattern hesitated/jumped, with no horizontal split. The loaded
patterns therefore reproduce a small whole-frame stutter when audio is enabled,
but do not reproduce the earlier game-image splitting.
Both manual logs again pass 1,200 frames, unchanged pattern checksums, zero
ARM PRESENT, restoration and clean exit.

| Manual run | Sampled host ACK intervals | Intervals >20 ms | Maximum | Audio gap indicators |
| --- | ---: | ---: | ---: | ---: |
| Core | 1,141 | 3 | 33,609 us | 0 (output off) |
| Audio | 1,141 | 27 | 34,565 us | 24 |

The histogram excludes startup frames and measures when the 68k observes ACKs,
not scanout instants. Audio completed all 641,626 stereo sample pairs without
an error or full-queue wait. These aggregate counters do not identify which
events caused the two visible jitters. The result associates added PCM/AHI
work with less even host timing, but does not prove an AHI driver defect or
explain the earlier game-image split. Next isolate PCM copy/queue work from
AHI worker scheduling and collect per-frame event timing before changing
priorities or buffering. Evidence: `zzsnes-loaded-pattern/manual/`.

No new hardware run, core/firmware replacement or audio setting change was made
in response to this feedback. Successful checksums, ACKs and lifecycle checks
do not establish a general tear-free scanout contract.

## What changed

The following region changes remain the basis of all display experiments.

- Accept standard LoROM/HiROM region codes 0–12; 0/1 select NTSC, 2–12 select
  PAL, matching this pinned core. Enhancement-chip cartridges remain rejected.
- Pass region timing and completed-picture height from ARM to 68k. The host
  checks this metadata and uses the appropriate scheduling deadline.
- The pinned snes9x2002 core models NTSC at 59.922743 FPS and PAL at **50.319740
  FPS**, with 16,689/19,873 microsecond rounded host deadlines. This is the
  core's PAL approximation, not a claim of exact 50 Hz emulation or a 50 Hz
  monitor mode. The existing 320×240 RTG mode is retained.
- Accept 256×224 and 256×239 pictures; preserve the full picture within the
  existing private buffer and scale it to the bordered viewport. Store height
  in reserved save-header bytes; old height-zero saves remain 224-line saves.
  Separate height and pixel-format bits in the presentation cache key.
- Define the intended wrapping envelope arithmetic and Mode 7 sign extension
  without signed-shift undefined behaviour found by the new ROM tests.

## Contra III

The supplied USA image is a 1 MiB LoROM with no enhancement chip. Its internal
name is 22 bytes, `CONTRA3 THE ALIEN WARS`: the final `S` occupies the position
normally used for the map byte. Our strict loader previously rejected its
value `0x53`. A narrow exception checks this title, size, region, ROM/SRAM
codes and chip type, while retaining checksum-complement/reset-vector guards.
It does not accept arbitrary nonstandard map bytes.

The ROM's calculated 16-bit checksum is `0x0c3c`, matching its stored checksum;
its complement is `0xf3c3`. MD5: `ede82658dafdd6febbfbe6a9c9082500`.
The supplied image was copied without modification. Header validation remains
an admission check, not a guarantee of complete game compatibility.

## Validation and limits

All eight supplied games completed 600 native frames under AddressSanitizer
and UndefinedBehaviorSanitizer. Audio counts match the core clocks: 320,813
stereo pairs for NTSC and 382,036 for PAL at 32,040 Hz. Aero Fighters emitted
no nonzero samples in this startup sequence; this does not verify its music.

Exact 20-frame save replay passes for six games, including Contra III and
Cannon Fodder. **Another World and The Chaos Engine differ visually on the
first frame after restoring the tested save.** Their audio and following 19
frames match. This remains an open strict-replay failure, not a passed test
or a guarantee of complete save compatibility. No speculative snapshot fix
was deployed.

Synthetic sanitizer tests exercise both heights, mode transitions, old save
height, malformed headers, border masks, pixel formats and row guards. A9
instruction-model tests exercise PAL/Contra startup and state relocation at
two allocation bases, plus 224/239 presentation, challenge rejection, row
padding and temporary mapping revocation. Synthetic 239-line coverage does
not establish that every extended-height game works.

Physical run results are recorded in
`amiga/records/2026-10-11/zzsnes-regions/hardware-results.json`. Each run is
bounded to 600 frames with scripted Start input and framebuffer readback on
frame 1/every 60th frame. These tests check startup/execution, audio delivery,
flips and teardown, not a full playthrough. Their measured FPS includes
expensive readback and must not be presented as normal gameplay performance.

## Build and repeat

The accepted border/LUT/STMIA frontend cache is required by the isolated build:

```sh
python3 scripts/build_pocketsnes_regions.py
python3 scripts/test_pocketsnes_library_native.py
python3 scripts/test_pocketsnes_regions.py /private/path/game.sfc
python3 tests/amiga/pocketsnes/check_library_host.py .context/amiga/pocketsnes-regions/build
```

The native ROM test deliberately exits nonzero for the strict replay failures
above. A9 tests use the workspace Unicorn environment:

```sh
clang -O2 -I.context/amiga/pocketsnes-regions/build tests/amiga/pocketsnes/library_view.c -o .context/amiga/pocketsnes-regions/build/view-native
.context/amiga/mesen-bench/venv/bin/python tests/amiga/pocketsnes/check_regions_arm.py .context/amiga/pocketsnes-regions/build /private/path/game.sfc
```

Use this matching player/library build for PAL. Legacy cached player binaries
and the older presentation-only build do not contain the region protocol.
ROMs, private save states and commercial-game screenshots are excluded from
versioned evidence.
