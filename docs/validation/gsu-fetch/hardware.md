# Hardware measurements

Star Fox 2, 900 guest frames per fresh process, Normal CPU, fullscreen, scanlines off,
Paula AHI mode `0002000e`, 22,030 Hz output, ARM 800 MHz KEEP. Eight runs total;
each experiment uses reference/candidate/candidate/reference order. Automatic
capture and periodic pixel verification are absent apart from the existing first-frame
check. Core0 AGA service is inactive. No user input or listening assessment.

| Experiment/run | Core time (s) | Wall time (s) | Starvations | Silence queued |
|---|---:|---:|---:|---:|
| fetch-0-reference | 16.489238 | 23.857372 | 224 | 334 |
| fetch-1-candidate | 16.878345 | 24.018859 | 228 | 332 |
| fetch-2-candidate | 16.877270 | 23.829179 | 230 | 332 |
| fetch-3-reference | 16.486671 | 23.531478 | 220 | 307 |
| fetch-loop-0-reference | 16.488917 | 23.505298 | 223 | 310 |
| fetch-loop-1-candidate | 16.379820 | 23.300572 | 220 | 305 |
| fetch-loop-2-candidate | 16.380667 | 23.507541 | 220 | 309 |
| fetch-loop-3-reference | 16.488630 | 23.458264 | 226 | 313 |

The mask candidate increases mean core execution time by **2.36%** and is rejected
for production. The separate-loop candidate reduces mean core execution time by
**0.66%**. Its wall times overlap the reference range, and audio starvation remains
high (220 events in each candidate run versus 223/226 in the matched references).
This is a small measured core-time improvement, not an established gameplay FPS
or audible-continuity improvement. Keep it opt-in pending broader matched gameplay
and listening checks; the released player remains unchanged.

All eight runs complete 900 frames with exit=0, display fault=0, audio error=0,
register/cache restoration and owned-memory release. Sampled guest core hashes at
frames 1 and 600 agree. Final status confirms Core1 idle and Core0 AGA inactive.
These samples do not constitute full-frame hardware equivalence; exact replay was
checked separately in native and ARM instruction-model tests.

The pipeline experiment changes only `core/src/fxinst.c` among recorded core inputs.
The generated payload also changes, as expected. The mask experiment additionally
changes `fxinst.h`, `fxemu.c` and `fx_state.h`. See [identities](builds.json).

Results apply to this bounded Star Fox 2 sequence at 800 MHz, not all Super FX
games or 666 MHz operation. Longer/later gameplay remains outstanding.
