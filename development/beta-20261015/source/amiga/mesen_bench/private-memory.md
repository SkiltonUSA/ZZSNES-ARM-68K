# Bounded private MMU/L1 experiment

Verified on the A4000TX's XX19c/XACP 1.7 Core1, 9 October 2026. This policy is
only selected by `--private-mmu` or `--private-cache`. Default SDK, fractal,
video and earlier Mesen modes retain their existing execution contracts.
Nothing was flashed or installed persistently on the Amiga.

## Ownership and mapping

The existing launcher reserves **1 MiB of actual Exec free ZZ9000 RAM** and
proves the candidate ARM translation with eight session-derived values using
the short P96 bootstrap. No fixed DDR address or ARM arena is claimed.
All code/data/stack/tables survive until Core1 has returned and been reset idle.

Offsets within the allocation:

| Range | Purpose and attributes |
| --- | --- |
| `0..0x3ffff` | Image, inner WB/WA, outer NC where pages are fully owned |
| `0x40000..0x4ffff` | Page-table reserve, normal NC, execute-never |
| `0x60000..0x603ff` | Cached writeback witness; adjacent zero canaries |
| `0x80000..0x83fff` | Control, stop, mailbox, saved state, reports: normal NC, XN |
| End of fully owned final page | Private stack, inner WB/WA, outer NC, XN |

The image's first section contains only the entry trampoline, followed by a
4 KiB image-relative pad. Ordinary code then lies beyond the partially owned
first physical page, avoiding the slow uncached execution seen in the first
private-cache run. The linker rejects images reaching the table reserve.

The L1 table is dynamically aligned to 16 KiB inside the table reserve. Up to
two 1 KiB coarse tables cover the allocation; a third covers the single
read-only, execute-never page at `0xf8f00000` containing the global timer.
This is an architectural peripheral address, not a DDR allocation. The shared
PL310 controller page at `0xf8f02000` is not mapped during execution.

Normal private small-page descriptors use `TEX=100,C=0,B=1,S=0,AP=01`:
inner WB/WA, outer NC, privileged read/write. Public/table pages use
`TEX=001,C=B=0,S=1,AP=01`: normal inner/outer NC. Domain 0 is client; other
domains and unrelated virtual sections fault. TTBR walks are uncached.
TEX remapping/access-flag reinterpretation are disabled locally and restored.

**Page granularity caveat:** the allocation is 64-byte aligned. Its partial
first/last 4 KiB pages are mapped uncached, so a few adjacent bytes are
addressable, though never accessed by the test. No partially owned page is
cacheable. This is a bounded application policy, not a security sandbox.
There is no alias map or claim that a barrier makes cached mailboxes coherent.

## Entry, return and failure

1. Require MMU/I/D off, physical Core1, the verified Cortex-A9 ID and privileged
   SVC/System mode. Save registers/firmware stack and CPSR; mask A/IRQ/FIQ on
   Core1 while the private map is active. No firmware callback is invoked then.
2. Save SCTLR, TTBR0/1, TTBCR, DACR, CSSELR, ACTLR, ContextIDR, PRRR, NMRR and
   VBAR. Read (never write) PL310 control/auxiliary state for comparison.
3. Select L1 data CCSIDR temporarily, verify 32 KiB/four ways/256 sets/32-byte
   lines, then restore CSSELR. Construct bounded short-descriptor tables.
4. Perform **local L1** clean/invalidate by set/way, local I-cache/branch/TLB
   invalidation, DSB/ISB. Install private TTBR0, TTBCR and DACR. Enable MMU/I;
   optionally D. ACTLR and shared L2 registers are not modified.
5. Execute the unmodified Mesen instruction logic with the existing strict
   alignment compile flag. The host communicates only through uncached pages.
   Each command carries a nonce-bound sequence challenge and returned echo.
6. After timed samples, request repeated CPU work, set the uncached stop word
   after at least 100 ms, and require the worker's cancellation result. Stop is
   sampled every 256 emulated instructions. The 68k also honors Ctrl-C and a
   ten-second request deadline.
7. Assembly cleans/invalidates L1, including the active stack, then restores
   SCTLR without any intervening stack store. Restore translations/domain and
   invalidate only local TLB/I/branch state. Restore firmware stack and CPSR
   control/extension fields (including A/IRQ/FIQ), publish RET1, and return.
8. The host resets Core1 idle before inspecting the 256-word cached witness,
   boundary canaries and restoration report, then frees memory. This checks
   dirty private data reached RAM before release. Thirteen recorded control
   values match exactly; CPSR interrupt/mode bits are compared separately.

The running SCTLRs were `08c51879` (MMU/I) and `08c5187d` (MMU/I/D), restored
to `08c50878`. The runtime-verified CCSIDR was `701fe019`. These are observations
and guards for this machine, not portable configuration constants.

On failure the launcher's bounded stop/reset path still precedes FreeMem.
There is no private exception handler; a synchronous ARM fault cannot be
assumed to return or publish its syndrome. Cold boot, long-duration stress,
arbitrary applications, interrupts, DMA coherence and a full emulator remain
separate validation work. Changing D-cache policy elsewhere requires its own
ownership/coherence review.

## Evidence and checks

Three final cached launches matched 26,973 reference CPU states, 30 timed
batches plus warm-ups, three active-work cancellations, mailbox echoes,
writeback witnesses, restoration, RET1 and exit 0. Five launches in this
investigation (two intermediate plus three final) returned cleanly.

Native ASan/UBSan page-table tests exercise **16,384 base alignments**, checking
private/shared/table/boundary descriptors, privilege/XN permissions, and absence
of mappings for L2/unrelated RAM. ARM instruction-model tests separately check
the CPU payload at two synthetic bases, corrupt-reference rejection and
cancellation. They do **not** model cache coherency or prove MMU operation;
those claims rely on the physical results.

```sh
clang -O2 -g -fsanitize=address,undefined -Iamiga/mesen_bench \
  tests/amiga/mesen/mmu_plan.c -o .context/amiga/test-mesen-plan
.context/amiga/test-mesen-plan
python3 scripts/analyze_mesen_cache.py
```

Primary references reviewed:

- [Xanxi's XACP 1.7 notes](https://github.com/Xanxi-Amiga/XACP-ZZ9000/blob/main/docs/XACP_V1_7_DEVELOPER_NOTES.md): Core1 must avoid generic operations reaching shared PL310.
- [XX19c Core1 launch implementation](https://github.com/Xanxi-Amiga/XACP-ZZ9000/blob/main/firmware/source/XX19c/ZZ9000_proto.sdk/ZZ9000OS/src/core2.c): firmware launch-side L1 maintenance and synchronous reset sequencing.
- [Arm Cortex-A9 TRM](https://documentation-service.arm.com/static/5e8e264088295d1e18d37ce8): translation/control/cache registers and maintenance.
- [AMD/Xilinx L1 cache implementation](https://github.com/Xilinx/embeddedsw/blob/master/lib/bsp/standalone/src/arm/cortexa9/xil_cache.c): L1 set/way geometry and clean/invalidate sequencing, separate from L2.

Measured timings, exact build hashes and raw logs are in
[the evidence directory](../records/2026-10-09/mesen-cache/). This is a CPU
throughput improvement; it does not establish NES gameplay FPS.
