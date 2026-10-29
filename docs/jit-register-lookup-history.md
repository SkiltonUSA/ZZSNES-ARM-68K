# Experimental register translation — 0.7.1-dev

The released interpreter remains the default. This candidate reserves a **32 MiB
executable cache** in a **96 MiB Exec-owned allocation** on ZZ9000 Core1.
It is a small A32 register translator, not a complete CPU JIT. There are 262,144
128-byte code slots plus 2 MiB of tag metadata. Cache bounds are shared by the
emitter and MMU; page-table storage is enlarged and the witness is relocated.

The previous 8 MiB build and its evidence remain available. The measurements
below describe that older build, not the new 32 MiB candidate.

## Earlier 8 MiB measurements

A physical 300-frame comparison completed with matching output and clean
MMU/register/cache restoration. Core time was 1,480,360 us for the interpreter
and 1,646,231 us for the candidate: **11.20% slower**. This is not a speed
improvement, so the candidate has not replaced the installed player or release.
Wall time includes presentation and verification work and is not a CPU benchmark.

The translator handles X/Y increment and decrement in 8- and 16-bit modes.
Everything else falls back to the interpreter. Each instruction returns to the
original scheduler, preserving opcode reads, cycles, audio and interrupt/event
boundaries. Auxiliary cartridge processors are unchanged. Multi-instruction
traces are future work and require event-timing comparisons before use.

Cache lookup validates the current opcode and CPU width. Register values are
loaded at execution time; no guest memory values are cached. Generated code uses
ARMv7 UBFX for wrapping. There are no arbitrary 32-bit constants in these small
fragments, so adding MOVW/MOVT would add no benefit. The existing core cycle table
already provides precomputed timing; variable event costs must remain intact.

Code publication cleans local L1 data lines, invalidates local instruction lines
and synchronizes execution. It never changes shared L2. A rewritten-code witness
runs before guest execution. Only fully owned code-cache pages are executable;
stack/shared regions remain non-executable. The allocated cache capacity exceeds
the tiny amount of code used by this initial experiment.

## Earlier verification

- 9,576 Cortex-A9 instruction-model cases: 8/16-bit arithmetic, flag representation
  and unchanged surrounding state.
- 16,384 allocation alignments: cache bounds, executable pages and table capacity.
- 600 linked interpreter/JIT frames: matching Super Mario World video and PCM output.
- Physical 300-frame run: matching output, exit 0 and owned-memory release.
- Audio: 160,414 frames queued, submitted and completed; no reported error.

Build separately with `make jit-registers` using the existing cross-toolchain
setup. Outputs go to `.context/jit-registers32-build`; normal artifacts are retained.
Launcher/player versions both read 0.7.1-dev for this candidate. It is not a
release package. The shared emitter test is `tests/amiga/test_console_jit.py`;
full-core comparisons use `scripts/test_console_jit_core.py` with explicit
`--baseline`, `--candidate`, `--baseline-packer` and `--snes`, plus a local ROM.
ROM data is not included in the evidence.

[Build hashes and physical logs](../amiga/records/2026-10-28/jit-registers/).

## 32 MiB revision — 29 October 2026

Native regressions, 9,576 emitter cases, 16,384 page-map alignment cases and
600 linked interpreter-comparison frames pass. The layout test now also checks
witness separation from actual video/audio, ROM, save, control and table ranges.

An initial physical run completed rendering but failed its witness check: the
relocated witness overlapped the video output buffer. The launcher returned
exit 20 and released memory. A new native overlap test reproduced the fault;
the corrected witness is at allocation offset 0x12c000, between the page tables
and video output. The failed run and build hashes are retained in the records.

The corrected physical run completed 300 frames using a 100,663,296-byte owned
allocation. Output hash ca7174c7 matches the reference; all 160,414 PCM frames
were queued, submitted and completed. Registers and the witness restored with
zero mismatches; launcher exit 0 and owned memory released. Twelve audio-gap
observations remain, so this is not an uninterrupted-audio claim. Core time
was 1,647,549 us; no speed improvement is established from this single run.

Build and test evidence is recorded separately from the earlier measurements.
[32 MiB records](../amiga/records/2026-10-29/jit-registers32/).
