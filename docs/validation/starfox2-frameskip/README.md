# Star Fox 2 rendering/audio tradeoff

Same isolated clipped-tile candidate, 800 MHz KEEP, Normal guest CPU, legacy
Super FX timing, fullscreen 320×240, Paula mode 0002000e at 22,030 Hz. Each run
starts from reset and executes 900 guest frames. No gameplay input, periodic
VERIFY, scanlines, FPS overlay or automatic preview write. Production settings
and binaries are unchanged. No synthesis or firmware change.

| Rendering | Elapsed seconds | Core seconds | Rendered / skipped | Starvations |
|---|---:|---:|---:|---:|
| all | 23.574 | 16.914 | 900 / 0 | 222 |
| fixed1 | 20.585 | 13.307 | 450 / 450 | 153 |
| fixed2 | 19.207 | 12.056 | 300 / 600 | 102 |
| auto | 22.922 | 16.133 | 826 / 74 | 224 |
| threshold50 | 17.774 | 9.965 | 186 / 714 | 30 |
| threshold75 | 17.773 | 9.837 | 37 / 863 | 31 |
| threshold90 | 17.795 | 9.837 | 37 / 863 | 31 |

[Executable identity and results](summary.json). Every run completes all audio
samples with zero device errors, restores registers/cache witnesses and releases
owned memory. Final Core1 is idle and Core0 inactive; the retained clock is unchanged.

Threshold 50 reduces starvation events from 222 to 30 (86.5%) and run time from
23.574 to 17.774 seconds (24.6%). This is an existing picture-omission option,
not a new synthesis optimisation. Higher thresholds have almost identical results.
Fixed and automatic modes also retain every CPU/APU frame; only rendering is omitted.

A normal 900-frame NTSC interval is about 15.02 seconds. Even the best result
is slower and still runs out of queued sound. Stage timing also shows individual
long frames, so an average core time below the frame budget does not prove stable
delivery. Larger buffering alone cannot repair sustained insufficient throughput.

Keep Normal CPU and the existing default rendering. Threshold 50 remains an
optional tradeoff when fewer rendered pictures are acceptable; it is not a complete
sound fix. These reset/startup runs do not prove race/gameplay compatibility or
subjective listening quality. No game data or PCM is published.
