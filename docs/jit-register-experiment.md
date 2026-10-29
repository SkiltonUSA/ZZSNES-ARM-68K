# Register translation — 0.7.2-dev

The measured register-translator slowdown is resolved in the Super Mario World
startup workload. The normal interpreter and existing releases remain unchanged.
`make jit-registers` now selects the improved experimental path.

## Physical comparison

Two 300-frame runs per build on the A4000TX, ordered interpreter → old translator
→ new translator → new translator → old translator → interpreter. Same ROM,
window, audio and verification settings. These are ARM core times, excluding
presentation and frontend work; they are not gameplay FPS.

| Build | Mean core time, 300 frames | Mean core time per frame |
|---|---:|---:|
| Interpreter | 1.477668 s | 4.926 ms |
| Earlier address-lookup translator | 1.652405 s | 5.508 ms |
| Opcode-table translator | **1.459818 s** | **4.866 ms** |

The old path was 11.83% slower than the interpreter in this repeat of the earlier
11.20% regression. The new path uses **11.65% less core time than the old path**,
and **1.21% less than the interpreter**. Two runs establish this result for this
workload, not a universal speed increase across all games.

All six runs match the video endpoint and complete 160,414 queued/submitted/
completed PCM frames. Register/MMU/cache witnesses restore, exit is zero and
owned memory is released. Audio-gap observations remain (new path: 6 and 9);
this does not establish uninterrupted audio. Wall-time variation is retained
in the raw logs and is not used for the core-time result.

## What changed

The old path classified every opcode, then looked up a guest-address cache for
simple INX/INY/DEX/DEY operations. The lookup, mode checks, counters and extra
function call cost more than the tiny operations they replaced.

The new path emits eight handlers once: four operations in 8- and 16-bit modes.
It installs them in the core's existing four opcode tables. The original opcode
fetch, CPU-mode table selection, interrupts, audio and event checks stay intact.
Other instructions and auxiliary processors keep their original implementations.
There is no per-instruction JIT eligibility check or address-tag lookup.

MOVW/MOVT binds each handler to the runtime-owned register/flag structures,
without literal pools. ARMv7 bitfields preserve register wraparound. The handler
reads live overclock controls, applies the original cycle increment and clears
the CPU wait address when the core requires it. State loads and CPU mode changes
continue to select the appropriate original table.

The **32 MiB executable reservation** and **96 MiB Exec-owned allocation** remain.
Eight handlers occupy slots within the first 2 KiB; an address-tag table is no
longer needed. The improvement comes from removing dispatch overhead, not cache
size. This remains single-instruction translation, not a complete CPU JIT or
multi-instruction trace compiler. Local cache publication, rewritten-code witness
and the corrected non-overlapping memory layout are retained; shared L2 is untouched.

## Validation and build

- 25,536 instruction-model emitter cases across three addresses.
- 32,768 comparisons against the actual original opcode handlers across two
  addresses: CPU modes, exact register/flag state, live cycle settings and wait reset.
- 600 Super Mario World and 300 PAL Chaos Engine frames match the interpreter;
  model hooks confirm generated handlers execute, without runtime profiling overhead.
- All 16,384 MMU allocation-alignment cases and native core regressions pass.
- Six physical runs pass the lifecycle checks described above.

Use the existing cross-toolchain setup and `make jit-registers`. Output is
`.context/jit-dispatch32-build`; launcher, normal candidate player and FX player
all identify **0.7.2-dev**. Installed programs and release packages are unchanged.

`make test-jit-layout` checks the built layout. Emitter tests are in
`tests/amiga/test_snes_dispatch.py`; `scripts/test_snes_dispatch_core.py` compares
built interpreter/candidate handlers. Full-core comparisons use
`scripts/test_console_jit_core.py --snes --table-dispatch` with a local ROM.
ROMs are not included in the records.

[Build hashes, measurements and test logs](../amiga/records/2026-10-29/jit-dispatch/).
[Earlier lookup-cache experiments and failures](jit-register-lookup-history.md).
