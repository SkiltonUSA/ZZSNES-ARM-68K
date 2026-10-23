# ARM SNES core references — 10 October 2026

The user supplied these references during the NES-style optimisation pass.
They were reviewed from pinned source checkouts, not run on the Amiga. No code
from these cores has been merged into the Mesen2 GPL integration.

| Reference | Pinned revision | Most useful direction |
| --- | --- | --- |
| [frank-snes](https://github.com/rh1tech/frank-snes) | `a88025e6a28b996cf2aaf4f6243804a5e7310b0c` | Tile batching/cache ideas; newer embedded Snes9x/snes9x2010 integration |
| [ARMSNES-libretro](https://github.com/RetroPie/ARMSNES-libretro) | `dbea0479cb6fba8583c0ca642ad70dd1bf0b58d1` | Small libretro boundary; ARM SPC700 and sample decoder; first comparison candidate |
| [DrPocketSNES](https://github.com/Apaczer/DrPocketSNES) | `0eef0a64828fa185fec67a6cfb712a9101650894` | ARM 65C816 fast core as well as ARM audio; explicit fast/compatible builds |

## ARMSNES-libretro

The [README](https://github.com/RetroPie/ARMSNES-libretro/blob/dbea0479cb6fba8583c0ca642ad70dd1bf0b58d1/readme.md)
describes PocketSNES ancestry. `src/snes9x.h` identifies Snes9x 1.39. The build
produces a libretro shared library with C/C++ emulation and ARM assembly;
it is not an Amiga executable or bare-metal XACP payload.

The [SPC700 assembly](https://github.com/RetroPie/ARMSNES-libretro/blob/dbea0479cb6fba8583c0ca642ad70dd1bf0b58d1/src/spc700a.s)
credits notaz and bitrider. It uses ordinary ARM registers/instructions and
hard-coded core structure offsets. `src/spc_decode.S` provides a BRR sample
block decoder. The A9 instruction set is a closer fit than frank-snes's M33
platform code, but correct ABI, relocation, data alignment, clock scheduling,
stack bounds and stop handling still need actual integration tests.

A narrow assembler check was performed with Clang targeting arm-none-eabi /
cortex-a9 / ARM state / soft-float. `spc_decode.S` assembled unchanged.
The SPC700 file initially failed Clang's integrated assembler on legacy
conditional load/store mnemonic ordering (for example ldrneb). A generated
research-only copy normalized 1,914 such mnemonics and assembled successfully.
**Object assembly only:** neither object was linked into an emulator, executed
in the A9 model or run on the physical board. No performance follows from it.
The upstream checkout was not modified.

The [libretro frontend](https://github.com/RetroPie/ARMSNES-libretro/blob/dbea0479cb6fba8583c0ca642ad70dd1bf0b58d1/libretro/libretro.cpp)
sets RenderThisFrame true in retro_run, runs the core, then mixes a 32 kHz
stereo block. Although initialization contains SkipFrames=AUTO_FRAMERATE,
that setting alone is not evidence that this frontend skips output frames.
It also enables SpeedHacks and disables SupportHiRes. Those must be explicit
in any comparison; do not imply Mesen-level accuracy from equal frame rates.
The loader later enables the APU, despite an earlier initialization assignment
of APUEnabled=false. Source presence alone does not establish enhancement-chip
or save-state compatibility after porting.

The Snes9x source headers contain noncommercial license terms. Keep any such
core a separately attributed integration with its actual license, rather than
relabeling it as Mesen2 GPL code.

## DrPocketSNES

The [README](https://github.com/Apaczer/DrPocketSNES/blob/0eef0a64828fa185fec67a6cfb712a9101650894/README.md)
targets MiyooCFW/ARMv5. `src/snes9x.h` also identifies version 1.39. The
[Makefile](https://github.com/Apaczer/DrPocketSNES/blob/0eef0a64828fa185fec67a6cfb712a9101650894/Makefile)
targets ARM926EJ-S and distinguishes a fast ASMCPU build from a compatible
USE_SA1 build. It includes the os9x 65C816 assembly family and SPC700 assembly,
so it is a particularly useful reference for reducing CPU interpreter cost.
The frontend depends on handheld/Linux services and is not an Amiga frontend.

The build enables OLD_RASTER_FX, and the
[changelog](https://github.com/Apaczer/DrPocketSNES/blob/0eef0a64828fa185fec67a6cfb712a9101650894/CHANGELOG)
explains deferred raster work and explicitly warns of possible colour/graphics
glitches. Historical speed gains reported there apply to those handhelds and
that trade-off; they cannot be transferred to the A9 or treated as exact
optimisation of our cycle/pixel-checked Mesen workload. The build also offers
profile-guided compilation; representative training needs more than our one
checkerboard fixture. Its COPYRIGHT file retains Snes9x noncommercial terms.
No DrPocketSNES compilation or runtime test was performed in this pass.

## Decision

First preserve the verified Mesen baseline and the small exact word-copy gain.
For a larger speed opportunity, a separate ARMSNES/PocketSNES headless benchmark
is a reasonable next experiment, analogous to the NES Mesen-versus-FCEUX core
comparison. This is an engineering judgement about integration effort, not a
measured performance ranking. DrPocketSNES's ASMCPU is the next reference if
65C816 execution remains dominant in that alternative core.

Use the same original cartridge, every rendered frame, active SPC/DSP, explicit
accuracy settings and full XACP/cache/teardown validation. Different emulator
cores may legitimately have different startup/cycle/audio signatures; establish
an independent native reference and compare visible semantics instead of simply
forcing Mesen's internal hashes. Then add representative authorized ROMs, active
audio and sprites/scroll/raster tests. No 60 FPS claim is established by these
repositories or the assembler probe. Full frank-snes notes are in
[FRANK-SNES-REVIEW.md](FRANK-SNES-REVIEW.md).
