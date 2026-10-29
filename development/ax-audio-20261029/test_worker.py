from pathlib import Path
import subprocess,sys
w=Path(__file__).parent;out=w.parents[1]/'.context/ax-audio-native';out.mkdir(parents=True,exist_ok=True)
worker=(w/'ax_worker.h').read_text().replace('#include "ax_stream.h"','')
(out/'worker.h').write_text(worker)
source=r'''
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "ax_queue.h"
typedef unsigned ULONG;typedef int LONG;
#define AX_RATE 32040
#define AX_TYPE 2
#define AX_BYTES 4
#define AUDIO_SLOTS 8
#define AUDIO_PREBUFFER 6
#define AX_STEREO
struct Task {int unused;};
struct audio_block {unsigned count;unsigned char data[4096];};
static struct audio_block blocks[8],*audio_queue=blocks;
static unsigned audio_write,audio_read,audio_stop,audio_force,audio_started,audio_done,audio_opened;
static unsigned audio_worker_mask,audio_parent_signal=1,audio_submitted,audio_completed,audio_error;
static unsigned audio_ax_mode;
static unsigned audio_pending_samples,audio_feedback_ready,audio_state_drain,audio_state_drained;
static struct Task *audio_parent;
struct ax_stream {struct ax_queue q;unsigned mode,mixrate,playing;unsigned char buffers[3][12288],*data[3];};
static struct ax_stream *active;
static unsigned sent,total,burst,tick,limit,consumed,part,allocs,depth,disabled,fail_open,fail_start,cancel,state_test,drained,fallback;
static LONG AllocSignal(int n){(void)n;allocs++;return 2;}
static void FreeSignal(LONG s){assert(s==2&&allocs==1);allocs--;}
static void Signal(struct Task*t,ULONG m){(void)t;assert(m);}
static void Forbid(void){depth++;}static void Permit(void){assert(depth);depth--;}
static void Disable(void){disabled++;}static void Enable(void){assert(disabled);disabled--;}
static void CopyMem(const void*s,void*d,unsigned n){memcpy(d,s,n);}
static int ax_open(struct ax_stream*s,unsigned rate,unsigned type,unsigned bytes,unsigned mask){
 assert(rate==AX_RATE&&type==AX_TYPE&&bytes==AX_BYTES&&mask==4);if(fail_open)return 0;
 memset(s,0,sizeof(*s));ax_init(&s->q);for(unsigned i=0;i<3;i++)s->data[i]=s->buffers[i];
 s->mode=123;s->mixrate=rate;active=s;return 1;
}
static void ax_close(struct ax_stream*s){assert(s==active);active=NULL;}
static int ax_start(struct ax_stream*s){if(fail_start)return 0;s->playing=1;return 1;}
static void Wait(unsigned mask);
static void audio_worker(void){fallback++;}
#include "worker.h"
static void Wait(unsigned mask){
 assert(!depth&&!disabled&&mask==4);assert(++tick<10000);
 if(active->playing){
  ax_started(&active->q);int cur=active->q.current;
  if(cur<3){
   for(unsigned i=0;i<active->q.count[cur]*AX_BYTES;i++){
    assert(active->data[cur][i]==(unsigned char)consumed);
    if(++part==AX_BYTES*(1+(consumed*131)%1024)){part=0;consumed++;}
   }
  }
 }
 if(cancel&&tick>=cancel){audio_stop=1;return;}
 if(state_test&&!drained&&sent>=9){
  if(!audio_state_drained){audio_state_drain=audio_force=1;return;}
  assert(ax_idle(&active->q)&&audio_completed==audio_submitted&&consumed==sent);
  audio_state_drain=audio_state_drained=0;drained=1;
 }
 for(unsigned i=0;i<burst&&sent<total&&audio_write-audio_read<AUDIO_SLOTS;i++){
  struct audio_block*b=&blocks[audio_write%AUDIO_SLOTS];b->count=1+(sent*131)%1024;
  memset(b->data,(unsigned char)sent,b->count*AX_BYTES);sent++;audio_write++;
 }
 if(sent==total)audio_stop=2;
}
static void run(unsigned n,unsigned b,unsigned failure,unsigned cancellation,unsigned state){
 total=n;burst=b;fail_open=failure==1;fail_start=failure==2;cancel=cancellation;state_test=state;
 sent=tick=consumed=part=allocs=depth=disabled=drained=fallback=0;
 audio_write=audio_read=audio_stop=audio_force=audio_started=audio_done=audio_opened=0;
 audio_submitted=audio_completed=audio_error=audio_state_drain=audio_state_drained=0;
 audio_dispatch();assert(!active&&!allocs&&!disabled);
 if(fail_open){assert(fallback==1&&!audio_opened&&!depth);return;}
 assert(!fallback&&audio_done&&depth==1);
 if(!failure&&!cancel){unsigned sum=0;for(unsigned i=0;i<n;i++)sum+=1+(i*131)%1024;
  assert(consumed==n&&!part&&audio_completed==sum&&audio_submitted==sum&&!audio_error);
  if(state)assert(drained);
 }
 if(fail_start)assert(audio_error);
}
int main(void){
 for(unsigned n=1;n<10;n++)run(n,1,0,0,0);
 run(600,8,0,0,0);run(600,1,0,0,0);run(61,1,0,0,1);
 run(30,8,1,0,0);run(30,8,2,0,0);run(100,8,0,5,0);
 puts("PASS actual AX worker: ordered PCM, three-buffer reuse, partial stop, state drain/resume, open fallback, start failure, cancel/join");
}
'''
(out/'worker.c').write_text(source)
subprocess.run(['clang','-std=c99','-O2','-g','-fsanitize=address,undefined','-Wall','-Wextra','-Werror','-Wno-misleading-indentation','-Wno-unused-function','-Wno-unused-variable','-I'+str(w),str(out/'worker.c'),'-o',str(out/'worker')],check=True)
subprocess.run([str(out/'worker')],check=True)

source=source.replace('#define AX_RATE 32040','#define AX_RATE 32000').replace('#define AX_TYPE 2','#define AX_TYPE 1').replace('#define AX_BYTES 4','#define AX_BYTES 2').replace('#define AUDIO_PREBUFFER 6','#define AUDIO_PREBUFFER 4').replace('#define AX_STEREO','')
(out/'worker.c').write_text(source)
subprocess.run(['clang','-std=c99','-O2','-g','-fsanitize=address,undefined','-Wall','-Wextra','-Werror','-Wno-misleading-indentation','-Wno-unused-function','-Wno-unused-variable','-I'+str(w),str(out/'worker.c'),'-o',str(out/'worker-mono')],check=True)
subprocess.run([str(out/'worker-mono')],check=True)
