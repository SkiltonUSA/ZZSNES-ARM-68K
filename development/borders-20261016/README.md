# EmuELEC border update

The 68k frontend/ZIP sources are in source/ and host/. player/ contains the
presentation overlays, matching packed ARM image, ELF and session validator.
Other core inputs remain in development/beta-20261015/. make current rebuilds
the host/library; make current-arm also recompiles the payload. Core emulation,
ROM gates, transfer layout, firmware and cache policy are unchanged.

The supplied PNGs have byte-identical outer 240-pixel panels. borders/ preserves
both originals and a reproducible converter. Run python3 borders/build_emuelec.py
with Pillow to generate the 16-colour, packed-nibble 9,632-byte table. Copy the
generated header to player/ and build-inputs/ before rebuilding. Only side
panels are sampled; no game pixels or centre scanline textures are imported.

The first RGB565 table exceeded the NES image/page-table boundary. The compact
table fits without relocating anything. Linker allocation assertions remain.
The new theme occupies existing theme ID 7; older IDs/defaults remain unchanged.
Existing players reject theme 7, so keep the updated launcher and player together.

Artwork supplied by the user; original authorship/licence was not
provided. This artwork is not claimed as original GPL artwork by this project.
See docs/emuelec-border.md for tests, screenshots and limitations.
