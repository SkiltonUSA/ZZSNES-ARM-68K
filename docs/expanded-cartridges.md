# Expanded cartridges — 0.6.4-dev

The subsequent [0.6.5-dev options update](additional-options.md) extends these controls and adds battery saves and profiles.

This development build integrates more PocketSNES cartridge hardware and adds
emulation settings to **Display**. These changes are included in 0.6.5 Beta.

| Feature | Development support |
| --- | --- |
| SA-1 | Enabled; Kirby Super Star used for testing |
| DSP-1 | Enabled; Pilotwings used for testing |
| C4 | Enabled; original RAM/save-state fixture tested |
| S-DD1 | Built-in decompressor enabled; DMA and bank mapping tested with original data |
| Super FX | Enabled, with additional timing, graphics and GSU save-state data |
| FX3 | Experimental; original instruction and in-flight save-state fixtures tested |
| Larger cartridges | Up to **6 MiB**; above 4 MiB requires S-DD1 or ExHiROM mapping |
| High-resolution graphics | Experimental 512-pixel source detail, presented at 640×480 |
| Frameskip | Off, skip 1 of 2, 2 of 3, or 3 of 4 pictures |
| Audio filter | Optional upstream low-pass filter, 60% strength |
| Emulated CPU timing | Normal, Compatible overclock, Maximum overclock |

These options are under **Display → Resolution / Emulation**. Normal timing,
frameskip Off and filtering Off remain defaults. Frameskip reduces picture
updates while CPU/APU emulation continues. Overclock changes the emulated SNES
CPU's cycle costs; it does **not** change the Cortex-A9 clock and can increase
host workload or affect game behaviour. PAL/NTSC pacing still applies.

Use the matching launcher and both players together. The development build
reserves **48 MiB of ZZ9000 RAM**, plus display and AmigaOS allocations. Existing
libraries migrate when next saved. ROMs are not modified.

## Save states

New version-3 states include GSU registers/pipeline/cache metadata, DSP command
continuation, C4 RAM, SA-1 execution fields, scheduler/audio timing, and render
state. Addresses are reconstructed for the current allocation. States restore
emulation options; display resolution stays with the current presentation setting.

Version-2 standard-cartridge states remain readable. Old incomplete special-chip
states are rejected. New states require the new player; keep an older installation's
states when rolling back. No commercial FX3 compatibility is claimed.

The Star Fox 2 investigation also found an underlying graphics bug: writes to
Super FX SCBR changed the drawing-buffer address without invalidating cached
column pointers. Marking those pointers dirty fixes drawing-buffer selection
and the reproduced save/replay mismatch. A synthetic buffer-swap regression
covers the same condition.

## High-resolution limits

The core's former 320-pixel row allocation and Mode 5/6 half-width drawing path
could not retain 512 distinct source pixels. This build expands the rendering
pitch and buffers, retains those pixels and adds a 640×480 presentation path.
Ordinary low-resolution play keeps its smaller conversion path.

Interlace currently uses **bob/line doubling**, not field-accurate rendering.
This remains an experimental compatibility option, not a claim that every
high-resolution or pseudo-hires game renders correctly or runs at full speed.
Full-detail output increases display work, particularly in a Workbench window.

## Validation

Native sanitizer tests cover cartridge gates, C4 RAM separation from larger ROMs,
S-DD1 DMA decompression and non-power-of-two bank mirroring, 6 MiB ExHiROM,
512-column tile drawing, interlace dimensions, options migration and corrupt-state
rejection. FX3 fixtures resume an in-flight GSU loop and its second RAM bank in
a fresh core. These fixtures contain no commercial game data.

At frame 2,400, nine supplied games reproduce the next 30 video and PCM hashes
after both same-process and fresh-process restore: Super Mario World, Pilotwings,
Kirby Super Star, Star Fox, Star Fox 2, Doom, Stunt Race FX, Super Star Fox Weekend
and Final Fantasy III. Eight option combinations also pass 600-frame replay tests.
This establishes deterministic continuation at those checkpoints, not complete
game compatibility.

Relocated Cortex-A9 instruction-model checks cover 320×240 and 640×480 output,
pixel formats, software scanlines, SIMD preservation, cancellation and state
restores at two addresses. A previously captured version-2 SMW state also passes
30-frame replay in the new build. Models do not substitute for physical cache
or display checks.

The physical A4000TX checks passed normal/windowed and 640×480 fullscreen
readback; original C4, S-DD1, ExHiROM and FX3 fixtures; and paused save/load,
F9 capture, display switches, clean exit and fresh-process resume for Star Fox 2,
Pilotwings and Kirby Super Star. SMW also passed with frameskip, low-pass,
compatible emulated overclock and high detail enabled together.

Captured previews showed the Pilotwings title, Kirby’s Spring Breeze selection
and the Star Fox 2 ship introduction; these are not complete playthroughs.

All accepted physical runs restored the register/cache witnesses and released
owned RAM. Audio gap counters were nonzero in several runs, particularly Super
FX and checks involving readback or display changes; this is **not** a claim of
uninterrupted audio or a new FPS benchmark.

[Physical results](../amiga/records/2026-10-19/expanded/physical.json) ·
[Source and executable hashes](../amiga/records/2026-10-19/expanded/build.json). C4 and S-DD1 commercial-game testing remains outstanding; no suitable
commercial fixtures were part of this batch. FX3 remains experimental.

## Build

```sh
make current-expanded
make test-expanded
make test-native
make test-expanded-arm
make verify
```

The ARM build uses the existing cross-compiler environment; the model target
requires `MODEL_PYTHON` with Unicorn and `LLVM_NM`. Build and test commands do
not deploy automatically. Sources are in `development/expanded-20261019/`,
layered over the preserved beta, compatibility, FX3 and presentation sources.
Upstream Snes9x/PocketSNES notices and licences remain in place.
