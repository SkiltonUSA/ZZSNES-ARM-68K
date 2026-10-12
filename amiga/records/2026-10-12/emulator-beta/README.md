# Emulator Beta migration — 12 October 2026

Installed ZZNES 0.5.0 Beta and ZZSNES 0.4.0 Beta under SD032G:Emulator,
with 11 NES and 10 SNES ROMs, existing states, thumbnails and preferences.
Previous Dev drawers remain intact as recovery copies. No GitHub push.

- `build.json`: source hashes and compiled package provenance before personal
  library configuration. `package-md5.json`: final configured package hashes.
- `verified-copies.json`: original file copy hashes, including unchanged saves.
- `verified-short-roms.json`: final ROM paths/hashes after shortening names to
  leave room for battery-save suffixes. No ROM or save bytes are in this record.
- `move-plan.json`: final portable paths, titles retained in the configuration,
  NES save filename remapping; SNES states retain content keys.
- `battery-result.json`: filesystem truncation aliases discovered; apparent
  `.sav` files had the same bytes as their ROM, not genuine battery saves.
- SNES lifecycle and resume logs: paused save/load, snapshot, unrestricted
  interactive session then fresh-process resume. Clean restore and audio drain.
- NES resume log: existing migrated user save loaded, 120 frames, clean exit.
- UI logs and final audit: both versioned libraries opened and closed, no
  emulator/Core1 owner left running, executable/config hashes verified.

The bridge directory parser misreads filenames containing parentheses as
metadata (e.g. Airwolf/Contra/Aladdin/F-Zero may display size zero). Literal
native copies to a simply named RAM file supplied the verified hashes; the
zero-size directory presentation is not evidence of an empty ROM.

Screenshots stay in `.context/amiga/emulator-beta/` on the Mac. Build with
`scripts/build_emulator_beta.py`; one-time migration planning is in
`scripts/prepare_emulator_move.py`. The SNES Beta promotes the accepted host
cadence into normal play with states/SRAM/snapshots, without changing its ARM
payload, audio worker or fullscreen handoff. These runs validate lifecycle,
not new FPS claims or universal cartridge compatibility.
