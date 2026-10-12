/* Explicit CPU-benchmark opt-in to the already verified local I-cache
 * trampoline. The implementation's historical VIDEO names do not select
 * a decoder. It saves/restores SCTLR; MMU/D-cache/PL310 remain untouched. */
#define ZZ_VIDEO 1
#define ZZ_VIDEO_ICACHE 1
#include "../arm_debug/zz9000/entry.S"
