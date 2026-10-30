# Output-quality tests

`r9` / `r9b`: nine-frame priming before the capture-drain addition.
`pw`: warm-preview PAL runs (Asterix / The Chaos Engine).
`pc`: final build, first-time capture, PAL fullscreen and 3× windows.
`er`: final output-route smoke checks. Configured unit 0 uses the original
linked-request API: its callback fields are zero, not evidence of no underruns.
SNES `fx`, `fas`, `ffs`: Stunt Race FX race checkpoint, respectively Off,
Auto and Fixed 1 frameskip. These stress runs remain below real-time audio speed.

The machine's clock is not used to date this evidence. Source frame counts and
monotonic microsecond counters establish run lengths. Silence schedules include
intentional capture/pause/drain periods. No listening or eight-hour soak claim.

Native sanitizer/model tests and physical results are distinct. Hardware runs
check PCM accounting, register/cache/MMU witnesses and owned-memory release.
No ROMs, checkpoints, battery files or private service credentials are included.

An initial AHI diagnostic queried before opening the device and failed; it was
suspended. Corrected probes and a regression test cover the query order. One
reboot is required to clear that suspended diagnostic before normal use.
The emulator executable was not the failing diagnostic.

[Findings, comparisons and limitations](../../../../docs/output-quality.md).
