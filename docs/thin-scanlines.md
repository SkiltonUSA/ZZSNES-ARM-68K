# Thin scanlines candidate

Classic at 320×240 blanks alternate game-image rows. Small text can lose entire
strokes; monitor scaling then magnifies each black row. The formula is unchanged
from the 15 October sidebar beta. Saved pre-ZIP/pre-border preferences had
software scanlines Off; the settings inspected on 16 October selected Classic.
That comparison identifies a setting difference, not a proven renderer regression.

The optional **Thin (640x480)** mode instead doubles each existing presentation
row horizontally and places a one-pixel black separator below it. All 240 logical
rows remain visible. It preserves the existing viewport/aspect mapping; it does
not add a sharpening filter or recover detail already lost by that mapping.
Off, Classic, Soft and Gradient retain their existing 320×240 output.

Thin uses an existing 640×480 true-colour P96 mode for fullscreen, or a 640×480
Workbench window. Alt+Return retains the selected effect. Screen buffers are
still OS-owned, runtime-validated and flipped using the existing XACP protocol.
The larger span can cross three MMU sections, all revoked after presentation.
No firmware or separate Workbench scanline control is changed.

The FPS panel is drawn over the effect. Captured library pictures sample the
intact image rows rather than baking the inserted black lines into thumbnails.
The larger framebuffer costs more rendering work; do not infer unchanged game
performance from the unchanged emulation core.

Build separately with `make current-thin` and, for the supplemental player,
`make current-thin-superfx`. Outputs go to `.context/thin-build` and
`.context/thin-fx3-build`. Normal builds and published beta remain unchanged.

Pixel-model checks compare every output pixel against doubled unfiltered rows,
including untouched row padding, both pixel formats, two relocation bases,
both optimisation settings, three-section mapping cleanup and cancellation.
Run `TEST_THIN_SCANLINES=1 CURRENT_ARM_DIR=.context/thin-build/player` with
`tests/amiga/emulator/check_current_arm.py` after building. These checks do not
prove the monitor's visual appearance or physical display timing.

The candidate passed physical fullscreen, window/fullscreen switching, snapshot
and exit checks. The 1,200-frame fullscreen run averaged 16,653 µs per loop after
warm-up, but whole-run display throughput was 45.56 FPS including startup and
readback. Thin has a real rendering cost. [Measurements and test scope](../amiga/records/2026-10-16/fx3-thin/).
