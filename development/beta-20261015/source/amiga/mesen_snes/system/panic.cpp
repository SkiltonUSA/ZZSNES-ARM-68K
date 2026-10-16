#include "tiny.h"
#include "wire.h"
extern "C" void mesen_progress(uint32_t);
extern "C" void ss_panic(){
 // Bounded host request deadline cancels/resets Core1 before freeing memory.
 // Used only for impossible adapter bounds; never continue after overflow.
 mesen_progress(0xdead0001);
 for(;;)__asm__ volatile("nop");
}
