#include <assert.h>
#include <stdio.h>
#include "flip_retire.h"
int main(void){
 struct nf_slots s;unsigned scan=0;nf_init(&s);
 for(unsigned n=0;n<200000;n++){
  unsigned old=s.front,ready=s.render,retired=s.retired;
  uint16_t ack=(uint16_t)n;
  assert(ready!=scan && ready!=old && ready!=retired);
  assert(nf_queue(&s,ack));
  assert(s.render!=scan && s.render!=old && s.render!=ready && s.render!=retired);
  assert(!nf_queue(&s,ack));
  assert(nf_ack(&s,ack)==0);
  assert(nf_ack(&s,(uint16_t)(ack+2))==-1);
  /* Pessimistic model: the ISR consumes the prior software front before
   * the firmware publishes this acknowledgement. Not a hardware assertion. */
  scan=old;
  assert(nf_ack(&s,(uint16_t)(ack+1))==1);
  assert(s.front==ready && s.retired==old && s.render!=scan);
  assert(s.flips==n+1 && nf_ack(&s,ack)==-1);
 }
 puts("PASS 200000 four-slot handoffs, delayed scanout model, ACK wrap/rejection");
}
