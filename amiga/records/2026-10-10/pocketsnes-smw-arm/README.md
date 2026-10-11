# Super Mario World — physical ZZ9000 A9 test, 10 October 2026

The separate PocketSNES/Snes9x2002 portable-C core passed 600 startup/menu
frames on physical Cortex-A9 Core1. Nine measured 60-frame batches after the
first 60-frame startup batch averaged **5.01744 ms/frame**, equivalent to
**199.30 core frames/second**. Batch means ranged **3.92164–5.88238 ms/frame**;
these are batch averages, not minimum/maximum individual-frame latency.

The 60 Hz budget is 16.667 ms/frame, leaving 11.649 ms at this mean for work
outside the measured core. This is encouraging headroom, not a demonstrated
60 FPS player or compatibility claim. No fullscreen display, framebuffer
transfer, AHI playback or sustained directional gameplay is measured.

## Method and correctness

User-supplied 512 KiB Super Mario World (USA), SHA256
`0838e531fe22c077528febe14cb3ff7c492f1f5fa8de354192bdff7137c27f5b`.
The ROM is local private input, excluded from source and executable images.
Scripted Start input at zero-based frames 360–367 and 480–487, no movement.
Continuous emulated state across batches; first batch excluded as startup.
This measures different startup/menu scenes, not repeated identical work.

Every 60 frames, RGB565 video, cumulative PCM hash, stereo sample count and
nonzero sample count matched the Mac ASan/UBSan reference. Final video
`edf1acde`, PCM `a2053b75`, 320,813 stereo pairs and 327,934 nonzero interleaved
sample values. Audio is generated and validated, not played to speakers.

Each timed interval covers `retro_run()`: CPU, PPU, SPC/DSP and audio callback
checksum. Video checksum, ROM loading, initialization/deinitialization, 68k
polling, mailbox waits and logging are outside it. No frameskip, idle shutdown,
game speed patches, VFP or NEON. Timer calibrated to 333,332,807 Hz.

Portable C sources remain pinned to libretro/snes9x2002
`6ffbf9ef4f0063e1f1b78a40d10c50fc52f2524c`. The isolated BRR signed-shift fix
from the native game test is present. Both relocated A9-model runs matched
all ten native checkpoints; cancellation passed. Existing checkerboard
benchmark and Mesen/NES apps are unchanged.

## Deployment and lifecycle

Installed `SD032G:Dev/ZZSnes-Pocket/ZZSnes-Pocket-GameBench`, 511,712 bytes,
MD5 `a14f3df6e1f55b1647a0d4fe70ce4848`, with `game-build.json` and the separate
private `SMW-Test.sfc`. The exact ROM size/FNV is checked before worker launch.
Use `Stack 65536`; the diagnostic writes text, not a game window.

The established 24 MiB Exec-owned allocation, runtime mapping probe, private
cache policy and single-Core1 ownership are retained. ROM occupies offsets
0x190000–0x210000, between mailbox end 0x184000 and BSS start 0x220000.
Host copies it before the full allocation cache sync and ARM startup; it is
immutable afterward. No static hardware address is reserved. MMU layout checks
passed all 16,384 allocation alignments.

Physical active cancellation took 133,986 us. RET1, all 14 restored registers,
MMU/cache witness and owned memory release passed. Workbench was the only
screen afterward, with no ARM owner; Roadie/bridge remained present. No firmware,
AHI or startup settings changed. Temporary HTTP transfer server stopped.

Raw physical measurements: `benchmark.log`; computed arithmetic: `results.json`.
Native/model/MMU/common ARM and MCP checks are retained alongside deployment
checksums and build metadata. Next: fullscreen display and audio queue/pacing,
then a longer joystick-controlled gameplay run.
