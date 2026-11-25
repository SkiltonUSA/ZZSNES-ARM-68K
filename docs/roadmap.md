# ZZSNES remaining work

Current features and installation are in [the README](../README.md). Earlier
0.2.x measurements are historical. Per-game thumbnail states, battery SRAM,
expanded cartridge/settings controls and per-app audio choices are implemented.

The 0.7.13 clipping correction, row renderer, signed installer and installed-player
checks are complete. [Release evidence](validation/release-0.7.13/README.md).

1. **Super FX continuity:** Star Fox 2, Stunt Race FX and Doom still miss audio
   deadlines in demanding scenes. Profile GSU and PPU work before adding buffers. The existing timed
   Super FX experiment was slower in the recorded boot test and stays optional.
   [Investigation](starfox2-audio.md). Two fetch experiments are now measured: a
   derived mask is slower; an ordinary-GSU loop reduces core time by 0.66% in the
   matched Star Fox 2 sequence but does not resolve starvation. Neither is promoted.
   [Results and next validation](validation/gsu-fetch/README.md).
2. **Cartridge coverage:** broaden commercial C4/S-DD1, enhancement-chip states
   and FX3 checks. Existing gates and fixture passes do not prove every game.
3. **Video:** broaden high-resolution/interlace and scrolling checks; preserve
   colour math, sprite clipping and pixel-aspect behaviour during optimisation.
4. **Audio/input:** extend long-session and listening checks for Paula/AHI/AX,
   two-player controllers and pause/resume. Correct PCM publication is already
   implemented; starvation is a separate production/scheduling problem.
5. **Persistence:** broaden SRAM/state recovery and legacy-state coverage.
   SRAM persistence on clean exit already exists.
6. **Release provenance:** keep source, player hashes, launcher, README and signed
   installers aligned. [Published-source audit](release-source-audit.md).

22.05 kHz Paula and optional clock controls are integrated. Native SNES audio
synthesis is retained; selecting a lower output rate does not remove SPC700/DSP
emulation. No Next AY/TurboSound firmware dependency is introduced.

[Current released-player audio checks](audio-release-check.md).
