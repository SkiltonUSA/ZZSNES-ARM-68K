/* GPL-3.0-or-later. No emulation lookahead: wait for the regional deadline.
 * Zero is the existing reset sentinel. Signed subtraction handles timer wrap. */
#ifndef PS_PACE_TARGET_H
#define PS_PACE_TARGET_H
#include <stdint.h>
static inline uint32_t ps_pace_target(uint32_t now,uint32_t deadline,uint32_t period){
 int32_t delta;
 if(!deadline || !period || period>100000u)return 0;
 delta=(int32_t)(deadline-now);
 if(delta<=0 || (uint32_t)delta>period)return 0;
 return (uint32_t)delta;
}
#endif
