# Mesen CPU core: minimal ZZ9000 test

This is **Mesen's actual NES CPU implementation running on physical Core1**,
not a complete NES emulator or a libretro port. PPU, APU, cartridge mappers
beyond the diagnostic's fixed NROM mapping, controllers and display are absent.
The PPU/APU integration callbacks are deliberately empty. Their cost is not
represented in these results. No game/frame-rate claim follows from this test.

Upstream: [SourMesen/Mesen](https://github.com/SourMesen/Mesen), commit
`e1180957b31979d4b82d613af9263fcb9442a685`, GPL-3.0-or-later. Three unchanged
upstream source files and the license are in `vendor/`; hashes are pinned.
The integration code in this directory is also GPL-3.0-or-later.
The build generates a reviewable `integration.patch`, replacing desktop
headers/shared ownership, fixing reset alignment to upstream's deterministic
case, and removing save-state integration. Opcode/address/cycle logic remains
upstream code. No heap or C++ standard library is needed by this isolated test.
Upstream development of this old version has stopped; this is a feasibility
experiment, not a recommendation to distribute the old desktop application.

## Private-cache result, 9 October 2026

**The CPU-only real-time gate now passes.** Three fresh physical launches,
30 measured batches: **3.352 ms median** (3.343–3.379 ms), compared with
99.766 ms in the earlier instruction-cache-only test. This is **29.76x faster**,
96.64% less time, and **4.43x NTSC CPU real time** for this diagnostic workload.
Every launch matched all 8,991 reference states and the final RAM/register
signature. Graphics, sound and game FPS have still not been measured.

| Execution policy / layout | Median per 26,553-cycle batch |
| --- | ---: |
| Earlier I-cache only, MMU/D off | 99.766 ms |
| Private MMU + I-cache, D off; boundary layout | 52.300 ms |
| Private MMU + I/D-cache; boundary layout | 18.891 ms |
| Private MMU + I/D-cache; hot code moved into fully owned pages | **3.352 ms** |
| Time represented at NTSC NES CPU speed | 14.836 ms |

The improvement comes from **the ARM memory policy and executable layout**.
Mesen's instruction implementation and compiler optimization level remain
unchanged. No SDL changes, display transfers, or shared-L2 tuning are involved.
The first physical page is only partially owned by our 64-byte-aligned
allocation and must remain uncached. Padding the entry section moves the hot
CPU interpreter out of that boundary page and into fully owned cacheable pages.

The private map caches only complete pages in the Exec allocation, using
inner write-back/write-allocate and outer noncacheable attributes. Control
mailboxes, translation tables and boundary pages remain noncacheable.
No global/shared L2 maintenance is performed. Read the
[memory policy and teardown](private-memory.md) before reusing this experiment.

All three final launches also passed cancellation during repeated CPU work,
nonce/sequence mailbox echoes, a 1 KiB private-data RAM readback with boundary
canaries, original/restored MMU/control register comparisons, RET1 and exit 0.
The named owner port and temporary Amiga test files were removed. The default
debugger/fractal cache contract remains unchanged; this is an explicit opt-in
for this bounded CPU benchmark, not general cache support for other apps.

```sh
# Mapping/I-cache baseline, with D-cache disabled:
python3 scripts/build_mesen_bench.py --arm --private-mmu \
  --container-command '["podman","--connection","nuflix-converter-root"]'
# Final private-cache test (includes active-work cancellation acceptance):
python3 scripts/build_mesen_bench.py --arm --private-cache \
  --container-command '["podman","--connection","nuflix-converter-root"]'
python3 scripts/analyze_mesen_cache.py
```

Outputs: `.context/amiga/mesen-bench-mmu/` and `mesen-bench-cache/`.
The current `--private-mmu` build includes the final padded layout, whereas
the recorded 52.300 ms intermediate run used the earlier boundary layout.
Records and exact build identities: [mesen-cache evidence](../records/2026-10-09/mesen-cache/).
The next application gate is **CPU+PPU frame correctness and timing**, followed
by sound and display integration. A CPU-only diagnostic is not representative
proof that every game will run at full speed.

## Initial uncached results, 9 October 2026

XX19c / XACP 1.7, A4000TX, Cortex-A9 Core1; 1 MiB allocated by Exec in ZZ9000
Fast RAM, independently challenged mapping, firmware Core0 left running.

| CPU-only workload | All caches off | Instruction cache on |
| --- | ---: | ---: |
| Reference CPU states checked | 8,991 passed | 8,991 passed |
| CPU cycles per timed batch | 26,553 | 26,553 |
| Median batch time, 10 samples | 106.183 ms | 99.766 ms |
| Speed relative to NTSC NES CPU time | 14.0% | 14.9% |

The same workload represents 14.836 ms of NES CPU time. I-cache improves
throughput approximately 1.064x (6.04% less CPU time), but **this execution
configuration fails the real-time CPU gate before adding PPU/APU/display**.
These historical numbers did not establish the maximum performance of the A9.
MMU and D-cache remained disabled in both modes. The private-memory experiment
above subsequently passed, improving CPU throughput substantially.

Each timed batch executes the diagnostic once without per-instruction trace
comparison. Constructor/reset, trace validation, final RAM/register hash,
68k polling, logging and network transfer are outside the measured interval.
A bounded cancellation check every 256 instructions is included. There is one
discarded warm-up and ten measured samples. Each batch must reproduce the native
reference's cycle count, zero test errors, no unsupported bus access, and
RAM/register signature `f86dd990`. The read-only ARM global timer is calibrated
against Amiga timer.device over roughly two seconds; its rate is an estimate.

Both successful physical runs reached `RET1`, reset Core1 to idle before freeing
memory, exited 0 and removed the owner port. The I-cache run verified original
SCTLR `08c50878`, running `08c51878`, restored `08c50878`. No display buffer is
used by the CPU test; the launcher's short mapping probe uses a P96-owned bitmap.

## Alignment finding

The first two physical runs timed out. Added progress markers localized the
second stall to the CPU constructor. Disassembly showed compiler-generated
unaligned word stores, including offsets `0xc42` and `0xc2a` within the CPU
object. The ARM model initially accepted these accesses, while the physical
MMU-off environment did not complete initialization.

Adding **`-mno-unaligned-access`** to the Cortex-A9 build resolved the failure:
the full trace and benchmark then passed. This is strong evidence for an
alignment-related fault, not a captured ARM exception diagnosis (we did not
install exception vectors or capture DFSR/DFAR). Retain strict alignment for
this runtime. Do not infer that this was the cause of earlier unrelated
experiments. The two failed launches required the launcher's synchronous reset;
neither produced a normal-return marker. Their logs are retained.

## Build and verification

```sh
# Native C++ with AddressSanitizer and UndefinedBehaviorSanitizer:
python3 scripts/build_mesen_bench.py

# ARM payload plus standalone Amiga Hunk launcher; does not deploy/run:
python3 scripts/build_mesen_bench.py --arm \
  --container-command '["podman","--connection","nuflix-converter-root"]'

# Explicit instruction-cache variant, using the existing verified trampoline:
python3 scripts/build_mesen_bench.py --arm --icache \
  --container-command '["podman","--connection","nuflix-converter-root"]'

# Reproduce recorded results without contacting hardware:
python3 scripts/analyze_mesen_bench.py
```

Artifacts are in `.context/amiga/mesen-bench/` and `mesen-bench-icache/`.
The linked image contains the ROM/trace fixtures. The build downloads those
fixtures from a pinned [nes-test-roms commit](https://github.com/christopherpow/nes-test-roms/tree/95d8f621ae55cee0d09b91519a8989ae0e64753b/other)
and verifies SHA-256. `nestest`, by Kevin Horton, supplies the diagnostic;
the accompanying reference log supplies expected registers and cycle counts.
Fixture files/binaries are not redistributed here. This test uses automation
entry `$C000`, not the diagnostic's graphical menu. It does not establish
interrupt/DMA correctness, mapper compatibility, graphics or sound correctness.

Optional ARM instruction-model checks require `unicorn==2.1.4` in a local venv:

```sh
python tests/amiga/mesen/check_arm.py
```

They check both relocations at different synthetic addresses, reject unaligned
data accesses, detect a corrupted reference, and cancel after 512 checked
instructions. Model checks do not establish hardware/cache behavior.

Before physical use, follow `docs/amiga-arm-debugging.md`: re-enumerate tasks,
screens, clients and owner ports, ensure Core1 is free, verify the transferred
executable, use a fresh nonzero nonce and a 65536-byte Amiga stack. CLI syntax:
`ZZMesenBench <nonce> RUN`. Ctrl-C requests shutdown; each host request has a
10-second deadline, followed by the existing bounded return/reset teardown.
The test leaves no persistent installation or startup changes.

Evidence: [physical logs, build identities and checks](../records/2026-10-09/mesen-cpu/).
The next meaningful gate is CPU+PPU timing with frame correctness.
It is not yet a playable NES port.
