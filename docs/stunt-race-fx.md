# Stunt Race FX race graphics

The race-only corruption came from the emulator. The supplied ROM is unchanged.

## Corrections

- **Colour-window registers:** the active legacy raster renderer writes live PPU
  registers, but clip-window calculation read deferred-renderer shadows. The
  stale values allowed fixed cyan colour into the race image. The legacy path
  now reads `$212C–$212F` and `$2130` from live register RAM; the alternate path
  retains its deferred shadows.
- **GSU RAM:** expose both 64 KiB Super FX RAM banks to match the CPU map.
  A one-bank limit aliased the second bank and clamped the `$25` screen base.
- **GSU execution:** allow mirrored low-half ROM addresses when resuming an
  instruction slice. Rejecting every PC below `$8000` suppressed race geometry.
- **8-bit plotting:** use POR's freeze-high bit, rather than object-layout bit,
  to select low-nibble transparency. Full-byte nonzero colours remain visible.

These are general core corrections, not a title-name workaround. The Super FX
RAM, execution and pixel behaviour was compared with
[modern Snes9x](https://github.com/snes9xgit/snes9x/tree/1bcc369e89f08243e0a462882fb1f3e42e51de3a).
The working colour-window reference was
[Snes9x2005](https://github.com/libretro/snes9x2005/tree/a79dfe9047e7fec58808aefe48ad2bf499c7af11).
Existing upstream licence notices are retained.

## Validation

- Original synthetic cartridge: 8,192 POR/colour combinations, adjacent pixels,
  X advance, two-bank drawing, mirrored ROM execution and live colour-window
  changes. Each affected behaviour failed before its fix. Sanitizers pass.
- Expanded-cartridge, FX3, preference and battery tests pass.
- Stunt Race FX, Star Fox 2, Doom and Super Mario World: deterministic 30-frame
  video and stereo-PCM replay after save/load, including a fresh process.
- The built ARM player resumes the physical race checkpoint at two relocation
  addresses, with identical 30-frame video/PCM results and corrected colour.
- Earlier interpreter and generated-register-handler runs matched native output;
  the ARM register optimisation was not the cause of the colour fault.

Inspect race-entry frames as well as later play: repeated Start presses can
change the game's fixed colour and hide the cyan problem. A late screenshot or
successful state replay alone did not establish correct rendering.

On the A4000TX, a fresh-boot 3,940-frame run showed the corrected race image in
windowed and fullscreen captures. Pause, F5 save, F8 load, Alt+Return switching,
resume and F10 quit passed. All 2,106,779 PCM frames were queued/submitted/completed;
the log reports zero audio gaps/errors and successful register/MMU/cache-witness
restoration and owned-memory release. This is accounting evidence, not a listening test.

![Corrected race capture on the A4000TX](screenshots/stunt-race-fixed.png)

The image above is the 256×240 framebuffer captured in the physical player’s
save state (frame 3,478). The separate F9 window/fullscreen previews also agree.
Native race-entry output and ARM checkpoint rendering were inspected.

These are bounded compatibility checks, not full-game completion or a new FPS
benchmark. Save states made after executing with the older corrupt mapping may
contain damaged guest state; start a fresh game if an old slot still misbehaves.

## Other repositories reviewed

[Snes9x GX](https://github.com/dborth/snes9xgx) is a useful modern-core reference;
its Wii frontend is not an Amiga integration layer.
[Snes9x-Z](https://github.com/andrezaiats/snes9x-Z/tree/c3c6710f9efcfec4c2c513baeb602cf17dd7d909)
adds renderer/audio optimisations and a threaded PPU. Its reported gains were
measured on x86-64 and Apple arm64, not Cortex-A9. Most of its gain needs a spare
CPU core; our XACP application uses Core1, so claiming that gain here would be
incorrect. Sprite batching, fixed-colour specialisation and avoiding redundant
PPU work are candidates for later single-core profiling. None is included in
this compatibility fix.

[Recorded checks](../amiga/records/2026-10-29/stunt-race/).
