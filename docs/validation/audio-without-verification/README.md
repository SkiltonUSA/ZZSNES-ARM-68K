# SNES audio measurements without periodic verification

The same isolated clipped-tile candidate was run at the retained 800 MHz setting,
Paula AHI mode `0002000e`, actual mixer rate 22,030 Hz, 900 guest frames from reset,
Normal guest CPU, all core frames rendered, no scanlines or FPS overlay.
Chrono Trigger, Super Mario World and Doom used windows; Contra III, Stunt Race FX
and Star Fox 2 used fullscreen. No gameplay input was sent.

`VERIFY` is absent. The five previously tested games reuse their private preview
files, so no automatic screenshot is written. Star Fox 2 creates one new preview.
Original media/SRAM and production settings are unchanged. All six runs complete
with zero device errors, restored registers/cache witnesses and memory released.
Public/installed 0.7.12 is unchanged.

| Game | Run seconds | ARM core seconds | Queue starvations |
|---|---:|---:|---:|
| chrono | 17.372 | 5.674 | 0 |
| contra | 17.153 | 3.757 | 0 |
| smw | 17.115 | 5.822 | 0 |
| stunt | 17.220 | 5.503 | 0 |
| doom | 17.133 | 6.515 | 0 |
| starfox2 | 24.289 | 16.913 | 222 |

[Raw results and executable identity](summary.json).

The earlier Chrono diagnostic run spent 32.439 seconds in presentation post-work;
this run spends 2.015 seconds. Core time is nearly unchanged (5.673 seconds).
Periodic verification compares pixels on the 68k every 60 frames and produced
multi-second stalls in the earlier trace. Those diagnostic starvations must not
be described as normal-play audio failures. This is removal of test overhead,
not a new production optimisation or claimed game-FPS gain. The prior run also
created its preview, so the elapsed difference is not attributed solely to one
code path.

Star Fox 2 remains different: core execution alone averages 18.79 ms per guest
frame over this interval, above the nominal 16.69 ms NTSC budget. Rendering and
host servicing add cost. Its new preview adds a one-time interruption, but does
not explain the sustained core workload. Existing frameskip options require a
separate controlled comparison before recommending a setting.

Zero starvation in a short startup run does not establish audible quality or
interactive gameplay/scrolling correctness. The known special-chip gameplay
and sound acceptance work remains open. No synthesis, firmware or clock change
was made in this investigation.
