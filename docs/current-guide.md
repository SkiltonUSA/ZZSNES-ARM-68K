# ZZSNES 0.6.1 Beta

Extract the package into a writable drawer and open **ZZSNES**. Keep the player
executable beside it. When upgrading, retain your ROMs, States, Snapshots and
Library.cfg; older library settings are read automatically.

The TF4060 is part of the test machine, not a requirement. Host executables
target 68020 instructions; the ZZ9000 Cortex-A9 runs emulation. A faster 68k
helps OS, I/O and audio servicing. Performance on a 68030 is not yet verified.

## Games

Select a game, then use **Launch** in the bottom bar. It stays visible on every
page, so you can adjust settings and launch without returning to Games.
**Continue latest save** and the save-slot controls resume saved progress.

**Add** selects a ROM; **Disks** scans a chosen drawer or drive. Search, Recent and Favourites
filter the list. Search updates shortly after typing stops; Return applies it
immediately. Up/Down selects a game; a letter jumps to the first matching title.
**Rename** changes its library title while preserving its filename and saves.

**Delete** asks Yes/No, then removes the ROM file and its associated battery saves,
library save states and captured snapshot. Imported artwork is not deleted.

The library grows with available memory; there is no fixed ROM-count limit.
Use **Disks / Scan once** to discover ROMs in a selected directory or USB drive.

### ZIP cartridges

Add and Scan once accept ZIPs containing one matching cartridge
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

The development build also offers **Thin (640x480)**. It keeps each image row and
inserts a black line below it, avoiding Classic's loss of alternate rows of small
text. Thin uses a larger window or an existing 640×480 fullscreen RTG mode and
requires more rendering work. The published 0.6.1 package does not include it.

The **Software scanlines** setting uses the ZZDosBox-derived ARM pixel routine;
Xanxi's separate **ZZScanlines** Workbench utility is not required. The emulator
does not call that utility or write its scanline-control registers. If global
Workbench scanlines are enabled, turn them off for an undoubled software effect.
Classic can hide fine text at 320×240; Soft retains detail in the dimmed rows.

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

## Disks

Choose a drive or directory with **Browse...**, then press **Scan once**.
Subdirectories are included; existing library entries are skipped. **Stop scan**
ends a scan while retaining games already found. No automatic or background
drive discovery runs on startup or when a USB drive is inserted.
The red **Launch** button stays in the bottom bar on every page.
