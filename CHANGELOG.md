# Changelog

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
