#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "cadence.h"
int main(void){
 struct ps_cadence c={0};uint32_t p=16689,start=100000;
 assert(ps_cadence_next(&c,start,p)==start+p);
 /* A short scheduler stall must not permanently discard guest/audio time. */
 assert(ps_cadence_next(&c,start+3*p,p)==start+2*p);
 assert(c.rebases==0);
 assert(ps_cadence_next(&c,start+20*p,p)==start+21*p);
 assert(c.rebases==1);
 c=(struct ps_cadence){0};start=UINT32_MAX-100;
 for(unsigned i=0;i<10000;i++)assert(ps_cadence_next(&c,start+i*p,p)==start+(i+1)*p);
 assert(!c.rebases);
 c=(struct ps_cadence){0};ps_cadence_next(&c,100,p);
 for(unsigned i=0;i<30;i++){int show=ps_cadence_show(&c,100+20*p,p);assert(c.streak<=2);if(i%3==0)assert(show);}
 for(unsigned limit=3;limit<=5;limit+=2){
  c=(struct ps_cadence){0};ps_cadence_next(&c,100,p);
  for(unsigned i=0;i<30;i++){int show=ps_cadence_show_budget(&c,100+20*p,p,limit);assert(c.streak<=limit);assert(show==(i%(limit+1)==0));}
 }
 c=(struct ps_cadence){.deadline=100000,.shown=1};
 assert(!ps_cadence_show_budget(&c,90000,p,5));
 c.streak=0;assert(ps_cadence_show(&c,90000,p));
 puts("PASS short-stall recovery, long-stall rebase, timer wrap and bounded picture omission");
}
