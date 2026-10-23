# ZZNES mapper and PAL integration

11 October 2026. This extends the existing FCEUX-based ZZNES player, not the
SNES core. It preserves the accepted XX19c/XACP display, audio, allocation
and ownership lifecycle. No firmware or OS library change is required.

## Regional cadence update — 12 October 2026

After the user reported the ZZSNES interactive scheduler was “so much better”,
the same policy was adapted to ZZNES's existing FCEUX host. Each CPU/PPU/APU
frame still runs and all 32 kHz mono PCM is retained. The host follows the NES
regional deadline and may omit a late completed picture, at most twice in a
row. It permits two frames of pacing lead and rebases after a stall over 250 ms.
A zero deadline means reset, including after pause, state load/save or snapshot.

The ARM ELF/payload, three-slot fullscreen protocol, audio worker (eight slots,
three-frame prebuffer), ROM support and save-state ABI are byte-identical to
the accepted compatibility build. It does not import the SNES four-slot layout,
32,040 Hz stereo rate or six-frame audio prebuffer. This is host scheduling work,
not additional ARM emulation optimization or a firmware change.

Physical comparisons ran 1,200 identical scripted frames per variant:

| Measurement | Contra NTSC control | Contra cadence | Asterix PAL control | Asterix cadence |
| --- | ---: | ---: | ---: | ---: |
| Whole-loop emulation fps, including startup | 53.652 | 57.570 | 48.150 | 48.293 |
| Steady emulation fps, frames 600–1,200 | 55.908 | 59.893 | 49.920 | 50.005 |
| Omitted pictures | 0 | 54 (4.5%) | 0 | 1 (0.083%) |
| Audio gap indicators | 254 | 15 | 1 | 0 |
| Host ACK intervals >20 ms | 130 | 38 | 222 | 218 |

The Contra candidate's steady presentation rate was 58.395 pictures/second;
emulation FPS is not monitor refresh or presentation rate. Its final 600
frames added one gap indicator. Both Contra variants completed exactly 638,901
mono samples and both PAL variants 767,856. Sampled game hashes matched between
control and candidate for each ROM. Every requested flip completed; all runs
reported AHI error 0, stopped audio worker, checked MMU/register/witness
restoration and clean exit with owned memory released.

These are short pairs rather than five-minute runs or full-game tests. The
remaining NTSC gap indicators are not claimed eliminated, and counters do not
measure audible dropouts or tear-free scanout. PAL ACK intervals naturally lie
near the 20 ms threshold, so its counts cannot be directly compared to NTSC.
Native ASan/UBSan tests cover both NES periods, timer wrap/reset, bounded
omissions and 200,000 handoffs using the actual NES three-slot planner.

Source adaptation: `scripts/fceux_cadence.py`, using the shared tested policy
in `amiga/pocketsnes/library/cadence.h` and `pace_target.h`. The host-only builder
requires the original verified `.context/amiga/fceux-compat` build cache. It
hash-checks that cache's executable/ELF and checks the unchanged ARM, fullscreen
and audio components. It does not download a different core.

```sh
PYTHONPATH=. python3 scripts/build_fceux_cadence.py --variant Release
PYTHONPATH=. python3 scripts/build_fceux_cadence.py --variant Baseline --variant Cadence --frames 1200
.tools/amiga-venv/bin/python scripts/run_fceux_cadence.py --series unique-name Baseline-1200 Cadence-1200
python3 scripts/analyze_fceux_cadence.py /path/to/completed.log
```

Production output: `.context/amiga/fceux-cadence/Release/ZZFceux-ui`. The Release
variant retains real input, normal unlimited play, save slots and thumbnails;
it does not include diagnostic scripted input or disable state transactions.
The old compatibility build command below creates the earlier scheduler; apply
this host-only build afterward for the current cadence player.
Evidence: `amiga/records/2026-10-12/zznes-cadence/`. No ROMs, saves or game artwork
are included in the versioned evidence.

The production player was installed at `SD032G:Dev/ZZNes-Borders/ZZFceux-ui`
after Castlevania III MMC5 passed physical thumbnail capture, paused save/load,
continued playback beyond frame 300, clean quit and a fresh-process state
resume. Tests used unique private state/snapshot paths. Installed player MD5:
`1d71b0627bce977fe1703db69dd9cc8a`. Previous executable retained as
`ZZFceux-ui.pre-cadence-20261012` (MD5 `3b3e008483c21c6b0bb6704bd4bc3207`).
The matching MUI library and Library.cfg were verified unchanged. The existing
normal icon uses the update. User visual feedback on ZZNES remains pending.

## Implementation

The ARM payload now includes upstream MMC1 (mapper 1), UNROM (2), MMC3 (4)
and MMC5 (5), alongside the existing NROM (0) and mapper 90. CHR RAM is
supported for the admitted mapper 1/2/4 cartridges. Mapper RAM and registers
participate in save states. The upstream cartridge CRC is computed from
PRG+CHR data so the existing mapper-specific RAM sizing remains effective.

