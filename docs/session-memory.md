# ZZSNES development handoff

## Separate PocketSNES core test — 10 October 2026 (latest)

Installed `SD032G:Dev/ZZSnes-Pocket/ZZSnes-Pocket-Bench`, 542,084 bytes,
MD5 `78a67d1434e85d6b304916333514e262`, plus README/build metadata and
`benchmark.log`. Standalone CLI: `Stack 65536`, then run it with output
redirected. No Workbench button or game loading added. Mesen System/MUI and
both NES library/player binaries retain their prior verified MD5s.

Actual tested core is libretro/snes9x2002 (formerly PocketSNES), pinned
`6ffbf9ef4f0063e1f1b78a40d10c50fc52f2524c`, portable C CPU/PPU/SPC/DSP path.
This is not a DrPocketSNES assembly-CPU measurement. Earlier ARMSNES/DrPocket
assembly paths remain untested. No frame skipping, idle shutdown, game speed
patches, audio mute, VFP or NEON. Snes9x noncommercial license retained;
no emulator code copied into Mesen. See `amiga/pocketsnes/README.md`.

Same original 32 KiB checkerboard, six startup frames per sample. Ten measured
batches after one warm-up: **4.6166 ms/frame**, range 4.6047–4.6297 ms.
Previous Mesen scalar-word baseline 39.945 ms: about 8.65x fixture throughput.
Cross-core caveats: PocketSNES returns cropped 256x224 RGB565 and 3,208 stereo
samples at 32,040 Hz; Mesen returns 256x239 SNES-colour pixels and 3,119 samples.
Startup frame boundaries differ. Silent headless fixture only, not game FPS,
real sound output or a 60 Hz compatibility claim. Video validation is outside
the timed interval; PCM hashing is inside the callback.

Pixel oracle and signatures match native ASan/UBSan reference: video67711dc5,
PCM77d82d05; SPC700 AA/BB boot-ROM handshake verified. ARM model passes two
relocation bases, palette-corruption rejection and cancellation. Initial Thumb
model experiment failed and was not deployed; verified executable is ARM mode.
Physical run passed active cancellation (134 ms), RET1, all14 registers,
MMU/cache restoration, witness and owned-memory release. No crash/reboot.
Common ARM and MCP smoke suites passed. Evidence:
`amiga/records/2026-10-10/pocketsnes/`.

Uses 24 MiB Exec-owned runtime-probed ZZ9000 allocation, including bounded
18 MiB heap. Generated MMU copies change image/table/mailbox offsets only;
private inner WB/WA, outer/mailbox NC, data XN and single-Core1 rules persist.
No observed address becomes a static allocation. Layout passes all16,384
64-byte alignments. Final Workbench only, no ARM owner; Roadie, bridge,
ZZTemperature, NES/Mesen MUI still live. No firmware/AHI/startup changes.
Temporary HTTP55031 stopped after transfer; GitHub backup is this standalone ZZSNES-ARM-68K repository.
