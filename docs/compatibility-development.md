# SNES compatibility development

**0.6.4-dev update:** [Expanded cartridge integration and save-state fixes](expanded-cartridges.md).
The results below describe the earlier candidates.

The normal 0.6.1 Beta still supports standard cartridges. Super FX and SA-1
remain experimental; the following work does **not** enable them in the release.

The supplemental development player now includes an [experimental FX3
implementation](fx3-development.md), with native, relocated A9 and original
physical instruction-fixture checks. Earlier launcher-only results below did
not exercise those extensions. Commercial FX3 games remain unverified.

## Implemented candidate fixes

Rebuild the derived sprite list with `S9xSetupOBJ()` after restoring the complete
snapshot. This leaves the serialized format unchanged. At frame 600, Imperium
and Stunt Race FX now reproduce all 30 subsequent video/PCM hashes, including
loading into a fresh native process. Previously Imperium differed for one frame
and Stunt Race FX for 18 frames. Longer tests still expose other failures.

The ROM timing correction described below is also included. Both changes are
stored in `development/compatibility-20261016/`, with a patch
against the archived core. Since 18 October, **all fresh ARM builds** include
the sprite-list restore fix; cartridge/timing changes still require the explicit
compatibility flags. `make current` without `--arm` retains the archived payload.
See [the standard-player restore regression](state-restore-20261018.md).

## Earlier sprite-only candidate results

Each native run uses ASan/UBSan, presses Start at frames 360–367, saves at the
specified checkpoint, and compares the following 30 video and PCM hashes after
same-process and fresh-process restore. Both restore modes give the results below.

| Supplied game | Frame 600 replay | Frame 2,400 replay | A4000TX candidate |
| --- | --- | --- | --- |
| Super R-Type | Exact | Exact | Earlier standard 120-frame check |
| Super Star Fox Weekend | Video differs | Video/PCM differ | 600 frames; clean teardown |
| Super Street Fighter II | Exact | Exact | Earlier standard 120-frame check |
| Stunt Race FX | Exact after fix | Video differs | 600 frames; clean teardown |
| Star Fox 2 | Exact, but blank | Exact, but blank | 600 frames; clean teardown |
| Star Fox | Exact, but blank at checkpoint | Exact, but blank at checkpoint | 600 frames; clean teardown |
| Kirby Super Star | Exact, blank at checkpoint | Video differs | 600 frames; clean teardown |
| Imperium | Exact after fix | Exact | 600 frames; physical save/load/capture passed |
| GunForce | Exact | Exact | Earlier standard 120-frame check |
| Final Fantasy III | Exact | Video differs | Earlier standard 120-frame check |
| Doom | Exact | PCM differs | 600 frames; clean teardown |

All native runs completed without reported ASan/UBSan errors. An exact replay
of a blank screen does not establish game compatibility. Physical runs used
no scripted game input and therefore do not reproduce every native checkpoint.
They verify Core1 execution, display/audio lifecycle and register/MMU/cache
restoration, not complete gameplay or sustained FPS. The unchanged memory layout
still reserves 32 MiB; none of these ROMs exceeds the existing 4 MiB ROM limit.

[Logs and tested binary hashes](../amiga/records/2026-10-16/compatibility/).
The normal installation, ROMs and existing states were not replaced. The
candidate was tested in a separate development drawer.

## Super FX ROM timing fix

The startup lead is now traced to `SuperFXROMMap()`: its timing loop initialized
only the lower eight 4 KiB blocks in each bank. ROM at `$00–3F:8000–FFFF` could
therefore retain zero-cycle fetch timings. A first-load Star Fox 2 trace showed
`CPU.MemSpeed = 0`; the later FastROM update covers upper banks, not these banks.

The candidate initializes all 16 blocks, preserving six-cycle I/O and eight-cycle
slow ROM/RAM timings. No sound-port acknowledgements are fabricated, and no
per-game delay, ROM patch, APU overclock or extra RAM is used.

