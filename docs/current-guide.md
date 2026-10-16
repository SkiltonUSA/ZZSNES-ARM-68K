# ZZSNES 0.6.1 Beta

Extract the package into a writable drawer and open **ZZSNES**. Keep the player
executable beside it. When upgrading, retain your ROMs, States, Snapshots and
Library.cfg; older library settings are read automatically.

The TF4060 is part of the test machine, not a requirement. Host executables
target 68020 instructions; the ZZ9000 Cortex-A9 runs emulation. A faster 68k
helps OS, I/O and audio servicing. Performance on a 68030 is not yet verified.

## Games

**Add** selects a ROM; **Folder** imports a drawer. Search, Recent and Favourites
filter the list. Search updates shortly after typing stops; Return applies it
immediately. Up/Down selects a game; a letter jumps to the first matching title.
**Rename** changes its library title while preserving its filename and saves.

**Delete** asks Yes/No, then removes the ROM file and its associated battery saves,
library save states and captured snapshot. Imported artwork is not deleted.

**Scan drives** searches mounted Workbench volumes, including USB volumes supplied
by your installed USB/filesystem drivers. Automatic discovery checks for new
volumes while the library is idle. It never mounts disks or launches files.
Scanning shows results as they arrive. **Stop scan** cancels the search. The library
grows with available memory; there is no fixed ROM-count limit. Use Scan drives again after adding files to an existing
volume.

### ZIP cartridges

Add, Folder and Scan drives accept ZIPs containing one matching cartridge
(`.nes` for ZZNES; `.sfc` or `.smc` for ZZSNES). Stored and deflated ZIPs work;
no external unpacker or temporary extraction drawer is needed. Companion text
files and macOS metadata are ignored. The game is checked against its CRC before
being sent to the ARM.

Multi-ROM, encrypted, split and ZIP64 archives are not supported. Extract the
selected game from those archives first. Existing cartridge-size and hardware
restrictions still apply. ZIP loading does not enable new cartridge processors.

The archive path is retained in the library. Renaming the library title retains
its saves. Converting an existing raw-ROM entry into a ZIP is a new library path;
ZZNES path-based saves are not automatically migrated. ZZSNES content-based save
identity is unchanged. Keep old saves when reorganising your collection.

**Delete removes the entire selected ZIP archive**, plus that entry’s associated
saves and captured snapshot, after confirmation. It does not delete just the
member inside the archive.

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

**EmuELEC bezel** adds the supplied dark speaker panels. Both supplied PNGs
have the same frame, so they share one border choice. Their centre scanline
textures are excluded; use the Scanlines selector independently. The game
viewport is unchanged. NES needs widescreen mode for external side panels;
its 4:3 viewport fills the screen.

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
