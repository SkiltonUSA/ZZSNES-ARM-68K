# User-supplied Nintendo side panels

The two reference screenshots were supplied by the owner on 11 October 2026.
The NES artwork uses the outer 195 pixels at each side of its 1600x900 source;
the SNES artwork uses the outer 240 pixels of its 1920x1080 source. The embedded
game pictures, scanlines, curved television screen and mouse cursor are excluded.
Original artwork authors/licences were not supplied; these images are not
claimed as original GPL artwork. No public release is made by this change.

`python3 scripts/build_border_assets.py` converts the panels to two 40x240
RGB565 strips per theme. It requires Pillow. `border_assets.h` is generated;
edit the converter or source masters rather than its numbers. `assets.json`
records source hashes and exact crop rectangles. Runtime tables total 76,800
bytes and fit inside the existing payload allocation/linker bounds.

Both libraries expose theme 5 **Nintendo artwork** and theme 6 **Super Nintendo
artwork**. Existing theme IDs and defaults are preserved. The panel widths adapt
to the existing game viewport; widescreen mode gives the clearest side panels.
NES 4:3 fills the display, so there are no external side panels in that mode.
The original game geometry and independent 0/8/16-pixel edge masks are unchanged;
masked game pixels remain black for these themes. Top/bottom margins are black.
No scanline effect, game scaling change or firmware dependency is introduced.

The new themes require matching updated players and launchers. Older players
reject these theme values; existing configurations remain valid in the update.
