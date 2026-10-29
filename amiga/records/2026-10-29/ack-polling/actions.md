# Fullscreen acknowledgement polling investigation

- Read host/screen ownership and current handoffs. Existing request loop already overlaps emulation with flip acknowledgement.
- Identified tight register polling as a measurable candidate; added private phase/read/ack-age counters and a 250 us read interval. No bitmap unlock, deferred PAN, timeout or cleanup contract change.
- Wrote and observed the missing-helper test fail, then passed interval/deadline/wrap tests.
- Built private host variants against identical recorded ARM payloads, with complete generated MMU headers.
- Use fixed 660 presentations, 60 warmup/600 measured frames, no recurring pixel audit/capture; original audio scheduling in both variants.
- Profiled phase-specific reads and ACK ages. Most ACKs retire in the blocking drain after ARM emulation; queue-to-ACK age includes overlapped work and is not blocked CPU time.
- Added an opt-in --ack-poll-us 250 build path to each standalone repository. Defaults remain zero. The exported generator produces headers byte-identical to the private tested candidate.
- Actual host-function mock tests pass for ACK wrap, retained bitmap lock, delayed retirement, foreign ACK and timeout cancellation. Interval policy tests cover timer wrap and deadline sampling.
- Began a counter-free repeat. Detected a harness output-directory naming collision: its NES logs overwrote the initial NES profiling logs. Preserved the initial summary and SNES profiling logs, separated output paths, and scheduled a fresh NES profiling ABBA repeat. Superseded NES profiling numbers will not be used as final raw evidence.
- Completed the counter-free ABBA pairs: NES 10.607100 -> 10.555073 seconds per 600 frames (0.49% less); SNES 10.936556 -> 10.985259 (0.45% more). No useful FPS/audio gain established.
- Repeated the NES profiling ABBA and archived it alongside the preserved SNES profile. Pending ACK reads fall 89.67% / 89.82%. Reference ACK age is about 12.4–12.5 ms, but explicit blocking drain is 4.61 / 4.24 ms per frame; most ACKs retire after emulation and before conversion.
- All 20 physical runs completed cleanly; 16 final comparison logs are archived, with the four superseded NES profiling runs excluded from reported evidence. Every accepted run completes 660 flips and PCM with restoration and release checks.
- Removed only SD032G:Dev/ZZPoll1029. Bridge remains responsive, Core1 idle, saved display CRC 9C80FBC5 unchanged. No installed executable, release, firmware, ROM/save or system preference changed.
- Prepared source, tests, reports and exact hashes in both standalone repos. Next investigation is conversion overlap with explicit render/lock ownership, not deletion of the ACK fence.
