#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "pace_target.h"
int main(void){
 const uint32_t periods[]={16689,20000};
 for(unsigned p=0;p<2;p++){
  uint32_t period=periods[p];
  /* Snapshot reset at low, high-bit-set and wrapping absolute times. */
  const uint32_t starts[]={0,12345,0x7fffffffu,0x80000000u,0xf8000000u,0xfffffff0u};
  for(unsigned n=0;n<sizeof(starts)/sizeof(starts[0]);n++){
   uint32_t now=starts[n];
   assert(!ps_pace_target(now,0,period));
   assert(!ps_pace_target(now,now-1,period));
   assert(!ps_pace_target(now,now+period,period));
   assert(ps_pace_target(now,now+3*period,period)==period);
   assert(!ps_pace_target(now,now+4*period,period));
  }
  for(unsigned wrap=0;wrap<2;wrap++)for(unsigned cost=1000;cost<=25000;cost+=4000){
   uint32_t now=wrap?0xffff0000u:100000u,deadline=0,epoch=now;unsigned count=0;
   for(unsigned i=0;i<3600;i++){
    if(!deadline || (int32_t)(now-deadline)>(int32_t)(2*period))deadline=now;
    deadline+=period;uint32_t before=now;now+=cost;
    /* Reproduce a thumbnail resetting pacing midway through a frame. */
    if(i%301==300){deadline=0;epoch=now;count=0;}
    uint32_t target=ps_pace_target(before,deadline,period);
    assert(target<=period);
    if(now-before<target)now=before+target;
    if(deadline){count++;assert((uint64_t)count*period<=(uint64_t)(uint32_t)(now-epoch)+2*period);}
   }
  }
 }
 puts("PASS reset sentinel, wrap, invalid deadline, PAL/NTSC pacing and mid-frame thumbnail reset");
}
