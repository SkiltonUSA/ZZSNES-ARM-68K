#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "cadence.h"
#include "pace_target.h"
static void run(uint32_t period,uint32_t refresh,uint32_t offset){
 struct ps_cadence c={0};uint64_t now=0,flip=0;unsigned pending=0,pcm=0;
 for(unsigned f=0;f<18000;f++){
  uint64_t start=now;uint32_t due=ps_cadence_next(&c,(uint32_t)now+offset,period);
  now+=6300;pcm++; /* Every core/APU frame runs, irrespective of presentation. */
  if(pending){if(now<flip)now=flip;pending=0;}
  now+=500;if(f%359==358)now+=21000; /* Simulated transfer and OS stalls. */
  if(ps_cadence_show(&c,(uint32_t)now+offset,period)){
   assert(!pending);now+=3000;flip=(now/refresh+1)*refresh;pending=1;
  }
  uint32_t target=ps_pace_target((uint32_t)start+offset,due,period);
  if(now-start<target)now=start+target;
  assert(c.shown+c.skipped==f+1 && c.streak<=2);
  assert((uint64_t)(f+1)*period<=now+2*period);
 }
 assert(pcm==18000 && c.shown>15000 && c.rebases==0);
 assert(now>=(uint64_t)18000*period-2*period);
 assert(now<(uint64_t)18000*period+4*period);
 printf("PASS period=%u display=%u shown=%u omitted=%u elapsed=%llu\n",period,refresh,c.shown,c.skipped,(unsigned long long)now);
}
int main(void){
 run(16639,16800,0);run(16639,16800,0xffff0000u);
 run(19998,16800,0);run(19998,20000,0xffff0000u);
 run(20000,16800,0);run(20000,20000,0x80000000u);
 struct ps_cadence c={0};ps_cadence_next(&c,1000,16639);
 assert(ps_cadence_show(&c,500000,16639)); /* First picture is required. */
 assert(ps_cadence_next(&c,600000,16639)==616639 && c.rebases==1);
 for(unsigned i=0;i<9;i++){int shown=ps_cadence_show(&c,900000,16639);assert(shown==(i%3==2));}
 c.valid=0;assert(ps_cadence_next(&c,100,20000)==20100);
 puts("PASS wrap, bounded catch-up, presentation gaps, regional cadence and reset");
}