An independent native comparison with
[Snes9x2005](https://github.com/libretro/snes9x2005/tree/a79dfe9047e7fec58808aefe48ad2bf499c7af11)
showed the same initial upload bytes, but different ordering afterward: the old
core sent command `$0F` before the SPC driver cleared its input ports with `$F1=$F0`.
The corrected timing lets initialization finish before that command arrives.
The original regression fixture verifies the bank table from both zero and
stale values, then checks mapped CMP/DEX/BNE costs of 30/14/22 master clocks.
It fails against the old mapper and passes with the candidate under ASan/UBSan.

The repeated 11-cartridge native survey completed 600 and 2,400 frames per ROM
without sanitizer errors. Start is pressed at frames 360–367. Both Star Fox
and Star Fox 2 now produce changing video and execute Super FX. Star Fox 2
records 112 and 712 picture changes respectively; these are checkpoint counters,
not display FPS or proof of complete graphical accuracy.

| Super FX title | Frame 600 replay | Frame 2,400 replay |
| --- | --- | --- |
| Star Fox 2 | Video differs | Video differs |
| Star Fox | Video differs | Video differs |
| Super Star Fox Weekend | Video differs | Video/PCM differ |
| Stunt Race FX | Exact | Video differs |
| Doom | Exact | Exact |

Same-process and fresh-process results agree. Standard-cartridge results and
Kirby Super Star's remaining state issue are unchanged by this mapper-only fix.
Doom previously differed in PCM at frame 2,400; its execution path has changed,
so this result does not establish that the general audio-state problem is solved.

Star Fox 2, Star Fox and Doom also completed 1,200-frame windowed runs on the
A4000TX, with clean register/cache restoration and owned-memory release. These
no-input startup runs establish lifecycle behaviour, not full gameplay support.

[Follow-up evidence](../amiga/records/2026-10-16/superfx-timing/).
The candidate remains separate from the published beta. Complete chip-state
serialization, graphics accuracy and longer gameplay checks are still required.

### MesenCE and FX3

The development launcher permits Super FX and FX3 launches with Play. Ordinary
Super FX cartridges use the timing-corrected player; FX3 header types `0x17`
and `0x18` display an advisory compatibility warning without blocking launch.
Detection works after ZIP decoding and copier-header removal, without filename
guessing. At this launcher-only stage, FX3 extensions were not yet implemented.
`make test-fx3-warning` checks all cartridge IDs and malformed headers under
ASan/UBSan. That header test alone establishes no FX3 game execution.

[MesenCE's GSU implementation](https://github.com/nesdev-org/MesenCE/blob/4b8a196e299b3de297b204fd1208d2981562c89b/Core/SNES/Coprocessors/GSU/Gsu.cpp)
has explicit FX3 mappings and a four-times emulated clock multiplier.
Its [instruction implementation](https://github.com/nesdev-org/MesenCE/blob/4b8a196e299b3de297b204fd1208d2981562c89b/Core/SNES/Coprocessors/GSU/Gsu.Instructions.cpp)
adds FX3 commands but still permits an interrupt on STOP, despite comments that
FX3 hardware has no IRQs. It is a useful comparison reference, not a host-speed
optimization or a fix needed by this regular Super FX cartridge. No MesenCE
code has been copied into the PocketSNES core.

The supplied [UltraStarFox2 source](https://github.com/Sunlitspace542/ultrastarfox2/tree/3397698f99cf89d5b7bcaec2ffcd520d4fc84a91)
is useful for following the sound-upload and coprocessor startup sequences.
It contains 65816 game code and Super FX code, rather than a native ARM engine.
Its [boot sequence](https://github.com/Sunlitspace542/ultrastarfox2/blob/3397698f99cf89d5b7bcaec2ffcd520d4fc84a91/SF2/ASM/BOOTNMI.ASM)
initializes sound early; its [configuration](https://github.com/Sunlitspace542/ultrastarfox2/blob/3397698f99cf89d5b7bcaec2ffcd520d4fc84a91/SF2/CONFIG/ROM.INC)
includes region, Super FX speed and cache switches. It is a modified game source,
so symbols and behavior must be matched to the user's ROM revision before use.
No game code or assets from it are incorporated into ZZSNES.

## What snes9xTYL contributes

[Reviewed revision](https://github.com/esmjanus/snes9xTYL/tree/6ce09a0ada7cc10b57ab8d7134385f44f29a9629), branch `mecm`:

- Its [history](https://github.com/esmjanus/snes9xTYL/blob/6ce09a0ada7cc10b57ab8d7134385f44f29a9629/README.md)
  records Super FX accuracy changes, SA-1 wake/sleep work, save-state fixes and
  broader coprocessor support. These are candidates to audit, not measured gains
  for the A9.
- Its [mapper](https://github.com/esmjanus/snes9xTYL/blob/6ce09a0ada7cc10b57ab8d7134385f44f29a9629/memmap.cpp)
  configures two GSU RAM banks and determines Super FX SRAM size from the extended
  header or a fallback, including Star Fox 2. This provides a more useful model
  than simply raising a global ROM-size limit.
- Its [Super FX execution](https://github.com/esmjanus/snes9xTYL/blob/6ce09a0ada7cc10b57ab8d7134385f44f29a9629/fxemu.cpp)
  differs in instruction budgeting and interrupt handling. Those differences need
  correctness tests before transplanting them into our older core.
- The PSP graphics, Media Engine coordination and platform-specific optimisation
  paths require different hardware. We would retain our Amiga/MUI/XACP frontend
  and adapt relevant portable core fixes individually. Game-specific speed hacks
  should be evaluated separately from accuracy fixes.

For portable Super FX snapshot coverage, modern Snes9x's explicit scalar and
relative-pointer fields remain the reference described in
[snes-memory-compatibility.md](snes-memory-compatibility.md).

## Next compatibility work

1. Check Star Fox/Star Fox 2 rendering and gameplay after the ROM-timing fix;
   do not confuse changing video with full cartridge compatibility.
2. Complete Super FX internal-state serialization without storing host pointers.
3. Preserve the raster/HDMA and audio timing state exposed by the longer replays;
   Super Star Fox Weekend can return from the core with a frame partly drawn.
4. Repeat active-gameplay and fresh-process/relocated-A9 state checks, then test
   on hardware. Enable individual cartridge families only after these pass.

## Reproduction

Original timing regression (no commercial ROM or hardware required):

```sh
make test-superfx-timing
# Expected to fail at the mapper assertion, demonstrating the old bug:
python3 scripts/test_superfx_timing.py --baseline
```

Native survey (provide your own ZIP cartridges):

```sh
python3 scripts/probe_cartridges.py --allow-experimental-chips --frames 2400 game.zip
# Add --baseline to compare the unchanged release core.
```

Results and optional frame/state artifacts stay under ignored `.context/`.
The survey records individual failures in JSON; completing the survey does not
mean every game passed. It does not deploy or contact hardware.

Build the candidate with the normal cartridge gate:

```sh
python3 scripts/build_current.py --arm --compatibility
```

For explicit CLI-only chip testing, use `--experimental-chips` instead of
`--compatibility`. Output goes to `.context/compatibility-build/`; the released
build directory and archived payload remain unchanged. The MUI gate is unchanged.
Use the ARM toolchain/container environment from the normal build guide.

```sh
CURRENT_ARM_DIR=.context/compatibility-build/player python3 tests/amiga/emulator/check_current_arm.py
python3 tests/amiga/pocketsnes/check_current_state.py \
  .context/compatibility-build/player/pocket.elf /path/to/authorised-rom.sfc --frames 6
```

The state model checks two relocation bases and rejects corrupted/wrong-ROM
states. Longer game runs require more time than the six-frame original fixture.

## Launcher integration

Build `make current-superfx` with the documented ARM toolchain. It produces
`ZZSNES` and `ZZSnes-FX` under `.context/fx3-build/`. Install both beside the
normal `ZZSnes-ui`; the launcher chooses the player from the selected ROM header.
Standard games continue to use the normal player. Super FX games include the
previously tested Star Fox 2 and Doom. Warning text does not disable Play.

This uses the same compatibility mapper/state overlay as the isolated tests,
with a narrower gate admitting Super FX/FX3 and standard cartridges, not SA-1.
The 4 MiB bound and header checks remain. Permitting a launch alone did not
implement FX3; the later core overlay is documented separately above. Special-chip
save-state accuracy remains experimental. Published beta archives are unchanged.
