/* Exercise the actual NES three-slot plan with omitted presentations. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "cadence.h"
#include "flip_plan.h"
int main(void){
 struct ps_cadence c={0};struct nf_slots slots;nf_init(&slots);
 uint32_t now=0xffff0000u;uint16_t ack=0;unsigned locked=0,writes[3]={0};
 for(unsigned frame=0;frame<200000;frame++){
  ps_cadence_next(&c,now,16689);now+=9000;
  if(slots.pending>=0){
   assert(locked);assert(!nf_queue(&slots,ack));
   assert(nf_ack(&slots,ack)==0);assert(nf_ack(&slots,++ack)==1);locked=0;
  }
  unsigned old_front=slots.front;
  if(ps_cadence_show(&c,now,16689)){
   assert(!locked && slots.pending<0);
   assert(slots.render!=old_front);
   writes[slots.render]++;locked=1;assert(nf_queue(&slots,ack));
   now+=7800;
  }else{
   assert(!locked && slots.pending<0);
   assert(slots.front==old_front);
  }
 }
 if(locked){assert(nf_ack(&slots,++ack)==1);locked=0;}
 assert(slots.flips==c.shown && c.shown+c.skipped==200000 && c.max_streak<=2);
 for(unsigned i=0;i<3;i++)assert(writes[i]>0);
 puts("PASS 200000 handoffs/omissions, ACK wrap, no writes to front/pending, clean drain");
}
