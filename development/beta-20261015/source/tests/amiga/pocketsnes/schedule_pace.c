/* Exercise generated cadence, including timer wrap, lag recovery and pause.
 * The host's two-frame lookahead must not become uncapped emulation. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
typedef uint32_t ULONG;typedef int32_t LONG;
#include "pace.h"
int main(void){
 assert(fc_next_deadline(100000,0)==116689);
 assert(fc_next_deadline(118689,116689)==133378);
 assert(fc_next_deadline(200000,116689)==216689);
 assert(fc_next_deadline(120000,0)==136689);
 assert(fc_next_deadline(0xfffffff0u,0xffffffe0u)==0x4111u);
 for(unsigned wrap=0;wrap<2;wrap++)for(unsigned cost=1000;cost<=25000;cost+=4000){
  ULONG now=wrap?0xffff0000u:100000u,start=now,deadline=0;
  for(unsigned i=0;i<3600;i++){
   ULONG before=now;deadline=fc_next_deadline(now,deadline);now+=cost;
   ULONG target=(LONG)(deadline-before)>0?deadline-before:0;target=target>33378u?target-33378u:0;
   if(now-before<target)now=before+target;
   /* No more than two frames of lookahead, even when compute is very fast. */
   assert((uint64_t)(i+1)*16689u <= (uint64_t)(ULONG)(now-start)+33378u);
  }
 }
 puts("PASS cadence: NTSC period, two-frame lead bound, slow core, jitter reset, pause reset and clock wrap");
}
