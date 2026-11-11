# Super FX audio: upstream routine review

Reviewed 30 October 2026 against the actual generated 0.7.11 ARM core and host.
This is source analysis; no player, firmware or saved settings were changed.
Pinned revisions and retrieved-file hashes are in
[review records](../amiga/records/2026-10-30/fx-audio-review/).

Super FX runs cartridge computation/graphics. The separate SPC700 and DSP produce
SNES sound. Heavy GSU execution can delay the host receiving audio, while incorrect
guest scheduling can also change when game code sends sound commands. These are
different failure modes. Our [physical measurements](starfox2-audio.md) demonstrate
delivery starvation, but do not establish audio accuracy against original hardware.

## Routine comparison

| Implementation | Super FX scheduling | Audio execution and delivery |
| --- | --- | --- |
| ZZSNES 0.7.11 | Star Fox 2 uses legacy `FxEmulate(~0)`; the title is outside the bounded `WinterGold` list | SPC execution is interleaved with CPU/HBlank events; final PCM mixing/upload happens after `S9xMainLoop()` returns for a frame |
| Snes9x | Per-scanline instruction budgets, or optional instruction/memory cycle costs | APU catches up on CPU audio-port access and scanline ends; DSP/resampler makes samples available in blocks |
| Snes9x GX | Bounded per-line GSU budget with clock multiplier | Timestamped SNES_SPC execution, sample-ready callback, prefilled DMA ring and queue-level rate correction |
| Snes9x-Z | Same two GSU scheduling approaches as reviewed Snes9x | The reviewed APU bridge is essentially the same; no separate FX sound workaround found there |
| MesenCE | GSU runs toward a master-clock target | SPC has its own master-clock-derived target; completed audio is submitted at frame end, with host resampling/latency control |

### Snes9x