Mapper sources are pinned, hash-checked copies from
[Rezzy-dev/fceux](https://github.com/Rezzy-dev/fceux), commit
`4d01530b6689b06a6d7cf0679b63625e3270ae9a`, in
`amiga/fceux_player/compat/vendor/`. Their original licence notices are
preserved. `scripts/fceux_compat.py` applies the platform integration to
generated copies; it does not modify the vendor originals.

Automatic PAL selection uses the iNES PAL flag or a case-insensitive
`(Europe)`, `(Australia)` or `(E)` tag in the ROM basename. Directory names
do not select PAL. This explicit filename fallback covers the supplied old
dumps whose headers omit region information; it is not a general cartridge
region detector. Untagged ROMs with no PAL flag default to NTSC.
ROM bytes are never modified.

PAL selection configures the CPU/PPU timing, colour emphasis and APU/filter
clock, including the ARM integer audio constants. The host and ARM exchange
and check the selected region. Frame deadlines are 19,998 µs PAL and
16,639 µs NTSC, approximating this core's console clocks. These are scheduling
targets, not a guarantee of achieved frame rate or monitor refresh. The RTG
display mode remains unchanged.

The save-state transfer capacity grows to 122,880 bytes and moves to offset
`0x202000` within the existing dynamically mapped 4 MiB allocation, after the
maximum ROM and before BSS. These are allocation-relative offsets, not static
DDR addresses. State headers now store region in the previously zero field;
old NTSC states remain compatible. A mismatched region is rejected before
restoring state. A bounded 128 KiB arena holds cartridge allocations for the
single-ROM worker lifetime; the launcher frees the entire owned allocation
only after normal ARM return.

## Build and local verification

Use `python3 scripts/build_fceux_compat.py` for the matching player and MUI
library. It extends the established generated border/snapshot frontend in
`.context/amiga/game-borders/nes` and original worker staging in
`.context/amiga/fceux-player`; those existing build stages and pinned toolchain
are prerequisites. The older build commands do not include this integration.
Outputs are under `.context/amiga/fceux-compat/`.

`scripts/test_fceux_compat.py` builds native, ARM-style integer-audio and
sanitized native tests. All eight supplied ROMs passed 600 frames and exact
30-frame video/audio/cycle save replay in all three variants. Original
Mario Bros and mapper-90 Super Mario World also passed. Native state tests
add four 120-frame replay windows, malformed-state rejection without mutation,
and cold loading of saves produced by the previous build. Sanitizer coverage
excludes the existing upstream bank-pointer array-bounds/pointer-overflow
checks; these results do not establish the absence of every memory defect.

`tests/amiga/fceux_player/check_compat_arm.py` runs actual ARM instructions
against the native oracle, checks aligned accesses and save replay, and tests
two relocation bases for mapper 4/5. These tests passed for all eight supplied
ROMs; they do not replace physical cache/coherence validation. Cartridge
rejection/region/deadline/buffer bounds, the existing audio worker and all
16,384 MMU allocation alignments also passed. The packed payload is 247,308
bytes with 498 relocations, within the unchanged layout limits.

## Physical cartridge checks

All eight supplied ROMs completed 600-frame runs on the A4000TX. Each run
acknowledged 600 framebuffer flips, verified the first framebuffer readback,
completed every queued audio sample, restored the checked ARM/MMU registers
and returned through RET1 before releasing its allocation and coordination port.
The scripted Start input was held for ten frames. The native test's Start
sequence is one frame later, so its final gameplay hash is not a physical
oracle for this script.

| ROM | Mapper | Timing | Observed test FPS | Audio gap counter |
|---|---:|---|---:|---:|
| Airwolf (Japan) | 1 | NTSC | 49.44 | 145 |
| Asterix (Europe) | 2 | PAL | 45.99 | 0 |
| Aussie Rules Footy (Australia) | 1 | PAL | 46.00 | 0 |
| Castlevania III (USA) | 5 | NTSC | 50.50 | 134 |
| Contra (USA) | 2 | NTSC | 50.82 | 127 |
| Contra Force (USA) | 4 | NTSC | 48.82 | 157 |
| Donkey Kong 3 (World) | 0 | NTSC | 50.33 | 137 |
| Hammerin' Harry (Europe) | 4 | PAL | 45.99 | 0 |

These elapsed-time measurements include the bounded test's setup/teardown
and instrumentation and are not full-speed claims. NTSC audio starvation is
still visible despite successful completion of submitted buffers. PAL runs
produced about 383,921–383,927 samples, versus 319,446–319,455 for NTSC, as
expected from the different console clocks over 600 emulated frames.
Further pacing/audio work remains separate from cartridge admission and
correct region timing.

Asterix PAL and Castlevania III MMC5 also passed physical paused save/load,
snapshot capture, normal quit and a fresh-process state resume using isolated
test slots. Early short automated key injections missed save/load actions;
holding the injected keys, and repeating them for MMC5, produced the verified
operations. Both the first attempts and successful logs are retained. This
does not establish a cause for the missed injected input.

The matching player and library are installed in `SD032G:Dev/ZZNes-Borders`.
Previous binaries are retained as `.pre-compat-pal-20261011`; the 11-game
configuration, ROMs and existing user states were preserved. Player MD5 is
`3b3e008483c21c6b0bb6704bd4bc3207`, library MD5
`c628639da59e85c4392a997a9293d115`. The MUI library reopened successfully.

## Remaining limits

This is a bounded compatibility extension, not all of desktop FCEUX.
Admission remains limited to iNES 1, mapper 0/1/2/4/5/90, power-of-two ROM banks,
up to 512 KiB PRG and 256 KiB CHR, with exact file size. Battery-backed carts,
trainers, four-screen mirroring, NES 2.0 and other mappers remain rejected.
Mapper 0 retains its original 8 KiB CHR-ROM contract; mapper 5/90 require CHR
ROM. There is no manual timing override or automatic detection from game code.
Short deterministic and hardware runs are not full-game compatibility tests.

Physical results and installed hashes are recorded separately under
`amiga/records/2026-10-11/zznes-compat/`; consult session memory for the current
installed version. Private ROMs and saves are excluded from the records.
