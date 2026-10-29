# Changelog

## 0.7.3 Beta — ZZ9000AX audio and register dispatch

- Select ZZ9000AX 16-bit stereo through per-app AHI; use configured unit 0 if AX is unavailable. Global preferences remain unchanged.
- Include the tested limited ARM register dispatch with a 32 MiB executable reserve (96 MiB total runtime allocation).
- Keep normal regional pacing, ROM input, save/load, screenshots and fullscreen/window switching.
- Package matching launcher/player versions and the offline amipkg installer with bundled scanline tools.
- Matched short audio tests measured NES +3.2% / SNES +4.6% FPS; these are not all-title or long-session audio-quality claims.

## 0.7.2-dev — experimental register dispatch and audio

- Add opt-in per-app ZZ9000AX AHI output with unit-0 fallback, immutable buffers and hardware timing/lifecycle evidence; global audio preferences unchanged.
- Use eight generated register handlers through the core's existing opcode tables, resolving the measured lookup-path regression.
- Add isolated AHI/bitmap-call profiling and four-frame batch tests; preserve original audio defaults after matched hardware comparisons.
- Add isolated serial/overlap presentation tests with pinned render locks; hardware checks pass, but no performance gain. Defaults unchanged.
- Add opt-in 250 µs fullscreen ACK polling and phase profiling; roughly 90% fewer register reads, no useful counter-free FPS gain.
- Separate timing from recurring pixel verification; complete 16 fixed-presentation hardware comparisons without a repeatable earlier-audio gain.
- Retain the requested 32 MiB executable reservation and 96 MiB owned runtime.
- Add opt-in batch-ready audio notification; retain original release scheduling.
- [Matched results and limitations](docs/jit-register-experiment.md).

## 0.6.5 Beta — packaged release

- Publish the matched, physically tested 0.6.5-dev launcher and players.
- Include compact layouts, ROM Manager, temperature and fullscreen FPGA/windowed software scanlines.
- Add a complete offline amipkg installer with MUI/Picasso96/AHI/ZZ9000 checks and automatic install-missing scanline tools.
- Include a manual ZIP alternative, checksums and current feature/specification documentation.
- Incorporate the development changes below; no additional FPS gain is claimed by packaging.

## 0.6.5-dev — Additional options

- Automatic cartridge SRAM persistence on clean exit, with verified writes and previous-file backup.
- Per-game display/emulation/control/cheat profiles, separate from global defaults.
- AHI-buffer-driven automatic frameskip and configurable threshold.
- Adjustable 5–95% low-pass strength and transparency toggle.
- Two-player keyboard/gamepad mapping; second physical port remains opt-in.
- Bounded Game Genie / Pro Action Replay entry and enable control.
- Profiles and controller settings included in ROM Manager backups.
- Retains existing state compatibility; see [details and validation](docs/additional-options.md).

## 0.6.4-dev — expanded cartridges

- Enable SA-1, DSP-1, C4 and S-DD1 cartridge families.
- Add 6 MiB S-DD1/ExHiROM loading and correct non-power-of-two ROM mirroring.
- Extend special-chip save states; fix Super FX drawing-buffer cache invalidation.
- Add experimental high-resolution output, frameskip, audio filtering and emulated CPU timing controls.
- Increase the owned ARM allocation to 48 MiB; retain version-2 standard save loading.
- Preserve existing compact navigation, controls and scanline integration.

[Implementation, validation and limits](docs/expanded-cartridges.md).

## 0.6.3-dev — scalable launcher

- Compact layout for smaller Workbench screens, with independently scrollable pages.
- Smaller previews/loading cards, two-column save cards and a compact controller diagram.
- Keep navigation, Launch and settings controls visible during page scrolling.
- Preserve current player binaries, settings format, ROM Manager and scanline integration.

## Development — SNES region and pacing

- Show destination code, region and frame-rate target beside the game preview.
- Cap PAL at 50 fps and NTSC at 59.92 fps in both standard and Super FX players.
- Share output timing with PCM generation; retain 32,040 Hz stereo AHI output.
- Remove the two-frame pacing lead; compensate small timer jitter and rebase long stalls.
- Preserve ROM bytes, existing state format and display/scanline controls.

## Development — ROM Manager and temperature

- Bulk import from a selected directory or mounted SD/USB drive, including subfolders.
- Confirmed duplicate library-entry removal using decoded ROM content; files and saves kept.
- Verified save-state, battery-save, snapshot and library backups into new folders.
- Live read-only ZZ9000 Zynq temperature beside the version number.
- Experimental amipkg LHA/recipe builder with automatic install-missing Xanxi controls.
  Custom repositories require the documented local amipkg 0.7.11 recipe fix.

## Scanline backend development update

- Fullscreen: Xanxi FPGA Classic/Soft/Gradient; window: Soft software filtering.
- F7 cycles fullscreen modes or toggles windowed Off/Soft; no double filtering.
- Hardware effect disabled when entering a window and on normal exit.
- Legacy Thin setting migrates to Soft; fullscreen uses the low-resolution mode.
- Both packages bundle Xanxi controls, source/license and an install-missing helper.
- Existing control tools, firmware and startup files are preserved.


