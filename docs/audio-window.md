# Audio scheduling and enlarged windows

The current development build retains configured AHI unit 0. Paula RCA output
therefore works when that unit uses Paula; merely installing an AX driver does
not redirect playback. System AHI preferences and sample rates are unchanged.

## Changes

- Wake the audio worker immediately after a complete three-frame batch arrives.
- Use a six-frame NES startup prebuffer; SNES already used six.
- SNES catches up after short stalls instead of rebasing after one late frame.
- Integer window sizes enlarge the composed 320×240 image, including its
  aspect correction and borders; this is not native-console pixel-perfect scaling.
- ARM integer scaling provides 320×240, 640×480 and 960×720 windows. The OS owns
  the offscreen bitmaps; ARM never writes the Workbench screen directly.
- Repaint a window only for a new picture or an Intuition refresh event.
- At 2×/3×, budget more time for presentation and permit up to three/five
  consecutive omitted pictures. Fullscreen and 1× retain the two-picture bound. CPU/APU frames
  are never omitted by this policy.
- Enlarge the temporary bitmap mapping to at most four sections, validating
  pitch, aperture, nonce challenges and restoration. No fixed DDR allocation.
- Normalize library captures to 320×240. High-resolution SNES output remains
  experimental, including its larger window conversion cost.

## Matched audio checks

Two 1,800-frame runs per variant, in reversed order, used Super Mario World
(NES mapper 90) and Chrono Trigger (SNES) on the A4000TX. Both used a 1× window
and the same configured AHI unit 0, Paula Fast 8-bit mono at 8 kHz. Requested
PCM remains NES 32 kHz mono / SNES 32,040 Hz stereo; AHI converts it.

| Test | Previous late AHI links | Scheduling candidate |
| --- | ---: | ---: |
| NES, two runs | 5 + 9 = 14 | 4 + 3 = 7 |
| SNES, two runs | 54 + 40 = 94 | 4 + 12 = 16 |

A late link means the previous AHI request had already completed when its
successor was submitted. It is a useful scheduling signal, not a count of
proven audible dropouts. All PCM samples completed with no device error.
Listening confirmation is still pending; the tests were missed. These results
are not a universal stutter fix or an FPS gain claim.

## Window verification

Native sanitizer tests cover integer pixels, padding, snapshot reduction,
screen fitting, settings persistence, cadence wrap/catch-up and mapping bounds.
Relocated ARM tests compare every pixel at 1×/2×/3× in both 32-bit formats,
including software scanlines, guards, cancellation and mapping revocation.
SNES 512×480 source tests also pass at all three window sizes.

Physical 2×/3× VERIFY runs pass four complete frame checks in each emulator,
with clean register/MMU/cache restoration and owned-memory release. These
68k readbacks are intentionally expensive (the SNES 3× check took about
99 seconds for 180 frames) and are not normal-play performance measurements.
Normal enlarged windows skip those full readbacks; per-frame bitmap ownership
checks remain active. Compact MUI tests pass the four choices and seven pages.

The initial enlarged-window implementation recopied unchanged pictures even
when presentation was omitted. This cost audio time. Removing those redundant
Workbench blits and budgeting larger presentation work addresses that cause.

## Final enlarged-window checks

| Emulator | Window | Core frames | Run time | Late AHI links |
| --- | --- | ---: | ---: | ---: |
| NES | 640×480 | 600 | 10.113 s | 0 |
| NES | 960×720 | 600 | 10.083 s | 0 |
| SNES | 640×480 | 600 | 10.146 s | 0 |
| SNES | 960×720 | 600 | 10.147 s | 0 |

These are short normal runs with existing thumbnails; CPU/APU frames are not
screen refreshes. Larger windows deliberately omit some pictures. A repeated
2× NES run with the initial quarter-frame budget recorded 97 late links; the
final half-frame budget and three-picture omission bound resolved them in the
listed test. This does not establish sustained audio quality in every game.
Pause, screenshot, save/load, 3× → fullscreen → 3× and clean quit pass on both.

Reproduce native checks after `python3 scripts/build_current.py --release-ax`
with `python3 scripts/test_audio_window.py` and
`python3 scripts/test_configured_audio.py`. Relocated ARM checks use
`scripts/test_window_arm.py` with the existing Unicorn environment; SNES also
has `scripts/test_window_hires_arm.py`. These commands do not deploy or run
programs on the Amiga.

[Chrono Trigger fullscreen tearing review](chrono-trigger-display.md): the reported fullscreen path does not use the new window scaler; no tear-free claim.

### Experimental high-resolution detail at 3×

A physical test exposed a 50 ms presentation timeout in the initial 3× hires
path. The normal error cleanup restored the machine. Caching coordinates outside
the pixel loop and calculating the scanline strength once per row removes
per-pixel integer division. Relocated pixel tests still match exactly, and a
360-frame physical run now completes, including a valid screenshot and full
cleanup. The timeout remains 50 ms.

This demanding combination is **not audio-performance qualified**: that run
recorded 81 late AHI links and took 10.049 seconds for 360 core frames. Keep
normal detail for smoother sound at 3×. This limitation does not apply to the
normal-detail window results above. Hires captures now normalize once; actual
snapshot-encoder sanitizer tests compare saved pixels in both formats at all
three sizes.
