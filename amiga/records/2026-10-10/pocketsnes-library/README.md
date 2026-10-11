# ZZSnes MUI library and thumbnail saves — 10 October 2026

Installed separately at `SD032G:Dev/ZZSnesLibrary/ZZSnesLibrary` with Workbench
TOOL icon (65,536-byte stack), `ZZSnes-ui` child player, five content-addressed
slots per ROM and a 128x112 preview embedded in each state. Full controls,
architecture, compatibility and attribution: [library guide](../../../pocketsnes/library/README.md).

## Physical validation

A4000TX / TF4060 / ZZ9000 XX19c, XACP 1.7, OS3.2.3. Native MUI3.8-compatible
interface, 1920x800 8-bit Workbench; gameplay uses 320x240 32-bit ZZ9000 buffers.
No firmware, Roadshow, audio driver or earlier emulator installation changed.

- MUI opens, lists both ROMs and launches the player; returns after Escape.
- SMW: F5 writes 686,196 bytes; F8 restores while paused; fresh-process
  **Resume slot** restores successfully. Thumbnail is visibly the captured title
  frame in the Save slots panel.
- Original 32 KiB test cartridge: separate `bd6be8fa937f7914-1.zzs` save/load
  succeeds, with its own green image. SMW's `ae8466a18aa41dc9-1.zzs` remains.
- Native ASan/UBSan tests reproduce the next 30 frames and stereo PCM exactly
  after save/load. A9 instruction models repeat this at two relocation bases,
  including loading a save in a fresh allocation. Corrupt-thumbnail and
  wrong-ROM files are rejected without advancing the emulated frame.
- Supplemental state fixes omissions in upstream snapshots: fractional sample
  phase, echo/filter/mixer history, InternalPPU and GFX shadow registers. Saved
  runtime pointers are cleared and rebuilt against the current allocation.
- Actual generated stereo AHI worker model passes state-drain/resume, short
  flush, ring wrap, allocation/open failures, cancellation and device errors.
- MMU plan passes all 16,384 alignments for the owned 32 MiB region; common
  ARM/debugger and MCP suites pass. All border/aspect/mask combinations match
  an independent unoptimized pixel converter in both supported 32-bit layouts.

Every recorded hardware session returns RET1, restores all 14 registers and
cache/MMU state, reports witness_bad=0, releases its allocation and leaves the
bridge/Workbench alive. No crash or reboot observed during these checks.

## Border rendering cost

The first procedural renderer recalculated scaling and artwork for each pixel:
about 11.2 ms of ARM presentation per frame in interactive runs. Caching the
geometry and static bezel reduces this to **3.320 ms/frame** in the final
600-frame scripted-Start run. That run reports **52.447 displayed FPS**,
600 flips, zero display faults, all 320,813 stereo sample pairs completed,
zero AHI errors, and 83 late-link events. These are scheduling gaps, not proof
of uninterrupted audible playback.

The earlier plain preview measured 54.933 FPS and 2.272 ms presentation in its
own 600-frame run. The current UI/bezel build adds scaling and border work;
it is not a speed improvement over that preview. Interactive save/load sessions
include pauses and disk I/O and are not FPS benchmarks.

## Limits and evidence

Only standard NTSC LoROM/HiROM (up to 4 MiB), no enhancement chips, 256x224
output. Two tested ROMs do not establish broad compatibility. States are tied
to this version/ABI. SRAM is included in states; standalone battery files are
not written. Current AHI unit 0 remains Paula. Live sound quality and physical
joystick acceptance are still for the user to assess.

`physical-*.log` are target execution evidence. `optimized-model.log`,
`host-tests.log`, `native-final.log`, and `regression.log` are local validation,
not substitutes for physical execution. `library-build.json` records hashes.
Commercial ROMs and state files are deliberately excluded from this record.
Amiga save timestamps reflect its existing clock (2025); host evidence is
recorded in 2026. The clock was not changed.

Final optimized-binary check (`physical-final-save.log`) also passes fresh
resume, overwrite, load and exit0. Directory evidence confirms the prior save
was rotated to `.bak`. `final-check.log` verifies deployed executable hashes,
Workbench-only display, no Core1 owner and bridge response. MUI is left open
on the SMW Save slots panel; the temporary Mac transfer server is stopped.
