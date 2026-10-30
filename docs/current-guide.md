# ZZSNES user guide

Use the [offline installer](package-manager.md), or extract the manual ZIP into a writable drawer and open **ZZSNES**. Keep the player
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

Current development builds offer **Fullscreen**, **Window 1× (320×240)**,
**Window 2× (640×480)** and **Window 3× (960×720)**. A window falls back to a
smaller integer size when it cannot fit Workbench. **Alt+Return** switches at a
completed frame boundary and returns to the previous window size. Larger
windows may show fewer pictures to preserve emulation and audio timing; 1× or
fullscreen has the lowest display cost. The launcher remains resizable.

**Scanlines** uses Xanxi's FPGA interface in fullscreen and Soft ARM software
filtering in a window. **F7** cycles Off → Soft → Gradient → Classic in fullscreen,
and toggles Off/Soft in a window. Compatible FPGA firmware is required for the
fullscreen effect. The bundled tools are installed automatically by the offline
installer, or manually with **Install-Scanlines**. The separate utility need
not run. Hardware filtering is disabled in a window and on normal exit.
F9 captures the framebuffer before the hardware effect is applied.

**On-screen FPS** counts emulated frames over roughly one-second intervals.
Frameskip can reduce the number of distinct pictures displayed.
**Optimised presenter** enables guarded NEON row stores/dimming when firmware
has enabled NEON. An integer fallback remains available; this does not change
the ARM clock or firmware policy.

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
| F7 | Fullscreen scanline modes; windowed Off/Soft |
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

## Defaults and scaling

New libraries default to fullscreen with Soft scanlines. Existing saved choices
remain in effect. F7 changes the running session; the launcher retains your
saved default. Sharp pixels uses nearest-neighbour scaling. Smooth scrolling
blends adjacent horizontal pixels, softening edges at additional presentation
cost. It does not change game timing or region.

## ROM Manager

Bulk import scans the selected folder and its subfolders once. Duplicate cleanup
shows the file being scanned and permits cancellation; ZIP decoding takes longer.
It removes duplicate library entries, not ROM files or saves. Backup copies
saves, snapshots and library settings to a new verified folder on a mounted disk
or USB drive. [Details](rom-manager.md).

## Region, profiles and emulation options

The ROM header supplies the region and target beside the preview: PAL 50 fps or
NTSC 59.92 fps. Games that exceed the processing budget can run slower; a 60 Hz
monitor can show uneven motion for PAL material.

Display includes fixed/automatic frameskip, filter strength, transparency,
emulated CPU timing and experimental high-resolution output. Save for game stores
those choices with that ROM's controller and cheat settings. Edit defaults changes
the global defaults; Use defaults removes the selected game's active profile.
Save edits before selecting another ROM.

SRAM loads on startup and saves on clean exit. Save states include SRAM and chip
continuation state. Version-2 standard states remain readable; new option-bearing
states need the current player. Keep old settings/states when rolling back.

Controllers supports two-player mapping. Player 2 is disabled by default and its
keys must be assigned. Save states also contains the Game Genie / Action Replay
entry controls. [Complete settings guide](additional-options.md).

## Audio output

In Display, choose **Audio output (all games)** and save settings. Auto upgrades
an existing Paula unit-0 configuration to per-app 14-bit stereo at 22.05 kHz;
other configured output devices stay unchanged. Choose Paula 28 kHz for higher
rate, Configured AHI for system-managed output, or ZZ9000AX for AX-connected
speakers. Unavailable low-level hardware falls back to unit 0. The application
never changes global AHI preferences. [Details and tests](output-quality.md).

Automatic and manual screenshots briefly pause playback. Queued audio finishes
before capture; playback starts again with a fresh prebuffer.
