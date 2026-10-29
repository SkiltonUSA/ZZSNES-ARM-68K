# Experimental timed Super FX

**Legacy remains the default.** The isolated 0.7.5-exp build adds
Display → Emulation → Super FX → **Timed (experimental)**, saved globally or
per game. The published 0.7.4 Beta is unchanged.

Timed mode divides Super FX work into scanline budgets, accounting for memory,
cache fetches and instruction costs. It may change game speed and compatibility;
it is not an ARM overclock or a promised FPS improvement. FX3 keeps Legacy timing.
Loading a save restores the timing mode recorded in that save. Older players
cannot load Timed saves.

[Build instructions, design and upstream acknowledgement](../development/timed-superfx-20261029/README.md).

## Validation

Native ASan/UBSan tests pass:

- Legacy default, clock select, overshoot carry, cache hit/fill/flush.
- RAM access, multiply, ROM-buffer and plot costs; STOP/IRQ behavior.
- GO/R15 writes do not grant extra scanline budgets; FX3 retains old budgeting.
- Settings persistence, timing/debt state restore and invalid-state rejection.
- 3,600 video/stereo-PCM frames each for Stunt Race FX, Super Mario World,
  Star Fox 2 and Doom: candidate Legacy matches the previous core exactly.
- Each game also completes 3,600 Timed frames and a 60-frame save/load replay.
  Timed Super Mario World output matches Legacy, as it has no Super FX chip.
- `make test-native test-expanded` passes, including the existing cartridge,
  state, colour, options and battery regressions.

Relocated Cortex-A9 model checks pass for Stunt Race FX, Star Fox 2 and
Super Mario World at two load addresses: 30-frame Legacy video/PCM equivalence,
Legacy candidate saves resumed by the previous player, and Timed state/PCM
replay in a fresh instance. The first Timed frame also matches the physical A9
for all three games.

Physical A4000TX checks (XX19c / XACP 1.7):

| Game | Mode | Completed core frames | Result |
|---|---|---:|---|
| Stunt Race FX | Legacy | 660 | Clean completion; sampled hashes match the prior baseline |
| Stunt Race FX | Timed | 660 | Clean completion and timed-state resume |
| Star Fox 2 | Timed | 660 | Clean completion and timed-state resume |
| Super Mario World | Timed selected; no GSU | 660 | Clean completion; non-Super-FX control |

All four runs retire 352,913 stereo PCM frames with no AHI error, no pending
flip or flip fault, and successful register/MMU/cache-witness restoration and
owned-memory release. A real MUI 640×480-budget probe passes all seven pages,
resize checks and per-game Timed option save/reload/reset; Legacy is the default.
No physical screen-mode preference was changed.

These are bounded execution/state/lifecycle checks, **not visual acceptance,
listening tests or a demonstrated FPS gain**. Timed and Legacy game trajectories
and resumed checkpoints differ, so their elapsed times are not a matched-work
performance comparison. The audio backend still queues scheduled silence.
Long gameplay, bus-arbitration accuracy and FX3 timing remain future work.

The temporary Amiga test drawer was removed after testing. Installed launcher
and player remain 0.7.4-dev. Firmware, startup, ROM library, saves and global
preferences were not modified.

[Raw hardware logs, model/native results and hashes](../amiga/records/2026-10-29/timed-superfx/).
