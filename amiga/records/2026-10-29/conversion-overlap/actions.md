# Conversion overlap — action log

- Read current handoffs, both console agent guides, host, audio and framebuffer code.
- Kept baseline ACK polling and the previously recorded ARM payloads/MMU headers.
- Reproduced the premature bitmap unlock in a native test that injects a previous
  flip ACK during a new ARM render. This is unsafe only if overlap is introduced;
  the original serialized path does not render at that point.
- Added an isolated fixed-presentation transformation: retain render lock through
  old ACK, finish the old flip after conversion, unlock for input, reacquire and
  reject relocation before queuing the new flip. Cleanup unpins after Core1 reset.
- PCM stays before PRESENT. Only the sole-producer free-ring path may skip the
  initial drain; a full ring retains the unlocked waiting/input path.
- Native actual-host tests cover ACK wrap, held locks, reused geometry, relocation,
  timeout, foreign ACK, unpinned ACK and window fallback. Audio tests cover counter
  wrap, bounded metadata and locked nonwaiting versus unlocked waiting paths.
- Fixed two local build-harness issues before deployment: SNES cleanup has an
  additional scanout-gate reset; an instrumentation string needed C newline escaping.
- Cross-built reference, overlap and instrumented verification players for each
  console. The instrumented players use periodic readbacks; timing players do not.
- Existing sanitized settings/scanning tests pass. Their model steps initially
  could not find Python/Unicorn/llvm-nm; reran with the workspace model environment
  and explicit LLVM_NM, both passed. No dependency or source changes were needed.
- Preflight: bridge alive, Core1 idle, saved screenmode CRC 9C80FBC5 unchanged.
  Used only SD032G:Dev/ZZConvert1029, verified uploaded MD5s, private test save paths.
- Two 120-frame physical correctness runs passed three pixel readbacks each;
  old PAN ACKs arrived during conversion 106 times on NES and 87 on SNES.
- Eight fixed-presentation ABBA timing runs completed: mean 600-frame time grew
  2.64% NES / 1.46% SNES. No default/release promotion and no audio-quality claim.
- Fresh full integrated builds pass. ARM payload/MMU headers are byte-identical
  to the measured inputs; host/fullscreen source matches. The newer source audio
  header has an inactive early-wake conditional; preprocessing without that flag
  is identical to the tested header. Added serial/overlap test build choices.
- Two initial NES UI attempts sent keys before the first frame was ready and did
  not establish a return-to-fullscreen result. Both exited cleanly. Core1 port
  ownership is not player readiness. Removed that assumption: delete the old
  private Player.ready marker and wait for a fresh marker before injecting keys.
- With readiness gating, both NES and SNES pass pause, switch to window, resume,
  switch back to fullscreen and quit. All PCM completes, state restores, memory
  is released. No keys remain held. Fourteen physical runs in total end cleanly;
  the first two UI attempts are retained as incomplete automation evidence.
- Native lock/audio tests and fresh serial preparation checks pass. Existing
  sanitizer/model tests pass with the workspace model interpreter and llvm-nm.
- Postflight: bridge alive, Core1 idle, saved screenmode CRC still 9C80FBC5.
  Removed only the private SD032G:Dev/ZZConvert1029 test drawer. Installed apps,
  firmware, ROM libraries, saves and system preferences were not changed.
