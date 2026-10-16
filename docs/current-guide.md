# ZZSNES 0.5.0 Beta

Extract the package into a writable drawer and open **ZZSNES**. Keep the player
executable beside it. When upgrading, retain your ROMs, States, Snapshots and
Library.cfg; older library settings are read automatically.

## Games

**Add** selects a ROM; **Folder** imports a drawer. Search, Recent and Favourites
filter the list. Up/Down selects a game; a letter jumps to the first matching title.
**Rename** changes its library title while preserving its filename and saves.

**Delete** asks Yes/No, then removes the ROM file and its associated battery saves,
library save states and captured snapshot. Imported artwork is not deleted.

**Scan drives** searches mounted Workbench volumes, including USB volumes supplied
by your installed USB/filesystem drivers. Automatic discovery checks for new
volumes while the library is idle. It never mounts disks or launches files.
Scanning shows results as they arrive. **Stop scan** cancels the search. The library
holds up to 128 entries. Use Scan drives again after adding files to an existing
volume. ZIP archives must be extracted first.

## Display

Choose **Fullscreen** or **Workbench window**. **Alt+Return** switches at a completed
frame boundary. The window currently displays a fixed 320×240 image. Fullscreen
retains the existing deferred framebuffer handoff; windowed output uses a private
ZZ9000 bitmap and an AmigaOS blit.

Software scanlines: **Off**, **Classic** (alternating black rows), **Soft** (5/8 brightness
on alternating rows), or **Gradient** (8/8, 6/8, 4/8, 6/8). They work with either
display mode and the optional FPS overlay. They change this emulator's output only.

**On-screen FPS** counts presented frames over roughly one-second intervals.
**Optimised presenter** enables guarded NEON row stores/dimming when firmware has
already enabled NEON. Disable it for a scalar baseline comparison. It does not
change CPU emulation, game timing or firmware policy.

Monitor shape, border themes and overscan masks remain available.

## Controls and saves

| Key | Action |
| --- | --- |
| Alt+Return | Fullscreen / window |
| F10, Q or Escape | Return to library |
| P | Pause / resume |
| M | Mute / unmute |
| F5 | Save selected slot |
| F8 | Load selected slot |
| F9 | Capture library artwork |

Five slots are maintained per game. The Save states page supports resume and
confirmed deletion. Previous states are retained as `.bak` on overwrite.
The loading panel stays active until the first completed frame or a launch error.
The About page contains credits and a scrolling change list.
