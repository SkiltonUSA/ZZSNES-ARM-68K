/* GPL-3.0-or-later. Per-application worker backend, original unit-0 path is fallback.
 * Same source PCM and 3-frame batches; selected by the launcher audio setting.
 * Three immutable dynamic buffers allow current, scheduled and ready audio.
 * Prime with three complete batches (about 150ms NTSC / 180ms PAL). */
#include "ax_stream.h"
static ULONG audio_ax_mix,audio_ax_callbacks,audio_ax_silence,audio_ax_starvations;
static int audio_ax_worker(void){
 struct ax_stream stream;LONG signal=AllocSignal(-1);ULONG mask;int primed=0;
 if(signal<0)return 0;mask=1UL<<signal;
 if(!ax_open(&stream,AX_RATE,AX_TYPE,AX_BYTES,mask)){FreeSignal(signal);return 0;}
 audio_worker_mask=mask;audio_ax_mode=stream.mode;audio_ax_mix=stream.mixrate;
 audio_opened=1;audio_started=1;Signal(audio_parent,1UL<<audio_parent_signal);
 for(;;){
  ULONG available;int slot;
  if(audio_stop==1)break;
  stream.q.draining=audio_stop==2||audio_state_drain;
  Forbid();available=audio_write-audio_read;Permit();
  if(!available)audio_force=0;
  if(!primed&&(available>=9u||audio_stop==2||audio_force))primed=1;
  audio_completed=stream.q.completed;
#ifdef AX_STEREO
  {ULONG i,pending=0;Disable();for(i=0;i<3;i++)if(stream.q.occupied[i])pending+=stream.q.count[i];Enable();
   Forbid();audio_pending_samples=pending;audio_feedback_ready=primed;Permit();}
#endif
  slot=ax_free(&stream.q);
  if(primed&&slot>=0&&available&&(available>=3||audio_stop==2||audio_force)){
   ULONG blocks=available>=3?3:available,k,n=0;
   for(k=0;k<blocks;k++){
    ULONG index=(audio_read+k)%AUDIO_SLOTS,count=audio_queue[index].count;
    if(count>1024){audio_error=1;break;}
    CopyMem(audio_queue[index].data,stream.data[slot]+n*AX_BYTES,count*AX_BYTES);n+=count;
   }
   if(audio_error)break;
   Forbid();audio_read+=blocks;Permit();
   Disable();ax_publish(&stream.q,n);Enable();audio_submitted+=n;
   if(!stream.playing&&!ax_start(&stream)){audio_error=1;break;}
   continue;
  }
  if(!available&&ax_idle(&stream.q)){
   if(audio_stop==2)break;
   if(audio_state_drain){primed=0;audio_state_drained=1;}
  }
  Wait(mask);
 }
 audio_completed=stream.q.completed;audio_ax_callbacks=stream.q.callbacks;audio_ax_silence=stream.q.silence;audio_ax_starvations=stream.q.starvations;
 ax_close(&stream);FreeSignal(signal);
 Forbid();audio_worker_mask=0;audio_done=1;audio_started=1;
 Signal(audio_parent,1UL<<audio_parent_signal);return 1;
}
static void audio_dispatch(void){if(!audio_ax_worker())audio_worker();}
