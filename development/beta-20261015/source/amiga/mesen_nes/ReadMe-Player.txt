ZZNes 0.3.2-dev - Mesen on ZZ9000 Core1
Accuracy-first development player, not a full Mesen release.

Requires AmigaOS 3.x, ZZ9000 Zorro III with its Fast RAM enabled,
XX19c/XACP 1.7 firmware and an existing 320x240 32-bit Picasso96 mode.
Close all other ZZ9000 ARM applications first. No firmware changes needed.
Reserves 4 MiB of Exec-owned ZZ9000 memory, plus host/display/audio storage.

Execute Start-ZZNesPlayer
Choose your own uncompressed NTSC .nes file in the file requester.
Cancel returns to Workbench. To start directly in a Shell:
  Stack 65536
  ZZNes "path:Game.nes"
Diagnostic (fresh nonzero nonce, optional frame limit):
  ZZNes 193ad240 RUN DEMO 120
  ZZNes 193ad241 RUN ZZNesSoundTest.nes 120
DEMO is the original silent pattern/sprite demo. ZZNesSoundTest.nes
is our original quiet square tone + sprite demo, with a saved frame counter.
No commercial games are supplied.

Arrows move; Z=A; X=B; Return=Start; left Shift=Select.
P pauses/resumes; M mutes/unmutes output (APU timing stays active).
Q/Esc, mouse click, Ctrl-C or leaving the screen quits.
Battery-backed cartridges save to Game.nes.sav beside the ROM on a clean
exit. The preceding save is retained as .sav.bak. Do not reset/power off
to quit. Read-only filesystems or bad save sizes stop save/load safely;
consult the log and .sav.tmp/.sav.bak if a write failed.

Supported envelope: clean iNES NTSC mapper 0 (NROM), 1 (MMC1), 2 (UxROM),
3 (CNROM), 4 (MMC3B/C), 7 (AxROM), 90 (JY Company); power-of-two PRG/CHR sizes, up to
512 KiB PRG + 256 KiB CHR. Mapper-specific limits apply. MMC1 up to
32 KiB RAM; other original mappers 8 KiB. CHR RAM 8 KiB. Four-screen mirroring.
Mapper 90 requires 32-512 KiB PRG + 8-256 KiB CHR ROM, header RAM byte 0,
no battery or four-screen flag. Related mapper 209/211 are not enabled.
NES 2.0, trainers, PAL/Dendy, VS, MMC6, board-database overrides, other
mappers, expansion audio, save states, cheats, joystick/gamepad and second
controller are not implemented. Unsupported headers are rejected.

Mesen CPU, PPU, five-channel APU, DMC DMA and mapper timing retained.
Band-limited 32000 Hz mono audio uses blip_buf and the Mesen nonlinear DAC.
AHI unit 0 uses your current AHI preferences; no global settings changed.
If AHI cannot open, emulation runs muted. Pausing lets queued sound drain.
The player does not skip emulated frames or alter CPU/PPU/APU clock ratios.
Below real-time speed, sound may have gaps; full-speed NTSC is not promised.
Full 9-bit palette/emphasis is converted on ARM into the owned RTG bitmap.

Log: SD032G:Dev/ZZNes-Diagnostic/player.log
The earlier accepted fullscreen preview remains in Dev/ZZNesFullscreen.

Credits: Sour/Mesen (GPL-3.0-or-later); Shay Green/blip_buf
(LGPL-2.1-or-later); XANI's XACP and deferred-PAN interface/design guidance.
Port source: amiga/mesen_nes; deterministic adapters in scripts/.
COPYING and COPYING.LGPL describe redistribution requirements.

Mapper 90 update: original Mesen JyCompany implementation; CPU/PPU IRQ
callbacks, multiplier/register reads at $5000-$5fff, banked PRG at $6000.
Mapper 90 predecessor backup: ZZNes.before-mapper90.
Previous working 0.3.1-dev player: ZZNes.before-optimization.
Super Mario World launcher: Execute Start-SuperMarioWorld

0.3.2 optimization: sparse timestamp audio mixing preserves the same PCM.
The next ARM frame overlaps pending display flips and AHI submission from
host-owned buffers. No core frame skipping, clock changes or firmware changes.
Paula/AHI unit 0 remains selected by the app; ZZ9000AX direct output is not yet
implemented. Playback remains slower than real-time NTSC; gaps are expected.
