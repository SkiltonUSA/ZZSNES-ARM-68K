# Compatibility candidate evidence

Isolated candidate based on 0.6.1 Beta. Normal launcher/player and release are unchanged.
The experimental cartridge gate accepts paired LoROM Super FX and SA-1 header types.

- Native ASan/UBSan: eleven ROMs at 600 and 2,400 frames; 30 further video/PCM
  hashes checked after both same-process and fresh-process restore. Status 3
  is a replay mismatch, not a successful compatibility result. Black frames can
  replay identically; consult the report before interpreting status 0.
- A9 instruction model: fixture save/replay at two relocated addresses, corrupted
  state rejection; presenter pixels, scanlines, SIMD state and cancellation passed.
- Physical A4000TX: six special-chip cartridges and Imperium completed 600 frames
  each, with normal cache/register witnesses and owned-memory release. Workbench
  window, FPS enabled, scanlines off. No scripted game input in these runs.
- Imperium physical pause/save/load/snapshot/exit passed. This is lifecycle proof;
  exact frame/audio equality was tested natively, not measured on the physical run.

`build.json` identifies the tested binary. Native timing is not A9 performance.
`ZZSNES_POCKET END` is a bounded startup run including pacing; it is not a
sustained gameplay or displayed-frame-rate rating. The native tests press Start
at frames 360–367, so their paths differ from the physical startup checks.

[Findings and remaining failures](../../../../docs/compatibility-development.md).
