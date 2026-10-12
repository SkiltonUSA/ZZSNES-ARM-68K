ZZNes fullscreen development preview - 9 October 2026

Requires the verified A4000TX setup: ZZ9000 XX19c / XACP 1.7,
matching zz9000.card, Picasso96, and an existing 320x240 32-bit mode.
Close all other ZZ9000 Core1 applications before starting.

From an Amiga Shell:
  Execute SD032G:Dev/ZZNesFullscreen/Start-ZZNesFullscreen

The original built-in NES demo has a patterned background and movable sprite.
Arrows move; Z=A, X=B, Return=Start, left Shift=Select.
Q, Escape, a mouse click or window deactivation closes the fullscreen app.
There is also a five-minute session limit. Sound is not implemented.

ARM converts completed frames into three slots in the P96 screen bitmap.
The host uses the deferred-PAN/ACK interface demonstrated by XANI's
ZZPicoDriveMD. The next CPU/PPU frame can overlap the vertical-blank wait.
No per-frame window blit. 256x240 NES pixels are centred in 320x240 with
32-pixel black side borders; there is no scaling.

120-frame physical test: 28.88 fps, 120 acknowledged flips, correct sampled
pixels and clean exit. This is a silent original demo, not full-speed NES
emulation or a general game compatibility benchmark. Full NES pixel validation
runs on frame 1 and every 60th frame and is included in measured time.

Only NTSC iNES NROM is supported (16/32 KiB PRG, 0/8 KiB CHR).
No APU/audio timing, PAL, other mappers, saves or colour-emphasis display.
No commercial ROMs are included.

CPU/PPU: Mesen, GPL-3.0-or-later. See COPYING.
Fullscreen handoff guidance: Xanxi / XANI, XACP-ZZ9000 / ZZPicoDriveMD.
Our implementation is independent; no PicoDrive code is included.

This is a separate development preview. The accepted windowed preview in
SD032G:Dev/ZZNesPreview remains unchanged. No firmware or startup changes.
Persistent diagnostic log:
  SD032G:Dev/ZZNes-Diagnostic/fullscreen-preview.log
