#include <assert.h>
#include <stdio.h>
#include "scanline_key.h"
#include "scanline_backend.h"
int main(void){
 int pending=-1;unsigned current=0;
 const unsigned expected[]={2,3,1,0,2,3,1,0};
 for(unsigned i=0;i<8;i++){
  em_scanline_key(0xd6,0,current,0,&pending);assert(pending==-1);
  em_scanline_key(0x56,1,current,0,&pending);assert(pending==-1);
  em_scanline_key(0x56,0,current,0,&pending);assert((unsigned)pending==expected[i]);
  em_scanline_key(0x56,1,current,0,&pending);assert((unsigned)pending==expected[i]);
  current=(unsigned)pending;pending=-1;
 }
 /* Distinct presses queued before a display boundary must not be lost. */
 em_scanline_key(0x56,0,0,0,&pending);em_scanline_key(0x56,0,0,0,&pending);assert(pending==3);
 pending=-1;em_scanline_key(0x56,0,4,0,&pending);assert(pending==0);
 for(unsigned mode=0;mode<=4;mode++){
  assert(em_scan_software(0,mode)==0);
  assert(em_scan_hardware(1,mode)==0);
  assert(em_scan_software(1,mode)==(mode?2:0));
  assert(em_scan_hardware(0,mode)==em_scan_normalize(mode));
 }
 pending=-1;em_scanline_key(0x56,0,3,1,&pending);assert(pending==0);
 em_scanline_key(0x56,0,3,1,&pending);assert(pending==2);
 puts("PASS F7 scanline cycle, release/repeat suppression, queued presses and legacy mode handling");
 return 0;
}
