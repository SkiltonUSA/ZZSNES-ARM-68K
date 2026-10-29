# Experimental per-app AX audio

Low-level AHI backend for the recorded register-experiment build.
The 0.7.3 beta enables AX automatically, with AHI unit 0 as fallback. No global preferences are edited.

- `ax_queue.h`: interrupt/task buffer ownership and bounded FIFO positions.
- `ax_stream.h`: mode discovery, AHI controller, callbacks and resource lifetime.
- `ax_worker.h`: existing PCM-ring integration and original-worker fallback.
- `probe.c`: standalone silent mono/stereo hardware probe.
- `test_queue.c`, `test_worker.py`, `test_stream.py`: native sanitizer checks.

Build the normal beta with `--release-ax`. For controlled tests, use
`--presentation-test serial --ax-audio-test` plus the recorded
register-build flags. Run `python3 scripts/test_ax_audio.py` from the repository.
See [results and limits](../../docs/ax-audio.md). No automatic deployment.
