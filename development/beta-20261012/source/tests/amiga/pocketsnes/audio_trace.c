#include <stdint.h>
#include <assert.h>
#include <stdarg.h>
typedef uint32_t ULONG;
static int sink(const char *format,...){(void)format;return 0;}
#define printf sink
#include "audio_trace.h"
int main(void){
 for(ULONG i=0;i<1200;i++){
  for(unsigned s=0;s<6;s++)at_stage(s,i,i+s);
  at_audio(i,i*7,i,500*i,400*i,300*i,i/50);
  at_ack(i,0xffff0000u+i*16800u);
 }
 assert(at_count==1200 && at_ack_count==1200 && !at_overflow);
 assert(at_rows[1199].stage[4]==1203 && at_rows[1199].samples==599500);
 at_stage(0,1200,1);assert(at_overflow);at_overflow=0;
 at_audio(1200,0,0,0,0,0,0);assert(at_count==1200 && at_overflow);
 at_ack(1200,1);at_ack(1200,2);at_ack(1200,3);
 assert(at_ack_count==1202);at_print();return 0;
}