## Unreleased — F7 scanline control

- F7 cycles Off, Soft, Gradient (smooth), Classic (hard) during play or pause.
- Show F7 Scanlines on the loading panel and in the controls guide.
- Retain saved defaults; ignore key-repeat and apply changes after pending flips.


## Unreleased — save-state restoration

- Rebuild derived sprite lists after loading standard-player states, fixing a
  fresh-process resume failure. Existing save files keep the same format.
- Add original-ROM regression and relocated ARM checkpoint replay checks.

## Unreleased — display defaults

- Fullscreen with software scanlines for new libraries.
- Loading panel labels F9 as Screenshot.
- Sharp pixels / Smooth scrolling selector matches ZZNES; horizontal NEON
  interpolation supports NTSC/PAL, borders and all software scanline modes.


## 0.6.2-dev — optimisation candidate (not a tagged release)

- Remove per-pixel region tests from ARM presentation by processing contiguous spans.
- Add a measured `-O2` core build, retaining soft float and existing numerical rules.
- Preserve Thin scanlines and the separate Super FX player.
- Faster large-library sorting, metadata writing and token parsing.
- Add native, relocated ARM and physical validation; see [measurements](docs/optimisation-20261018.md).

## Unreleased launcher navigation

- Label the existing effect Software scanlines; explain that ZZScanlines is not required.

- Add a red Launch button to the permanent bottom bar, visible on every page.
- Replace automatic mounted-drive discovery with Disks / Browse / Scan once.
- Keep Continue and save-state actions separate from starting a game.

## Unreleased display and FX3 candidates

- Add Thin scanlines at 640×480: retain every image row and insert one-pixel
  black separators, improving small-text detail compared with Classic at 320×240.
- Retain the existing modes; support Thin in windows and fullscreen, with FPS
  overlay, switching and clean library snapshots.
- Implement experimental FX3 register/memory mapping, MERGE framebuffer clears,
  STOP behaviour and bounded scheduling in PocketSNES, informed by MesenCE.
- Add original native/A9 instruction fixtures and ordinary-cartridge regressions.
  Commercial FX3 compatibility and complete special-chip states remain unverified.

## Unreleased launcher changes

- Allow Super FX games, including Star Fox 2 and Doom, to launch with Play.
- Route these cartridges to the timing-corrected experimental player.
- FX3 warnings are advisory; Play remains enabled, including for ZIP cartridges.
- FX3 support remains experimental; successful gameplay is not promised.

## Unreleased compatibility research

- Fix zero-cycle Super FX ROM-bank timings in the candidate. Star Fox 2 now
  completes sound startup and executes Super FX; Star Fox also produces video.
- Add an original mapper/CPU timing regression, with a failing pre-fix control.

- Candidate: rebuild derived sprite lists after state restore; resolves the
  frame-600 Imperium and Stunt Race FX video replay failures without a format change.
- Add longer, fresh-process replay diagnostics and isolated A9 chip builds.
- Record remaining Super FX/SA-1, raster and audio failures; normal cartridge
  support and the published 0.6.1 Beta are unchanged.

## 0.6.1 Beta

- Add EmuELEC side-panel theme to both display modes.
- Keep viewport geometry, overscan and scanlines independent.
- Compact 16-colour artwork table; existing memory layout retained.
- Cartridge support and emulation core unchanged.

## 0.6.0 Beta

- Load single-cartridge ZIPs directly in the library and player.
- Discover ZIP cartridges through Add, Folder and mounted-drive scanning.
- Stored/deflate support, bounded decompression and CRC checks before ARM use.
- Preserve member filenames for NES PAL detection; no temporary ROM files.
- Explicit whole-archive deletion confirmation.
- Existing ARM core, cartridge limits and save formats retained.

## 0.5.3 Beta

- Remove launcher title-bar text; keep version in the footer and About.

## 0.5.2 Beta

- New colour logo in the sidebar, startup and loading screens.
- Matching palette-mapped Workbench icon with transparency.
- Existing icon position and launch settings preserved when updating.

## 0.5.1 Beta

- Remove the 128-ROM cap; library and display storage grow with available memory.
- Hash-based duplicate detection and faster sorting for large collections.
- Live search with a short typing delay on MUI.
- Graceful allocation failure; retain discovered entries and existing settings.
- Existing preferences, favourites, history and save paths remain compatible.

## 0.5.0 Beta

- Sidebar MUI launcher, original branding and Workbench icon.
- Startup/loading panels; loading ends after the first completed frame.
- Fullscreen/window switching, optional FPS and three software scanline styles.
- Guarded NEON row stores and fused byte dimming; scalar fallback and comparison.
- Add, rename and confirmed deletion; incremental mounted-volume ROM discovery.
- Existing cadence, AHI batching, mapper/region handling and save formats retained.

## 0.4.0 Beta

- Regional cadence and accepted four-slot C0 fullscreen presentation.
- PAL/NTSC, standard cartridge compatibility, SRAM and screenshot save slots.
- Interactive MUI library and About page.

## 0.2.0 development

- PocketSNES MUI library, thumbnail save states and fullscreen integration.

Historical build records retain the detailed intermediate experiments.
