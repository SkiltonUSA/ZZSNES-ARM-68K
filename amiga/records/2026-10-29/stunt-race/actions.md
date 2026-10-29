# Stunt Race FX graphics correction — 29 October 2026

- Read installed/release contracts; isolated the supplied Rev 1 ROM and kept it unchanged.
- Reproduced missing race geometry and colour corruption in native core output; inspected modern Snes9x, Snes9x2005 and the supplied GX/Z references.
- Added failing synthetic pixel, RAM-map and ROM-execution regressions before each GSU fix. Corrected transparency, two-bank RAM and mirrored low-half execution. Geometry returned, but early race colour remained wrong.
- Rejected the assumption that a correct late screenshot proved the fix: repeated Start input changes the game's fixed colour and hid the fault. Compared early race frames and stopped extra Start presses.
- Native, ARM interpreter and generated-register-handler output matched; relocations matched. The actual physical state reproduced cyan in the ARM model, excluding the host pixel conversion as its cause.
- Compared live PPU writes with Snes9x2005. Found stale deferred register reads in legacy clip.c. Added a failing register-write test, selected the live registers for that raster path and preserved deferred behaviour for the alternate path. Main/subscreen window mask tests also pass.
- Built matching 0.7.4-dev binaries. Expanded/FX3/options/battery and sanitizer tests passed; deterministic video/PCM replay passed in-process and fresh-process for Stunt Race FX, Star Fox 2, Doom and Super Mario World.
- The exact ARM player restored the physical race checkpoint at two relocations with identical corrected output. A fresh physical 3,940-frame run passed window/fullscreen capture, pause/save/load/resume/quit and resource cleanup. No new FPS claim.
- Built and checked ZIP and signed offline installer equivalence, dependencies, signatures, corrupted-signature rejection and private-data exclusions.
- Test fixture expansion initially had C preprocessing ambiguity in a hexadecimal literal followed by '+'; spacing fixed the fixture compilation. No product code change was required.
- Dedicated crapper/dryer/mutator tools were unavailable; no result is claimed for those tools.

- Installed the exact 0.7.4 signed package in SD032G:Emulator/ZZSNES after a private backup. Executable hashes match; Library/Controls/Session settings are unchanged.
- Removed only SD032G:Dev/ZZFX1029. Core1 idle and bridge alive; AHI CRC725CE7DB and saved screenmode CRC9C80FBC5 unchanged.
- Trimmed inherited whitespace in the copied copyright comment; a final rebuild is checked against the tested binary hashes before publishing.

