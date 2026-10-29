/* GPL-3.0-or-later. Elapsed-call histograms, not CPU utilization. */
#pragma once
#include <stdint.h>
struct hc_cost { uint32_t calls,total,maximum,bins[8]; };
static void hc_add(struct hc_cost *c,uint32_t start,uint32_t end){
 static const uint32_t limit[]={16,64,256,1000,4000,16000,64000};
 uint32_t elapsed=end-start;unsigned b=0;
 while(b<7&&elapsed>=limit[b])b++;
 c->calls++;c->total+=elapsed;c->bins[b]++;
 if(elapsed>c->maximum)c->maximum=elapsed;
}
