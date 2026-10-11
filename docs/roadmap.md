# Next steps after v0.2.0-dev

The MUI library, per-ROM thumbnail saves and fullscreen stereo player are
implemented. The bordered 600-frame run measured 52.447 displayed FPS.

1. Audio and frame pacing: reduce late AHI links and target steady 60 FPS;
   distinguish core time, presentation and scheduling before optimizing.
2. Longer real gameplay, joystick acceptance and save/load during levels.
3. Standalone battery SRAM persistence, with verified atomic writes.
4. Broader ROM/video-mode coverage and explicit unsupported-chip reporting.
5. Subsequent releases with measured improvements and retained working builds.

Mesen remains a separate reference path. No untested FPGA, NEON or direct-AX
acceleration is implied. The v0.2.0-dev archive and source backup complete the
current packaging milestone; source backup excludes commercial ROMs/states.
