ZZSnes-Mesen 0.2.0-dev - separate SNES development app

CPU test: original 65C816 instruction diagnostic.
System test: six original cartridge frames on the Cortex-A9, with
CPU, memory bus, DMA, PPU, SPC700 and DSP emulation. No ROM needed.
Results appear in the window; details are in Last-CPU.log / Last-System.log.
Close other ARM games before running a test.

Physical system test: approximately 40.7 ms per emulated frame.
Headless, simple silent startup fixture; not game/display FPS.
Display, audible audio, game loading, states and broader cartridge
support remain development work. ROMs/States are reserved for later use.

Original CPU executable and build.json retained. New system metadata
is system-build.json. Previous UI backed up as ZZSnes-Mesen.before-system.
Mesen2 GPL-3.0-or-later: b9fa69ddc6d0a331fb103fdb5eef6904305703c2
https://github.com/SourMesen/Mesen2
XACP by XANI: https://github.com/Xanxi-Amiga/XACP-ZZ9000
