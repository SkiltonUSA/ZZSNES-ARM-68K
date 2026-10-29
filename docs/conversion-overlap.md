# Pixel conversion during a pending flip

The overlap works, but this implementation does **not improve overall frame
performance**. Keep the existing scheduling as the default. The experiment is
available as a separate test build; installed players and release packages are
unchanged.

## What changed

The original player already emulates the next frame while the previous deferred
PAN is pending. It then drains that PAN before converting pixels. This experiment
also converts pixels into the next free slot while the old PAN is pending.

- Pin the P96 bitmap through conversion. An old flip ACK updates slot ownership
  but cannot unlock memory while ARM writes the next slot.
- Reuse the existing lock's geometry; never lock the same bitmap recursively.
- After the PRESENT token/hash checks and optional readback, drain the old PAN,
  release the render pin, and unlock for keyboard and joystick handling.
- Reacquire the bitmap before queuing the new PAN. Reject changes in its address,
  pitch or pixel format; never submit a stale address after relocation.
- Copy PCM before PRESENT replaces the reply metadata. Only a free audio queue
  slot may bypass the initial drain. The sole producer cannot lose that slot to
  another producer; a full queue still drains before entering its input/wait path.
- A failed/unfinished render retains its lock for reset-before-release. Normal
  display switching occurs only after completed ARM work and drained PAN.

The ARM payload, MMU headers, original ACK polling, three-frame audio batching,
framebuffer bounds, nonce checks and single-Core1 contract are retained. This is
CPU rendering into an owned framebuffer, not a newly introduced DMA engine.

## Hardware comparison

A4000TX / ZZ9000, 29 October 2026. Mario Bros. on ZZNES and Super Mario World on
ZZSNES. Each console ran reference–overlap–overlap–reference, 660 frames per run;
the first 60 warmed up the run and the following 600 supplied the timing sum.
Every emulated frame was presented. Periodic readbacks and automatic snapshots
were disabled for timing; the initial verification remained enabled.

| Console | Reference, 600 frames | Overlap, 600 frames | Extra time |
|---|---:|---:|---:|
| ZZNES | 10.5794 s | 10.8584 s | 2.64% |
| ZZSNES | 10.9938 s | 11.1547 s | 1.46% |

These are means of two runs per variant, not a broad game benchmark or a claim
about the frame rate of a game's internal animation. Variation is visible in the
raw runs; no useful improvement was established. The additional locking and
changed host/audio scheduling are candidates for further profiling, not a proved
single cause of the regression. Moving wait time into presentation must not be
reported as eliminating that wait.

All eight timing runs completed 660 flip acknowledgements, one exact initial
pixel readback, all queued PCM samples, register/MMU/cache restoration and owned
memory release. AHI late-link counters were not zero: NES reference 127/123,
overlap 135/120; SNES reference 77/101, overlap 84/129. Completed PCM does not
prove continuous or perceptually clean sound; no listening test is claimed.

Separate 120-frame correctness runs passed readback on frames 1, 60 and 120 for
both consoles. Instrumentation recorded 106 NES and 87 SNES conversions beginning
with an old PAN pending, and the same number of ACKs while the render pin was held.
This establishes the intended overlap, not optical tear-free scanout.

Both consoles also passed pause/resume, fullscreen → window → fullscreen and
clean quit after waiting for the player's first-frame readiness marker. Two early
NES automation attempts sent keys during initialization; those logs show only
one mode switch and are retained as incomplete UI checks. Core1 ownership alone
does not mean a player is ready for input. All 14 physical runs ended with full
PCM completion and clean restoration/release. These short tests do not resolve
the previously recorded NES freeze or establish long-duration stability.

## Checks and reproduction

Native tests exercise the actual staged fullscreen functions: ACK wrap during
conversion, geometry reuse, relocation rejection, timeout/foreign-ACK cancellation,
normal unlock and window fallback. The old retirement code fails the held-render
assertion when used in the proposed overlap schedule; that does not establish a
bug in its existing serialized schedule. Audio tests exercise queue-index wrap,
nonwaiting copies under lock, unlocked full-ring waits and the sample-count bound.
Existing sanitized settings/scanner and relocated ARM pixel/SIMD checks also pass.

Fresh integrated builds reproduce the tested host/fullscreen sources and exact
ARM payload/MMU headers. Audio preprocessing matches too; the source adds an
inactive `EM_AUDIO_EARLY` option which is forbidden in this experiment.

Use the repository's normal toolchain environment, then build **both** variants:

```sh
python3 scripts/build_current.py --expanded --jit-registers --presentation-test serial
python3 scripts/build_current.py --expanded --jit-registers --presentation-test overlap
```

`--presentation-test` requires the isolated register experiment and rejects
combinations with early audio or throttled ACK polling. It creates separate
`-present-serial` and `-present-overlap` output drawers. It forces every-frame
presentation and disables periodic verification/automatic capture. Use bounded
`--frames 660` runs in a private drawer; it is not a normal-play release preset.
Builds never connect or deploy automatically.

Run host tests against the complete overlap player stage:

```sh
python3 scripts/test_conversion_overlap_host.py . PATH_TO_PLAYER_STAGE
python3 scripts/test_conversion_overlap_audio.py PATH_TO_PLAYER_STAGE
```

Raw logs, patches, payload/build hashes, test output and actions are in
[`amiga/records/2026-10-29/conversion-overlap/`](../amiga/records/2026-10-29/conversion-overlap/).
