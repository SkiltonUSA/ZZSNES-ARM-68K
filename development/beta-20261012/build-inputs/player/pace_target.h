/* GPL-3.0-or-later. A zero deadline means pacing was reset (e.g. snapshot).
 * Never interpret that sentinel as an absolute time after signed clock wrap.
 * Retain two frames of lookahead; reject implausibly distant deadlines. */
#ifndef PS_PACE_TARGET_H
#define PS_PACE_TARGET_H
#include <stdint.h>
static inline uint32_t ps_pace_target(uint32_t now,uint32_t deadline,uint32_t period){
 int32_t delta;
 if(!deadline || !period || period>100000u)return 0;
 delta=(int32_t)(deadline-now);
 if(delta<=0 || (uint32_t)delta>3u*period)return 0;
 return (uint32_t)delta>2u*period?(uint32_t)delta-2u*period:0;
}
#endif
