# Output-quality investigation

- Reproduced the row-loss mapping with a consecutive-source-row check before changing it. Reviewed FCEUX SDL, modern Snes9x SDL/APU and Xanxi PicoDrive source at the pins in `docs/output-quality.md`.
- Read the live AHI configuration: unit 0 was Paula Fast 8-bit mono, 8 kHz. Kept global audio and Workbench preferences unchanged.
- Added per-app output selection and callback-buffer ownership tests. An initial standalone diagnostic queried AHI before opening its device and raised a software failure. Suspended it, corrected the call order and added a valid-AHI-base assertion. Corrected mono/stereo probes completed.
- Corrected SNES source-row mapping and high-detail parity. Native moving-pattern tests and relocated A9 scalar/NEON comparisons pass; no core/ROM/firmware timing changes.
- Compared configured output with per-app 22.05/28 kHz. Six-frame buffering still recorded shortages; tested a nine-frame prime with twelve producer slots.
- A test-harness filename exceeded the Amiga short-name limit and lost one diagnostic log. Shortened names and reran. Increased the harness allowance for initialization separately from timed emulation; a run that outlasted the old allowance later completed cleanly. Neither harness event is counted as an emulator pass without its retrieved teardown log.

Physical results, installation and release checks follow in the accompanying report/logs. Pixel-model results do not establish audible quality or tear-free physical scanout.

- Final route checks covered Auto, configured unit 0 and explicit AX. A concurrent diagnostic read caused a bridge-poll timeout; the retrieved player log nevertheless completed with exact PCM accounting and clean teardown. Subsequent hardware operations were serialized.
- Pause/capture/save/load and 3x/fullscreen/3x switching complete on both final players. NES records five starvation transitions and SNES one during this interactive pause/switch sequence; these are lifecycle checks, not uninterrupted-audio qualification.
- The backup harness initially treated an absent optional Controls.cfg as required. It stopped before installation; resumption records absent settings and verifies existing rollback copies instead of overwriting them.

- Compact UI selector bounds pass. The scripted scroll screenshot initially retained stale MUI pixels; an explicit diagnostic redraw produces the visible audio row. This is a probe-only change, not a production layout change.

- Both signed installer packages completed; installed executable hashes match. Original library/session files are unchanged. First launcher exit creates Audio.cfg with Auto=0, which the post-install harness now explicitly checks. Other absent optional settings remain absent. No global AHI or screenmode changes.

- An ARexx quit during the installed SNES launch check timed out. The launcher remained responsive to a later QUIT; no process kill or reboot was used. The follow-up checks the actual closed port and saved settings rather than treating a sent command as successful shutdown.
