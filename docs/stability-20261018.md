# Matching Display controls and follow-up checks

The 0.6.2-dev candidate now shares ZZNES's **Pixel scaling** selector:
**Sharp pixels** or **Smooth scrolling**. Both launchers default to **Fullscreen**
and **Soft software scanlines**, with Sharp selected. ZZSNES retains its extra
**Thin (640×480)** mode. The loading panel labels **F9 — Screenshot** explicitly.
Tagged releases are unchanged.

## Smooth presentation

The optional filter interpolates neighbouring horizontal pixels after the SNES
core renders. It uses centre-aligned fixed-point weights, clamps sampling to the
overscan mask, and leaves emulated game timing unchanged. A cached RGB565 palette
converts each source row; NEON table lookups blend four output pixels together,
with a scalar fallback and preserved SIMD state. Sharp remains cheaper.

Session bit 9 selects Smooth; bit 8 remains reserved for runtime PAL height.
Library.cfg now writes ZZSNESUI4, reading v1–v3 with Smooth off. Restore the
previous preferences together with an older launcher when rolling back. Standard
save-state data is unchanged; complete Super FX state support is still absent.

Native checks cover 3,317,760 presentation samples and library migration.
Relocated Cortex-A9 instruction-model tests cover both packed player variants,
two load addresses, both pixel formats, PAL height, masks, all five scanline
modes, scalar/NEON output, guard bytes, mapping revocation and SIMD preservation.
An independent channel oracle checks interpolation, including every weight and
short/tail length. Model execution is not physical cache or display validation.

The first supplemental-player model run failed because its injected framebuffer
hook was bypassed after translated-code invalidation. The fixture now redirects
the accessor's verified data literal instead. The model's CPACR-read workaround
also uses a verified instruction replacement instead of a hook, and tests assert
that the requested NEON path actually executes. Both variants then passed. These
are model-fixture corrections, not a claimed production renderer bug fix.

## Smooth hardware measurements

Four 120-frame framebuffer-readback checks passed: standard NTSC/Soft, PAL in a
Workbench window with Gradient, NTSC/Thin, and Star Fox 2 using the supplemental
player with Soft. Each used Smooth, masks and the FPS overlay. These checks
validate sampled output and cleanup; readback adds substantial diagnostic cost.

Two interleaved pairs of 600-frame SMW runs compared Sharp and Smooth with Soft
scanlines, FPS enabled and an existing snapshot. Mean ARM presentation cost was
**1.902 ms/frame Sharp** versus **2.438 ms/frame Smooth**, an extra **0.536 ms**
or **28.2%** at this stage. All four recorded zero audio gap events. This measures
presentation work, not a 28.2% change in game speed, and is not representative of
all scenes or Super FX workloads. Sharp remains the default.

The Smooth interactive run completed three paused save/load cycles, F9 capture,
four window/fullscreen switches and F10 exit (958 frames, two audio gap events).
Its freshly captured state then exposed a separate failure: a fresh process
accepted the state but timed out on the first emulation request, before any
presentation. The **previous optimisation player also failed with that same
state**, with Smooth disabled. Both attempts exited with code 20 and released
memory through the reset path; they did not complete the normal restoration
witness checks. They are retained as failed tests, not successful resumes.

The new player successfully resumed the earlier `snes-repeat-state` for 300
frames with normal restoration. Same-process load is therefore insufficient to
establish fresh-process compatibility for every saved state. The newly exposed
state-dependent failure was subsequently traced to an unbuilt sprite list and
fixed in the standard player; see the [restore fix and target tests](state-restore-20261018.md).
The presentation change itself did not repair save-state loading. See `snes-smooth-resume-failed.log`,
`resume-baseline-newstate.log` and `resume-smooth-oldstate.log` in the evidence.

Build the candidate using `scripts/build_current.py --arm --optimisation --core-o2`
(add `--superfx` for the supplemental player), with the documented toolchain.
Run `make test-optimisation-native`, `make test-optimisation-arm` and
`make test-smooth-arm`. For the supplemental image, run the smooth test script
with `CURRENT_ARM_DIR=.context/optimisation-o2-fx-build/player`.

## Longer hardware checks before the Smooth addition

| Run | Emulated frames | Elapsed seconds | Audio gap events |
| --- | ---: | ---: | ---: |
| Cannon Fodder, PAL | 3,000 | 63.987 | 10 |
| Star Fox 2 | 3,000 | 69.597 | 531 |
| Doom | 3,000 | 80.100 | 645 |
| F-Zero, scripted gameplay | 3,600 | 61.163 | 0 |

The first three are title/demo runs using Thin scanlines and FPS. F-Zero uses
scripted Start input, scanlines off and FPS off, so rows are not comparable
performance presets. F-Zero acknowledged 3,445 displayed frames; emulated frame
counts must not be advertised as displayed FPS. The heavy Super FX titles still
have substantial audio gaps and do not sustain their nominal rate.

Six alternating window/fullscreen SMW startups completed 180 frames each with
zero audio gaps. This did not reproduce the earlier isolated startup failure;
its root cause remains unproven. An interactive run completed three paused
save/load cycles, F9, four display switches and F10; a fresh process resumed for
300 frames. The interactive run recorded 16 audio gap events.

All these runs completed queued PCM, register/cache witness restoration and
owned-memory release with exit zero. Completed PCM does not prove uninterrupted
audio. The Soft diagnostic run includes framebuffer readback and is unsuitable
for performance comparisons.

See [follow-up logs and counts](../amiga/records/2026-10-18/followup/) and the
[earlier optimisation report](optimisation-20261018.md). Commercial ROMs, saves
and private library settings are excluded. No firmware, global AHI or saved
Workbench screen preferences were changed.
