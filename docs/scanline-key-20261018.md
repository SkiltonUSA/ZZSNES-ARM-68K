# F7 software scanline control

F7 cycles **Off → Soft → Gradient (smooth) → Classic (hard) → Off**.
The loading panel and controls guide list the key. It works while playing or
paused, in a Workbench window or fullscreen. It changes only the current
session; saved launcher defaults are retained. Releases and keyboard repeat
are ignored, and multiple distinct queued presses retain their cycle order.

The host applies the requested effect only after draining the outstanding
flip. It redraws the paused frame immediately. Leaving the SNES Thin mode
reopens the display at normal dimensions rather than reusing its larger bitmap.
The SNES save-state sprite fix remains in the compiled ARM payload.

This is a 68k host/launcher change. The ARM payloads were retained byte-for-byte;
no new raster effects, firmware registers or emulation timing changes were added.
Rebuild using the existing optimisation build commands. `make test-scanline-key`
covers cycle order, release/repeat suppression, queued presses and Thin exit.
Native presentation/library tests and relocated ARM pixel checks also pass.

## Physical checks

On A4000TX, scripted F7 presses exercised every standard mode during play and
pause in a window, followed by an Alt+Return transition and a fullscreen F7
change. The SNES standard-player run also started in Thin, then verified its
safe transition to Off. Framebuffer readback was enabled. All three players
matched the requested mode sequence, returned exit zero, restored register/cache
witnesses and released owned memory; display faults were zero.

NES recorded 388 frames / 20 audio gaps; standard SNES 420 / 11; supplemental
SNES 360 / 11. These are diagnostic runs with readback, deliberate pauses and
display changes, not speed benchmarks or evidence of uninterrupted audio.
The supplemental-player run used a standard cartridge; it does not expand
special-chip game or save-state compatibility.

The standard 320×240 effects shade existing output rows. Classic/Soft affect
120 alternate rows over the complete output; Gradient uses a four-row pattern.
This differs from the 224 active picture lines in a typical SNES game. Thin
640×480 preserves every logical row with a separate dark row after it. F7 does
not change standard effects into doubled-height effects.

[Build checks and hardware logs](../amiga/records/2026-10-18/scanline-key/).
