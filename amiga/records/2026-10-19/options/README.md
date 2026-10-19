# ZZSNES 0.6.5-dev options validation

`build.json` identifies the source, resolved build inputs and executables.
`installed.json` records the new and previous executable checksums. The matched
launcher and players were installed with the library file unchanged.

- Native sanitizers: cartridge families/state regressions; SRAM and fresh-core
  reload; independent pads; PAR/Game Genie validation, mapped SRAM apply/restore;
  automatic frameskip recovery, filter/transparency and option state storage.
- Host battery transaction: initial SRAM preserved; persist/reload; backup
  rotation; failed rename rollback; malformed-file refusal.
- Cortex-A9 instruction model: both relocation addresses, SRAM execution,
  both pads, low-audio skip/recovery and cheat bounds; presentation and exact
  state video/PCM replay, including a 120-frame SMW checkpoint with new options.
- Physical MUI: 640x480 layout budget, all seven scrolling pages and window
  resizes; global defaults preserved, profiles isolated/reloaded/reset,
  mapping/cheat fields and remapped joystick indicator.
- Physical player: mapped SRAM cheat persists 0x5a; next process without cheats
  loads it and advances to 0x5b. SMW pause/save/load/F9/window-fullscreen/exit and
  fresh-process resume pass. See `physical.json` for counters and audio gaps.

All final physical player cases restored registers/cache witnesses and released
owned memory. These are functionality checks, not steady-FPS or gap-free-audio
benchmarks. A second physical controller has not been tested. The 640x480 check
uses a window budget on the current desktop; no Workbench screen mode was changed.

`initial-cheat-mapped-write.log` is deliberately excluded from the final case
list. Its run exited cleanly but SRAM stayed unchanged by the cheat. Investigation
found reversed byte/address arguments in the inherited mapped-memory cheat
write/restore calls. Native and relocated tests now cover this path; the final
physical cheat and reload checks pass after that correction.

ROMs, states and private screen captures are excluded from this directory.
