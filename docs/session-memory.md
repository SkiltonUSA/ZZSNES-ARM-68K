# Compatibility candidate — isolated, not a release

`development/compatibility-20261016/` adds explicit sprite-cache reconstruction
on restore. The two frame-600 replay failures (Imperium/Stunt Race FX) now pass
same/fresh-process checks. Longer replays still fail for multiple games; blank
Star Fox/Star Fox 2 checkpoints are not compatibility passes. Star Fox 2 is
waiting on sound ports before Super FX begins in the native diagnostic.

Candidate player tested in a separate development drawer: seven 600-frame
physical runs and Imperium paused save/load/snapshot/exit, clean ownership,
register/cache witnesses and teardown. Native sanitizers, two-base ARM state
model and presenter regressions passed within their stated scope. Main 0.6.1
installation/payload unchanged. See compatibility-development.md and logs.
Use explicit `build_current.py --arm --experimental-chips` for CLI chip tests;
`--compatibility` retains the normal gate. Neither flag changes the MUI gate.
Normal make current/current-arm still builds the release. No firmware changes.

---

# ZZSNES 0.6.1 Beta — EmuELEC border

Matching rebuilt launcher/player installed with checksums. Original files and
current settings retained in Rollback-border-1016. Preferences, ROMs, saves and
icon metadata retained. EmuELEC occupies existing theme ID 7, with a 9,632-byte
packed palette table. Allocation bounds and standard cartridge gate unchanged.

Native theme/geometry tests and relocated ARM presenter regression passed.
Physical ZIP session, pause/save/load/snapshot and display switching passed.
NES window focus needed a click before injected F10; exit then completed cleanly.
Current source/payload overlay: development/borders-20261016/.

Eleven supplied cartridge ZIPs were added to the local library. Five standard
cartridges passed bounded 120-frame physical checks. Native-only experiments
with a relaxed gate reached 600 frames for all eleven; three video replay
mismatches remain. Super FX and SA-1 are still rejected by the installed player.
See cartridge-survey.md; no special-chip hardware support is claimed.

---

# ZZSNES 0.6.0 Beta — direct ZIP loading

Installed the new launcher/player pair with checksum checks. Existing Library.cfg,
Library.bak, ROMs, saves and icon metadata were retained. Previous executables
and settings are in the application drawer's Rollback-zip-1016 directory.
Only the 68k launcher/player changed; ARM payload bytes are unchanged.

Plain/ZIP bounded runs, invalid-CRC rejection, paused save/load/snapshot and MUI
launch/return passed on A4000TX. Native ZIP, large-library, scanner and relocated
presenter checks passed. See zip-validation.md for evidence and limitations.
Current launcher/host sources: development/zip-20261016/. Use make current and
make test-zip. Expanded SNES hardware remains a separate task, pending cartridge.

---

# Release 0.5.3 Beta

Packaged the blank-title-bar update with the colour logo. Standalone rebuild
passed. Exact title-bar behavior was checked on hardware before this version
bump; the new package itself has not been redeployed. The installed launcher
still reports 0.5.2 Beta with the same title-bar change. ARM/player unchanged.

---

# Title-bar cleanup — 16 October 2026

Current launcher source removes the window title text. Version remains in the
footer and About. The published logo beta ZIP remains unchanged; this is a
subsequent local/source update. Previous installed launcher and settings are
in Rollback-title-1016. Player, icons and libraries are unchanged.
Both builds passed; the physical windows have blank title bars and close cleanly.

---

# Colour logo update: ZZSNES 0.5.2 Beta

Current launcher sources/artwork: development/logo-20261016/. New supplied
colour logo in the sidebar, startup/loading panels and Workbench icon.
Physical launcher appearance checked; icon.library colour-image round trip
and existing icon launch metadata checks passed. Standalone make current and
make verify-current passed. Player and ARM payload unchanged.

Installed with launcher/MakeIcon checksum checks and unchanged Library.cfg.
Previous files and icon retained in Rollback-logo-1016. Initial installed
library opening was slow; do not infer startup performance from this visual
update. The isolated empty-library launchers opened normally.

---

# Large-library update: ZZSNES 0.5.1 Beta

Launcher-only update: dynamic library storage, path indexes and sorting.
No fixed ROM-count cap. Player binary/ARM payload unchanged. Source and prepared
launcher inputs: `development/library-20261016/`. `make test-library-large`
validates large collections and failure recovery. Physical 20,000-entry MUI
load/save and live-search checks passed; see the 16 October records.
Installed under the existing Emulator drawer with executable checksum checks.
Previous launcher and preferences retained in `Rollback-1016`; existing library
bytes were unchanged by installation. No player, ROM or save files replaced.

---

# Current beta: ZZSNES 0.5.0

Current editable sources and exact build inputs: `development/beta-20261015/`.
Artifacts: `artifacts/beta-0.5.0/`. `make current-arm` rebuilds the complete
pair; `make test-current` runs native/model checks. Physical results are in
`amiga/records/2026-10-15/refresh/`. See `current-development.md` for the selected
optimisations and rejected alternatives. Native/model tests are not hardware proof.

The shared sidebar, mounted-drive discovery, asynchronous startup/loading panels,
window/fullscreen switching, FPS and software scanlines are enabled. Preserve
existing ROM paths and library/save files when upgrading. Only one Core1 owner;
retain runtime mapping, cache witnesses and reset-before-free on every path.

Installed and checksum-verified on the A4000TX. Previous executables, icon and
library preferences are retained in `Rollback-1015` under the application drawer.
The installer preserved the existing library files; no original ROM/state files
were changed. Test ROM copies were used in a separate development drawer.

---

# Current Beta handoff — 12 October 2026

Current: ZZSNES 0.4.0 Beta, `SD032G:Emulator/ZZSNES/ZZSNES`;
player `ZZSnes-ui`. Exact installed binaries in `artifacts/beta-0.4.0/`.
Current sources/build inputs in `development/beta-20261012/`. This supersedes
the installation and performance statements in the historical entries below.

See the main README for steady versus whole-loop frame rates, picture omission
and audio scheduling results. The installed Beta includes accepted cadence,
PAL handling, current MUI tabs, simplified About credits and per-game saves.
Both libraries were visually checked and closed cleanly after migration.
Old Dev folders are recovery copies; final observed machine state was Workbench,
with no library/player/Core1 owner. During backup review, three NES ROM filenames were corrected to retain `(E)`
PAL tags; current settings and ROM bytes were preserved. No emulator was
launched and no new FPS measurement was performed. See the PAL path-fix record.

[Build/backup scope](beta-backup.md) and
[installed validation](../amiga/records/2026-10-12/emulator-beta/).

---

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
