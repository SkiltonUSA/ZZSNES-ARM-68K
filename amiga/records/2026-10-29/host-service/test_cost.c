#include <assert.h>
#include <stdio.h>
#include "cost.h"
int main(void){
 struct hc_cost c={0};hc_add(&c,UINT32_MAX-3,4);assert(c.calls==1&&c.total==8&&c.maximum==8&&c.bins[0]==1);
 const uint32_t v[]={15,16,63,64,255,256,999,1000,3999,4000,15999,16000,63999,64000};
 for(unsigned i=0;i<sizeof(v)/sizeof(v[0]);i++)hc_add(&c,100,100+v[i]);
 assert(c.calls==15&&c.maximum==64000&&c.bins[0]==2&&c.bins[7]==1);
 unsigned n=0;for(unsigned i=0;i<8;i++)n+=c.bins[i];assert(n==c.calls);
 puts("PASS cost histograms: unsigned timer wrap, exact boundaries, totals");
}
