ZZNes - silent Mesen CPU/PPU development preview

Hardware: ZZ9000 XX19c / XACP 1.7; tested A4000TX.
Close other ZZ9000 ARM applications before starting.

Open a Shell:
  Execute SD032G:Dev/ZZNesPreview/Start-ZZNes

Includes an original NES demo: blue patterned background, orange sprite.
Arrow keys move. Z=A, X=B, Enter=Start, left Shift=Select for supported ROMs.
Q, Escape or the window close gadget exits. Sessions end after five minutes.
Click the NES window if another application has keyboard focus.

This is a development preview, not a complete or full-speed NES emulator.
No sound; no APU/audio interrupt emulation. Only NTSC iNES NROM cartridges
with 16/32 KiB PRG and 0/8 KiB CHR. No battery saves, trainers or other mappers.
No commercial game files are included. Full game compatibility is untested.

Logs: SD032G:Dev/ZZNes-Diagnostic/preview.log
Do not use bridge screenshot capture during this preview: an earlier
self-restart during a display/capture sequence remains unexplained.

Mesen by Sour: https://github.com/SourMesen/Mesen
Pinned source: e1180957b31979d4b82d613af9263fcb9442a685
License: GPL-3.0-or-later; see COPYING.
XACP by XANI: https://github.com/Xanxi-Amiga/XACP-ZZ9000
Port/integration source and build instructions: amiga/mesen_nes/ in the
local development workspace. This preview has not been published as a release.
