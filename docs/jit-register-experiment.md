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

## Earlier AHI notification (experimental)

`--audio-early` wakes the existing AHI worker once a three-frame batch is ready,
before presentation. Request size, eight host slots, six-frame prebuffer and
pause/state/stop flushes stay unchanged. The original and candidate ARM payloads
are byte-identical; only the 68k host is rebuilt.

Two 600-frame Super Mario World startup runs per variant, in reference/candidate/
candidate/reference order, same window and verification settings:

| Scheduling | Mean elapsed | Audio gaps per run | Presented frames per run |
| --- | ---: | --- | --- |
| Original | 21.973 s | 74 / 37 | 565 / 567 |
| Batch ready, before presentation | 18.976 s | 10 / 20 | 558 / 544 |

Mean elapsed time falls 13.64%, with fewer recorded gaps but also fewer
presentations. Core time stays around 3.30 s. This is a host scheduling tradeoff,
not an ARM-core speed gain. Window drawing has large timing outliers; these two
pairs are insufficient for a broad improvement claim. All runs deliver all
320,829 PCM samples, restore register/MMU/cache witnesses, exit zero and release
owned memory. A temporary log-read failure on the first reference was recovered;
its saved log and exit marker confirm completion.

The actual producer copy/mute/wrap/notification test passes 1,088 cases; the
native core suite also passes. Keep this option separate from the release until
broader title and audible quality checks establish its value; fullscreen results
below retain the presentation tradeoff.
[Audio comparison evidence](../amiga/records/2026-10-29/audio-handoff/).

### Fullscreen follow-up

Two 600-frame runs per variant, reference/candidate/candidate/reference order,
same fullscreen mode with scanlines and the FPS overlay off:

| Scheduling | Mean elapsed | Audio gaps per run | Presented frames per run |
| --- | ---: | --- | --- |
| Original | 21.969 s | 44 / 72 | 561 / 564 |
| Batch ready, before presentation | 16.974 s | 13 / 19 | 540 / 523 |

Mean elapsed time falls 22.74%, with mean presentations falling from 562.5 to
531.5. Core time remains about 3.279 s. Earlier AHI servicing changes the host's
scheduling and automatic frame-skipping behavior; this is not a 23% faster ARM
core. All 320,829 PCM samples complete per run, output endpoints match, and all
four runs pass register/MMU/cache restoration, exit zero and free owned memory.
The option remains experimental pending listening and broader gameplay checks.
