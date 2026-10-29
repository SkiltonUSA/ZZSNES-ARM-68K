# AX audio beta release, 29 October 2026

- User authorizes installation on the Amiga, packages and GitHub publication.
- Live bridge healthy, Core1 idle. Installed players match prior release sizes; Version.txt still contains older0.7.1/0.6.1 labels. Upgrade will replace metadata and application files, preserving user data.
- Add explicit normal-play release selector with source version manifests (NES0.8.3-dev / SNES0.7.3-dev). Diagnostic options rejected. Failing CLI test observed before implementation; all five conflicts then pass per repository.
- Retain ordinary regional cadence, user capture/save/input paths, original ACK behavior and three-frame batches. Enable tested register dispatch/cache layout and per-app AX backend with original unit0 fallback; no global AHI or firmware edits.
- Normal release builds match recorded ARM/MMU bytes; launcher, player and SNES FX player carry the same new product version. Guarded ordinary cadence/capture source remains intact.
- Backed up installed launchers/players/icons/version and settings to private local backup; original settings/ROMs/saves are excluded from package payloads.
- Native queue/worker/adapter tests pass; signed installer and manual ZIP equivalence, executable hashes, corruption rejection, prerequisites, icons and user-data exclusions pass.
- First NES180-frame VERIFY run passed four exact pixel checks/all PCM/cleanup; harness then expected a version string in the log, which the host does not print. Removed that invalid log assertion (binary version+uploaded checksum already verified), retained the successful run and continued with SNES rather than rerunning NES.
- Both 180-frame normal-player checks and interactive save/load/capture/display-switch/quit checks passed; all hardware restoration and PCM accounting completed.
- Built and validated both signed offline installers; installed those exact packages on the A4000TX. Existing configuration files remained byte-identical; executable hashes matched the tested release builds.
- The initial CLI launcher check omitted its required 64 KiB stack. The launcher refused safely; both launchers then opened with Stack 65536 (their Workbench icons already specify 65536). Screenshots show the correct versions and existing libraries. Both closed cleanly; configurations still match backup.
- Removed only the private test/install drawer after all runs. Core1 is idle and the bridge responds. Global AHI and saved screenmode preference checksums are unchanged.
- Finalize release records, handoffs, checksums and GitHub prerelease assets. Keep earlier releases available for rollback. No new FPS benchmark or audible-quality claim.
- Final verify-current initially found four stale entries: two README terminology edits (23 October), the previously committed opt-in early-audio header, and the evolving build selector. Reviewed their commit diffs, refreshed only these current-source hashes; historical binaries and source bytes unchanged. Verification then passed.
