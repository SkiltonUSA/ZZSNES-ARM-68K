# PocketSNES fullscreen/stereo preview — 10 October 2026

The physical A4000TX now runs a separate **ZZSnes-Pocket** fullscreen preview
using the validated Snes9x2002/PocketSNES core on ZZ9000 Cortex-A9 Core1.
It accepts only the supplied 512 KiB Super Mario World USA cartridge. No ROM
bytes are embedded in its executable or source records.

| Same 600-frame scripted startup/menu sequence | Displayed FPS | Readback checks |
|---|---:|---:|
| Full validation | 45.92 | 11 |
| Normal presentation | **54.93** | 1 |

Both runs displayed all 600 frames, matched native video checkpoint hashes and
submitted/completed all **320,813 stereo sample pairs** through AHI without a
device error. The normal run took 10.922301 seconds, including its initial
readback and final audio drain. Mean A9 core work was 6.178 ms/frame; framebuffer
conversion/stores 2.272 ms/frame. Core time now includes copying to the padded
picture and its checksum, unlike the previous 5.017 ms headless benchmark.
No frames are skipped. Input is Start at frames360–367/480–487, no directions;
this is startup/menu performance, not sustained interactive gameplay.

**60 FPS and smooth audio are not established.** The normal run recorded 56
late-link events: the previous AHI request had completed before the next was
submitted. All samples completed, but there may be audible gaps. The counter
is not an underrun-duration measurement. AHI unit0 selects **Paula** on this
machine; ZZ9000AX and the saved Music Unit configuration are unchanged.
User listening and physical joystick acceptance remain pending.

## Presentation and controls

The 68k owns the screen, joystick/keyboard polling, AHI DOS process, pacing and
firmware handoff. ARM runs CPU/PPU/SPC/DSP, copies 256x224 RGB565 into a 256x240
picture with 8-pixel black top/bottom borders, then converts completed frames
directly into a locked, OS-owned P96 bitmap. There are 32-pixel side borders
inside the 320x240 screen. No per-frame picture transfer back over Zorro is
required in normal presentation; the first-frame diagnostic does read back.
PCM still crosses to the 68k-owned AHI queue.

Uses the established independent XACP XX19c deferred-PAN triple-buffer
implementation, guided by **XANI's XACP/PicoDrive example**:
https://github.com/Xanxi-Amiga/XACP-ZZ9000 . No PicoDrive source was copied.
SDL is not involved. No global graphics, firmware, AHI or startup change.

Arrows/joystick port2 move; Z/fire=B, X/second button=Y; A=A, S=X, C/V=L/R;
Return=Start, left Shift=Select. P pauses, M mutes, Q/Escape quits. Mouse click
or loss of active fullscreen window exits. Joystick polling was verified but
no physical movement/fire was provided in these automated runs.

This preview has a Workbench tool icon and ROM file requester, plus Start-SMW
Shell script. Cartridge SRAM persistence and save states are disabled; it is
not yet the full MUI library or a general cartridge-compatibility release.

## Validation and lifecycle

Native ASan/UBSan and two relocated A9 instruction models each reproduce all
600 reference video and stereo-PCM checkpoints. Tests include invalid-ROM
rejection, cancellation before frame advance, all 65,536 RGB565 values in both
32-bit display formats, row/buffer guards, invalid framebuffer challenges,
cancelled presentation and temporary mapping revocation. The actual generated
stereo AHI worker passes ordered-copy, wrapping, partial flush, failure and
cleanup models. MMU tests cover all16,384 owned allocation alignments and the
new disjoint ROM/video/audio transfer ranges. Common ARM/MCP regressions pass.

Physical short10, diagnostic600 and normal600 all return with RET1, all14
registers restored, MMU/cache witness correct and owned RAM released. Workbench
only afterward, no Core1 owner; Roadie/bridge remain present. No crash/reboot
observed. The short Shell command was reported STILL RUNNING while Type emitted
a large log; the complete file independently confirms successful app teardown.

Final installed binary `SD032G:Dev/ZZSnes-Pocket/ZZSnes-Pocket`, MD5
`8f44a8e4228e2bbfd29e104dcfa6bf6e`. Matching icon, Start-SMW, ReadMe-Player.txt
and player-build.json installed. Earlier full-check binary MD5
`11d450a373725faa932b80b26f9ef216`; final change adds INPUTFAST CLI mode to
suppress periodic readback with identical scripted input and identical ARM ELF.
`build.json` records the exact build; a later host-only build precondition guard
affects the build script only. No GitHub upload performed this turn.

Run `python3 scripts/build_pocketsnes_player.py`, then
`python3 tests/amiga/pocketsnes/check_player_host.py` and the Unicorn-enabled
`tests/amiga/pocketsnes/check_player_arm.py`. Builds are isolated under
`.context/amiga/pocketsnes-player`; headless diagnostics and Mesen/NES apps stay
separate. Source and output identities are retained in build.json.

Next: user picture/sound/controller check, remove avoidable normal-frame
checksum/conversion overhead and improve AHI/presentation scheduling; then
longer gameplay validation, SRAM round-trip tests and library integration.
