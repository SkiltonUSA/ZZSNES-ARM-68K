#include <assert.h>
#include <stdio.h>
#include "scanline_key.h"
int main(void){
 int pending=-1;unsigned current=0;
 const unsigned expected[]={2,3,1,0,2,3,1,0};
 for(unsigned i=0;i<8;i++){
  em_scanline_key(0xd6,0,current,&pending);assert(pending==-1);
  em_scanline_key(0x56,1,current,&pending);assert(pending==-1);
  em_scanline_key(0x56,0,current,&pending);assert((unsigned)pending==expected[i]);
  em_scanline_key(0x56,1,current,&pending);assert((unsigned)pending==expected[i]);
  current=(unsigned)pending;pending=-1;
 }
 /* Distinct presses queued before a display boundary must not be lost. */
 em_scanline_key(0x56,0,0,&pending);em_scanline_key(0x56,0,0,&pending);assert(pending==3);
 pending=-1;em_scanline_key(0x56,0,4,&pending);assert(pending==0);
 puts("PASS F7 scanline cycle, release/repeat suppression, queued presses and Thin exit");
 return 0;
}
