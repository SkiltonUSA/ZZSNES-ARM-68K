# Per-application AX output investigation, 29 October 2026

- Read handoffs, source worker and AHI original developer guide/autodocs. Unit255 is low-level access, not a replacement streaming unit number. No global AHI preference edits.
- Added FIFO ownership test first; missing-header failure recorded, then queue implementation passed ASAN/UBSAN.
- Compiled a separate 68k silent probe. Corrected SDK declaration/function-pointer/printf type errors before deployment.
- Preflight: Core1 idle, ENV AHI CRC725CE7DB, saved screenmode CRC9C80FBC5.
- Initial two-buffer mono/stereo probes each complete96000 frames and close. SoundFunc schedules ahead; two immutable buffers cause31silence schedules. Added third buffer and repeated (raw initial evidence preserved).
- Query AHIDB_MaxPlaySamples and allocated mix rate; allocate at least driver minimum, while silence playback length stays20ms. Mode enumerated by registered name, bits/stereo/realtime; no hard-coded mode ID.
- Three-buffer probes completed96000 frames each with63callbacks/3silence schedules, versus91/31 initially. Silence buffer allocated to driver minimum but plays20ms segments. No audible-test claim.
- Actual worker sanitizer checks pass for mono/stereo, partial stop, order/reuse, state drains, fallback, start failure and cancellation. Actual AHI adapter fault injection passes resource/mode/load/query/start failures and teardown ordering.
- Added bounded ring positions/ready flags to avoid unsigned modulo-3 rollover ambiguity. Saw the bounded-position assertion fail first, then pass. Final hardware timing uses this version.
- Two initial120-frame emulator smoke runs pass; final source uses bounded indices and correct backend-dependent SNES feedback capacity. Preserve exploratory logs without treating them as final-build timing.
- Added isolated --ax-audio-test requiring serial register experiment and rejecting conflicting scheduling experiments. Full builds pass; host/backend and recorded ARM/MMU inputs match tested binaries (inactive early-audio directives excluded during comparison).
- Existing native current tests pass; first model check lacked llvm-nm, corrected PATH and reran only failed model checks successfully. Negative CLI test first omitted required base-build flags, then corrected and passed all six rejection cases.
- Eight final ABBA 660-frame runs pass exact initial framebuffer readback, all PCM counts, every-frame presentation, register/MMU/cache witnesses and owned-memory release. Mean FPS: NES57.1846->59.0403 (+3.245%); SNES56.4304->59.0019 (+4.557%). Device and scheduling both change; no isolated-cause or all-title claim.
- Both final candidates pass readiness-gated paused state save/load, window/fullscreen round trip, resume and quit. All PCM accounted for. Twelve emulator runs plus four standalone probes this round; exploratory runs explicitly separated from final evidence.
- Postflight: Core1 idle, bridge alive; AHI CRC725CE7DB and saved display CRC9C80FBC5 unchanged. Removed only private SD032G:Dev/ZZAX1029. Installed players, ROM/save libraries, firmware and startup untouched. No listening test.
- Crapper/dryer/mutator tools are unavailable in this session; no tool-generated complexity/mutation result claimed. Reviewed buffer publication, hook restrictions, bounded indices, fallback and teardown manually and with the above tests.
- Final diff check found one whitespace-only blank line in ax_stream.h; removed it. Recorded build hashes describe the tested pre-cleanup file; executable tokens and behavior are unchanged.
