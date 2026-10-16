#include <assert.h>
#include <stdint.h>
#include <stdio.h>
typedef uint32_t ULONG;
static ULONG now,origin;static unsigned calls;static int quit,stuck,cancel;
static ULONG us(void){now+=100;return now;}
static int full_blank(void){return stuck?1:(uint32_t)(now-origin)%16667u<1000u;}
static void events(void){calls++;if(cancel)quit=1;}
#include "scanout_measure.h"
int main(void){
 for(unsigned i=0;i<2;i++){
  now=origin=i?0xffffff00u:0;calls=0;
  assert(full_measure_refresh());assert(calls>10);assert((uint32_t)(now-origin)<400000);
 }
 stuck=1;now=origin=0;assert(!full_measure_refresh());assert(now<801000);
 stuck=0;cancel=1;quit=0;now=origin=0;
 assert(!full_measure_refresh());assert(quit && now<10000);
 puts("PASS read-only blanking sampler: nominal clock, timer wrap, stuck status, event cancellation");
}
