#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <exec/memory.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include "ax_stream.h"
int main(int argc,char **argv){
 struct ax_stream s;LONG signal=AllocSignal(-1);ULONG sent=0,ticks=0;int rc=20;
 ULONG stereo=argc>1 && !strcmp(argv[1],"stereo");ULONG rate=stereo?32040:32000;
 if(signal<0)return 20;
 if(!ax_open(&s,rate,stereo?AHIST_S16S:AHIST_M16S,stereo?4:2,1UL<<signal)){puts("AX_OPEN failed (no preferences changed)");goto done;}
 printf("AX_OPEN mode=%08x rate=%u mix=%u capacity=%u silence=%u stereo=%u\n",s.mode,rate,s.mixrate,s.capacity,s.silent_frames,stereo);
 /* Silent PCM proves buffer turnover without an unexpected tone. */
 while(sent<60 || !ax_idle(&s.q)){
  int slot=ax_free(&s.q);
  if(sent<60 && slot>=0){Disable();ax_publish(&s.q,1600);Enable();sent++;}
  if(!s.playing && sent==3){if(!ax_start(&s))goto close;}
  if(++ticks>500){puts("AX_TIMEOUT");goto close;}
  Delay(1);
 }
 printf("AX_DONE submitted=%u completed=%u callbacks=%u silence=%u\n",sent*1600,s.q.completed,s.q.callbacks,s.q.silence);
 rc=s.q.completed==sent*1600?0:20;
close:ax_close(&s);puts("AX_CLOSED");
done:FreeSignal(signal);return rc;
}
