# Mesen2 SNES complete-system diagnostic, 10 October 2026

See [integration review and build instructions](../../../mesen_snes/README.md).
Physical A4000TX / ZZ9000 Core1, XX19c/XACP 1.7; unchanged owned-memory loader.
This is a separate development app, not a playable SNES emulator.

| Run | Mean six-frame batch | Mean per frame |
| --- | ---: | ---: |
| Shell | 244,300.5 us | 40.71675 ms |
| MUI System button | 244,317.6 us | 40.71960 ms |

Ten warm samples each, one warmup excluded. ARM timer covers RunFrame only.
Six-frame startup includes the first partial frame. The 24.56 frames/s arithmetic
throughput equivalent excludes RTG display, PCM playback, setup and validation.
It is not game FPS or a real-time claim. NTSC's roughly 16.6 ms budget is unmet.

Actual upstream CPU/bus/DMA/PPU/SPC/DSP run an original Mode 0 tiled LoROM;
no desktop fake rendering or precomputed output. Pixel oracle, CPU/master/SPC
cycles, silent PCM counts and signatures agree on native, A9 model and hardware.
SPC runs its upstream IPL waiting for upload; DSP voices remain silent. Thus
these results do not validate audible playback or realistic game workload.

Native ASan/UBSan: 60 frames; integer clocks exactly match floating reference.
A9 model: two load addresses, corrupt used-palette rejection, cancellation.
Physical: both logs pass active cancel, RET1, all 14 registers, MMU/cache witness
and owned allocation release. Final health shows Workbench only, no Core1 owner,
Roadie/bridge alive and unchanged NES/CPU executables. The MUI screenshot was
captured on the actual Amiga. `frame.png` is the native reference output.

`build.json` pins upstream, compiler settings and source/output hashes.
`integration.patch` records generated upstream adaptations. Build artifacts and
original fixture can be regenerated; no commercial ROM is included here.
The UI executable MD5 is d7235d37b73138b770e01bdf44fff3cf; the System executable
MD5 is 0c3b1311be30a1b368b36c2323d7a2bd. Both were verified after transfer.

Next: profile CPU/bus, PPU and SPC/DSP separately while preserving signatures;
add audible and broader graphics fixtures before display, ROM library or states.
