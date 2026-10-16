/* GPL-3.0-or-later. Host experiment: regional emulation clock with bounded
 * presentation omission. Never skips core/APU work or changes audio rate.
 * Caller drains the previous PAN and preserves all existing ownership checks. */
#ifndef PS_CADENCE_H
#define PS_CADENCE_H
#include <stdint.h>
struct ps_cadence {uint32_t deadline,shown,skipped,rebases,late_max,streak,max_streak;int valid;};
static inline uint32_t ps_cadence_next(struct ps_cadence *c,uint32_t now,uint32_t period){
 if(!c->valid){c->deadline=now;c->valid=1;}
 else if((int32_t)(now-c->deadline)>250000){c->deadline=now;c->rebases++;}
 c->deadline+=period;return c->deadline;
}
static inline int ps_cadence_show(struct ps_cadence *c,uint32_t now,uint32_t period){
 int32_t late=(int32_t)(now-c->deadline);
 if(late>0 && (uint32_t)late>c->late_max)c->late_max=(uint32_t)late;
 /* Budget a quarter frame for conversion/queueing. Always show the first
  * picture and never omit more than two consecutive completed pictures. */
 if(c->shown && (int32_t)(now+period/4-c->deadline)>0 && c->streak<2){
  c->skipped++;c->streak++;if(c->streak>c->max_streak)c->max_streak=c->streak;return 0;
 }
 c->shown++;c->streak=0;return 1;
}
#endif
