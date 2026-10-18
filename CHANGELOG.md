# Changelog

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
