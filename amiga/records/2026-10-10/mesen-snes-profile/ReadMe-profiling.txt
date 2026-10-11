ZZSnes-Mesen stage profiling - 10 October 2026

These are separate development diagnostics; close other ARM games first.
Stack 65536
ZZSnes-Mesen-Profile >Profile.log
ZZSnes-Mesen-Profile-O2 >Profile-O2.log

Both use the same original six-frame startup fixture as System test.
No external ROM file is used. No display or audible audio is produced.
Profile checks pixels, CPU/SPC/master clocks, silent PCM and cancellation.
Detailed output includes ARM register/cache/MMU restoration and release.

A4000TX results (ten warm batches each):
Uninstrumented System: 40.698 ms/frame.
Profile baseline: 40.797 ms/frame:
  PPU graphics 20.326 ms (49.82%)
  SPC/DSP 6.420 ms (15.74%)
  CPU/bus/scheduling plus profiler remainder 14.051 ms (34.44%)
PPU speed compiler candidate (-O2): 40.705 ms/frame; no useful gain.
These are simple headless startup results, NOT measured game FPS.
NTSC budget is about 16.6 ms/frame; real-time SNES remains unproven.

profile-build.json and profile-o2-build.json pin these two builds.
Original System/CPU/UI binaries and their metadata remain unchanged.
Next: finer graphics profiling, then CPU/bus scheduling investigation.
