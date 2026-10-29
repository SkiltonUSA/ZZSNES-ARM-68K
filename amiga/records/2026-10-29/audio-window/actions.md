# Work log

1. Read the installed routing and game logs. Kept configured AHI unit 0 and
   global audio/screen preferences unchanged.
2. Reproduced the SNES short-stall cadence reset in a native test, then kept
   short catch-up time. Raised NES startup prebuffer and enabled batch-ready
   audio notification. Native worker/queue failure and cleanup tests pass.
3. Ran two matched 1,800-frame physical comparisons per emulator and variant.
   Report late links separately from audible dropouts and FPS.
4. Added scaled window/session/profile choices, ARM integer output, bounded
   temporary mappings and logical snapshot reduction. Native sanitizer tests,
   relocated ARM pixel/guard tests and compact physical MUI tests pass.
5. Completed physical 2×/3× full-frame comparisons. Kept expensive large
   readbacks in explicit VERIFY mode; ownership checks remain mandatory.
6. Normal large-window tests exposed repeated blits of unchanged pictures.
   Removed them. A first 3× candidate still missed audio deadlines; added a
   larger bounded presentation allowance. A repeat exposed insufficient 2×
   allowance too. Final 600-frame enlarged-window checks show zero late links
   in these short runs. No claim across all games or of listening acceptance.
7. Passed pause/snapshot/save/load/fullscreen/return/quit lifecycles. Reviewed
   enlarged/hires captures and removed a legacy second reduction at the caller.
8. Reviewed the Chrono Trigger fullscreen tearing report separately: saved
   settings use normal-resolution fullscreen, so the new window scaler is not
   involved. Previous blanking-gate experiments had timed out and stay withdrawn.
   No firmware or Workbench mode change; no tear-free claim.
9. The added physical hires-3× test timed out on its first presentation and
   returned safely. Cached coordinate division and per-row dim strength outside
   the pixel loop. All relocated hires comparisons pass; the 360-frame retry
   captures and exits correctly, but still records 81 late links. Documented
   the performance limitation; did not loosen the timeout or claim smooth audio.
10. Actual snapshot-writer sanitizer tests compare the saved file against the
    unscaled source in both formats and all three sizes. All pass.
11. Installed matching launchers and players after checksum verification,
    preserving prior executables and confirming all three settings files plus
    global AHI/screenmode checksums. Published releases remain unchanged.
12. Removed only the private SD032G:Dev/ZZSound1029 test drawer after copying
    evidence locally. Core1 is free and the bridge responds. Retained installed
    rollback executables and local build/test inputs. No user ROM/save cleanup.