[`S9xSuperFXExec`](https://github.com/snes9xgit/snes9x/blob/1bcc369e89f08243e0a462882fb1f3e42e51de3a/fxemu.cpp#L145)
selects a bounded instruction budget or the cycle-cost mode. The latter accounts
for cache, memory and multiply costs. The HBlank scheduler then services audio.

[`S9xAPUExecute` and `S9xAPUEndScanline`](https://github.com/snes9xgit/snes9x/blob/1bcc369e89f08243e0a462882fb1f3e42e51de3a/apu/apu.cpp#L271)
convert CPU clocks into SPC time with a retained fractional remainder, synchronize
the DSP and notify the frontend when samples accumulate. Audio-port reads/writes
also catch the APU up before the access. `S9xMixSamples` drains a resampler; it
does not synthesize a whole video's worth of sound from final DSP registers.

`S9xUpdateDynamicRate` adjusts the resampling ratio using output buffer occupancy.
`SoundSync` principally prevents production overrunning available buffer space;
it cannot generate samples when emulation is too slow.

Our [Timed experiment](timed-superfx.md) already adapts this exact Snes9x revision's
GSU cost model. The recent Normal/Paula boot test was slower and still starved.
It must not be presented as a newly discovered or proven sound fix.

### Snes9x GX

[`S9xAPUEndScanline`](https://github.com/dborth/snes9xgx/blob/9a48bc2a2995aa3715cc59986e50319d443b0798/source/snes9x/apu/apu.cpp#L510)
advances SNES_SPC, retaining partial-clock remainder, and makes available samples
visible through its callback. CPU audio-port accesses pass their emulated timestamp.

The reviewed [OGC audio callback](https://github.com/dborth/snes9xgx/blob/9a48bc2a2995aa3715cc59986e50319d443b0798/source/drivers/ogc/OgcEmulatorAudio.cpp#L92)
uses a 16-slot ring, 2,048-byte buffers (512 stereo frames), four queued buffers
before starting DMA, and a 12-buffer queue ceiling. Occupancy below four or above
eight enables a correction; it returns to neutral at six. Resampling multipliers
0.995/1.005 provide small corrections without constantly toggling at one threshold.
Buffers are flushed for that platform's DMA before publication.

This is a useful control policy, not portable Amiga device code. The DMA callback
advances ring slots without our explicit occupied-slot/starvation scheme; retain
our lifetime and underrun safeguards rather than transplanting it literally.
Do not copy its approximate millisecond comments without checking sample units
and actual playback rate.

### Snes9x-Z

Compared [`apu/apu.cpp`](https://github.com/andrezaiats/snes9x-Z/blob/c3c6710f9efcfec4c2c513baeb602cf17dd7d909/apu/apu.cpp)
against the pinned Snes9x file. Apart from whitespace, the difference is a
`timer_resync()` call in state loading. It is not a steady-state FX audio
optimization. Its GSU path also offers bounded legacy and costed execution.
Do not attribute the fork's broader rendering gains to an FX sound routine.

### MesenCE

[`Gsu::Run`](https://github.com/nesdev-org/MesenCE/blob/4b8a196e299b3de297b204fd1208d2981562c89b/Core/SNES/Coprocessors/GSU/Gsu.cpp#L127)
runs until the emulated master-clock target is reached or the GSU stops. This is
frequent guest-clock synchronization, not a dedicated host audio thread.
[`Spc::Run`](https://github.com/nesdev-org/MesenCE/blob/4b8a196e299b3de297b204fd1208d2981562c89b/Core/SNES/Spc.Instructions.cpp#L7)
independently catches SPC execution up to its clock-ratio target. Audio-port
accesses synchronize SPC, with explicit handling of when CPU writes become visible.

[`Spc::ProcessEndFrame`](https://github.com/nesdev-org/MesenCE/blob/4b8a196e299b3de297b204fd1208d2981562c89b/Core/SNES/Spc.cpp#L392)
sends collected DSP samples to the mixer. Thus frame-end submission alone is not
evidence of a bug; the important differences include synthesis chronology and
available host processing time.
[`SoundResampler`](https://github.com/nesdev-org/MesenCE/blob/4b8a196e299b3de297b204fd1208d2981562c89b/Core/Shared/Audio/SoundResampler.cpp#L29)
uses measured latency, a dead band and small gradual corrections. Explicit slow
emulation also has a separate playback-rate adjustment in `SoundMixer`; that
changes playback speed/pitch and is not full-speed recovery.

## What should change in ZZSNES?

1. **Measure a lightweight bounded GSU experiment.** A per-line instruction budget
   is cheaper than charging every instruction's modeled bus costs. Treat this as
   a compatibility/timing experiment: arbitrary clipping can change game speed,
   CPU/GSU handshakes and sound-command timing. Validate STOP, IRQ, GO/R15 writes,
   saves and reference behavior; do not simply add Star Fox 2 to a title hack.
2. **Keep reducing actual core work.** Even the combined tile candidate averages
   21.23 ms per NTSC core frame versus a 16.69 ms budget, before host output work.
   GSU dispatch/memory helpers and `DrawBackgroundOffset` remain measured targets.
3. **Then test queue feedback with hysteresis and bounded resampling.** GX's policy
   is a good reference for small drift and occasional bursts. We already prebuffer:
   explicit Paula/AX starts with nine emulated audio frames, about 150 ms NTSC;
   configured AHI uses six. Increasing these blindly adds delay. A ±0.5% correction
   cannot absorb our sustained shortfall.
4. **Investigate incremental PCM separately.** Our `audio_upload_samples()` mixes
   after the frame; `S9xGenerateSound()` is empty. Moving that mixer into arbitrary
   slices can change envelopes, DSP-register timing and sample counts. Earlier
   delivery needs validated timestamped synthesis and bounded shared-buffer
   publication, not just more frequent copies or calling AHI from Core1.

Reference PCM comparisons should align the same ROM/boot/input trajectory;
different DSP implementations and rates need more than an exact byte comparison.
The existing exact-payload model comparison establishes transport consistency,
not original-console synthesis accuracy. No new performance or audio fix is
claimed from this source review.
