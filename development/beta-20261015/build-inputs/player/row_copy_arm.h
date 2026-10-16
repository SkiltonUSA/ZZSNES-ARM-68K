/* GPL-3.0-or-later. Exactly one owned 320-pixel row, both pointers word-aligned.
 * Use ordinary ARM integer registers only; no VFP/NEON state is touched.
 * Caller validates pitch/span, polls STOP per row, and DSBs before unmapping.
 */
static __attribute__((noinline)) void ps_copy_row(volatile uint32_t *dst,const uint32_t *src){
 unsigned groups=40;
 __asm__ volatile(
  "1: ldmia %[source]!, {r2-r9}\n"
  "stmia %[destination]!, {r2-r9}\n"
  "subs %[groups], %[groups], #1\n"
  "bne 1b\n"
  : [destination] "+&r" (dst), [source] "+&r" (src), [groups] "+&r" (groups)
  : : "r2","r3","r4","r5","r6","r7","r8","r9","cc","memory");
}
