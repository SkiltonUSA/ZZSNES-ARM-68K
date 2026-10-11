# Next steps

The current 4.6166 ms figure establishes promising simple-fixture throughput.
A working game pipeline is the next objective; further micro-optimisation can
wait until measurements identify a real bottleneck.

1. **Real-content core validation.** Add bounded ROM loading for ordinary
   LoROM/HiROM cartridges, initially rejecting unsupported enhancement chips.
   Use an original animated/audio fixture and a user-supplied SNES game.
   Compare ARM video and active PCM with the native reference, then measure
   sustained frame costs, worst-case spikes and cancellation. Exit gate:
   correct content and enough headroom for the region's 50/60 Hz budget.
2. **Fullscreen presentation.** Reuse the verified XANI-style XACP double-buffer
   approach from the NES work. ARM converts pixels into an OS-owned ZZ9000
   bitmap; 68k owns display setup, input and flips. Handle 256x224/239, aspect
   ratio and borders first. Leave hires/interlace until separately validated.
   Exit gate: correct geometry, no tearing/corruption, clean Workbench return.
3. **Input, AHI sound and frame pacing.** Map the connected joystick and keyboard;
   deliver PCM through the established bounded 68k/AHI queue. Do not assume
   direct ARM-to-ZZ9000AX audio ownership. Test pause/resume/quit, underruns,
   input response and long-session cleanup. Exit gate: sustained complete-game
   performance with active audio and every intended frame presented.
4. **MUI launcher and persistence.** Reuse the NES/xSysInfo interaction patterns
   for ROM selection, history, favourites, controller configuration and screen
   options. Add SRAM persistence first. Integrate upstream save-state routines
   only after round-trip state and resume correctness tests pass; states need
   explicit core/build compatibility rules.
5. **Compatibility and optimisation.** Test more games/video modes and region
   timing. Profile before choosing ARM assembly, NEON or specialised renderers.
   Evaluate enhancement chips separately. Keep Mesen as a reference path and
   preserve working binaries before changing core or scheduling.

No physical changes are implied by this roadmap. The installed program remains
a headless diagnostic until these milestones are implemented and accepted.
