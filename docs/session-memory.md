# ZZSNES development handoff

## Latest: MUI library and thumbnail saves


New independent Workbench app: `SD032G:Dev/ZZSnesLibrary/ZZSnesLibrary`
(91,040 bytes, MD5 89358045e6941538fbdc601e5546291e), TOOL icon stack65536.
Player `ZZSnes-ui` (642,752 bytes, MD5 4fc02f99b9fcc8808c3696090f39fbca),
PocketSNES/Snes9x2002 core; previous Mesen/NES/SMW-preview apps unchanged.
MUI tabs Library/Display/Controllers/Save slots, favourites/recent games,
ROM/folder selection, 4:3/16:9 monitor, black/white or original SNES purple,
Super Famicom and charcoal console bezels, optional8/16px edge masks.

F5 saves selected slot; F8 restores (including paused); Resume slot starts a
new core from disk. Five slots per ROM, two content hashes normalized after a
512-byte copier header; embedded128x112 RGB565 thumbnail, verified temp write,
previous .bak rotation. Version2 state686196 bytes on A9 includes SRAM, PPU,
SPC/DSP, fractional audio phase, mixer/echo/filter history and current picture.
No standalone battery .srm persistence. Conservative standard NTSC LoROM/HiROM
<=4MiB, types0–2 only; no enhancement chips, PAL or high-resolution output.

Physical SMW save/load/pausedload/fresh-processresume and independent original
32KiB fixture saves pass; both thumbnails inspected in MUI. Native sanitizers
and relocatedA9models reproduce30frames+stereoPCM exactly, including newbase;
corruptedthumbnail/wrongROM rejected. Runtime pointers rebuilt afterload.
32MiB Exec-owned layout: tables100000..11c000, witness120000, video130000,
audio170000, mailbox180000, ROM200000..600000, states600000..700000,
BSS700000 onwards, final192KiB stack. These are allocation offsets, NEVER
static board addresses. Existing runtime mapping, Core1 guard, cache/MMU
teardown retained;16384alignmentmodel and stereoAHIstate-drain model pass.

Final cached-bezel600frame scriptedStart hardware test52.447FPS, meanARM
presentation3.320ms,600flips/zero faults,320813PCM pairs completed/error0.
AHIlate-link83 remains; no claim of flawless audio or sustained60FPS.
Earlier plainpreview54.933FPS/2.272ms; newscale/bezel adds work. All hardware
runsRET1/registermismatch0/witness0 and releaseownedRAM. Source/buildguide
`amiga/pocketsnes/library/README.md`; builder`scripts/build_pocketsnes_library.py`.
Evidence `amiga/records/2026-10-10/pocketsnes-library/`. Do not commit ROMs or
save-state files. New app's owner portZZSNES_LIBRARY; child uses existing single
Core1 guard. Use MCP HTTP only; never a second raw bridge connection.
Final optimized binary also passes fresh-processresume, F5overwrite (previous
686196-byte .bak present), F8reload and exit0. UI left open on SMW Save slots;
Workbench only, no Core1 owner, Roadie/bridge alive. Documentation and build
manifest installed in drawer. Local deployment HTTP55031 stopped after checks.

## Earlier milestones
# ZZSNES development handoff

## Separate PocketSNES core test — 10 October 2026 (historical)

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

## First real-cartridge reference (native only)

Super Mario World, SHA256 0838e531fe22c077528febe14cb3ff7c492f1f5fa8de354192bdff7137c27f5b, passed 600 startup/menu frames with active PCM after the test-only BRR signed-shift patch. No new ARM build or hardware run. See amiga/records/2026-10-10/pocketsnes-smw-native/. Cartridge bytes are local-only.
