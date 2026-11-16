# Combined tile rows: firmware and workload regression

The opt-in `--ppu-test rows` renderer was compared with the unchanged release
on XX19d-RIX / XACP 1.8, with Paula output, normal core timing and 2x windows.

| Workload | Frames/run | Reference core time | Candidate core time | Reduction |
|---|---:|---:|---:|---:|
| Chrono Trigger opening, one pair | 1,200 | 9.206559s | 9.208201s | No improvement |
| Star Fox 2 checkpoint, reference/candidate/candidate/reference | 660 | 9.760529s mean | 9.202348s mean | 5.72% |

All runs finish and retire the same number of PCM frames per workload, restore
register/MMU/cache witnesses, and release memory. Star Fox 2 still starves the
audio device: reference 195/195 observations, candidate 199/159. This variable
result does not establish an audio fix. Chrono Trigger reports no starvation.

Core time is distinct from displayed FPS and wall time. Earlier native
video/PCM/state and relocated ARM comparisons remain the output-correctness
evidence; the hardware `output_hash=811c9dc5` field is not an image hash in this
mode and is not used to claim pixel equality. Installed binaries and releases
remain unchanged. Fullscreen results follow below.

Additional native regression: controller mapping, launcher layout, region
pacing, ROM manager and ZIP input pass. AX worker ordered PCM, buffer reuse,
pause/re-prime and cancellation tests pass, as do per-app AHI discovery and
worker failure cleanup. These mocked API tests do not prove audible AX output.

## Fullscreen follow-up

One reference/candidate pair: 660 frames each; ARM core time
9.690059s → 9.134541s (5.73% reduction).
Audio starvation observations were 138 → 123. Both runs
retired 352,913 PCM frames, passed the display handoff and teardown
checks, and restored register/MMU/cache witnesses. This is additional coverage,
not proof of audible improvement. The experiment remains opt-in.
