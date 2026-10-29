static volatile int audio_state_drain,audio_state_drained;
/* GPL-3.0-or-later. 68k-only producer/consumer; no PicoDrive code reused.
 * The worker exclusively owns AHI ports, requests and DMA buffers. It never
 * touches ARM shared memory, graphics, libc or the main task's timer.
 * It is a DOS Process because device initialization may use DOS internally.
 * Ring slots remain immutable until copied by the worker. On this single 68k
 * CPU, short Forbid/Permit sections publish indices and guard task lifetime. */
#include <dos/dostags.h>
#define AUDIO_SLOTS 8u
#define AUDIO_PREBUFFER 6u
struct audio_block { ULONG count; UBYTE data[4096]; };
static struct audio_block *audio_queue;
static struct Task *audio_task,*audio_parent;
static volatile ULONG audio_write,audio_read,audio_worker_mask;
static volatile ULONG audio_started,audio_done,audio_stop,audio_error,audio_force;
static volatile ULONG audio_highwater,audio_submitted,audio_completed;
static LONG audio_parent_signal=-1;
static int audio_opened;
static volatile ULONG audio_pending_samples,audio_feedback_ready;
static unsigned player_audio_feedback(void){
 ULONG queued=0,pending,available;
 if(!audio_opened||!audio_feedback_ready)return 0;
 Forbid();available=audio_write-audio_read;pending=audio_pending_samples;
 for(ULONG k=0;k<available;k++)queued+=audio_queue[(audio_read+k)%AUDIO_SLOTS].count;
 Permit();
 /* Queue plus two linked AHI requests. Coarse completion is conservative:
  * thresholds are a responsiveness choice, not a DMA cursor measurement. */
 ULONG capacity=(AUDIO_SLOTS+6u)*((32040u*fc_frame_us+999999u)/1000000u);
 ULONG total=queued+pending,occupancy=capacity?total*100u/capacity:0;if(occupancy>100)occupancy=100;
 return 256u|occupancy|((total<3u*((32040u*fc_frame_us+999999u)/1000000u))?512u:0u);
}

static void audio_wake(void){
 Forbid();
 if(audio_task&&!audio_done&&audio_worker_mask)Signal(audio_task,audio_worker_mask);
 Permit();
}
static void audio_worker(void){
 struct MsgPort *port=NULL;
 struct AHIRequest *req[2]={NULL,NULL};
 UBYTE *data[2]={NULL,NULL};
 int active[2]={0,0},opened=0,slot=0,primed=0;
 ULONG submitted_count[2]={0,0};
 LONG signal=AllocSignal(-1);ULONG mask=0,i;
 if(signal<0)goto finish;
 mask=1UL<<signal;audio_worker_mask=mask;
 port=CreateMsgPort();if(!port)goto finish;
 for(i=0;i<2;i++){
  req[i]=(struct AHIRequest *)CreateIORequest(port,sizeof(struct AHIRequest));
  data[i]=AllocVec(12288,MEMF_PUBLIC);
  if(!req[i]||!data[i])goto finish;
 }
 req[0]->ahir_Version=4;
 if(OpenDevice(AHINAME,0,(struct IORequest *)req[0],0))goto finish;
 opened=1;
 CopyMem(req[0],req[1],sizeof(*req[1])); /* AHI documented clone after OpenDevice. */
 audio_opened=1;audio_started=1;Signal(audio_parent,1UL<<audio_parent_signal);
 for(;;){
  ULONG available,n,index,blocks,k;int other=slot^1;
  if(audio_stop==1)break;
  {ULONG pending_samples=0;for(i=0;i<2;i++)if(active[i]&&!CheckIO((struct IORequest*)req[i]))pending_samples+=submitted_count[i];
   Forbid();audio_pending_samples=pending_samples;audio_feedback_ready=primed;Permit();}
  Forbid();available=audio_write-audio_read;Permit();
  if(!available)audio_force=0;
  if(!primed&&(available>=AUDIO_PREBUFFER||audio_stop==2||audio_force))primed=1;
  /* Keep the previous request intact until its linked successor has been
   * submitted. Reap a slot only when it is about to be reused. */
  if(primed&&available&&(available>=3||audio_stop==2||audio_force)&&(!active[slot]||CheckIO((struct IORequest *)req[slot]))){
   if(active[slot]){
    profile_wait_io((struct IORequest *)req[slot]);active[slot]=0;
    if(req[slot]->ahir_Std.io_Error){audio_error=1;break;}
    audio_completed+=submitted_count[slot];
   }
   /* Coalesce three immutable NES blocks without dropping/resampling PCM.
    * A short final block is flushed by stop/pause. Never exceed 12288 bytes. */
   blocks=available>=3?3:available;n=0;
   for(k=0;k<blocks;k++){
    ULONG count;
    index=(audio_read+k)%AUDIO_SLOTS;count=audio_queue[index].count;
    if(count>1024){audio_error=1;break;}
    CopyMem(audio_queue[index].data,data[slot]+n*4,count*4);n+=count;
   }
   if(audio_error)break;
   Forbid();audio_read+=blocks;Permit();
   req[slot]->ahir_Std.io_Command=CMD_WRITE;
   req[slot]->ahir_Std.io_Data=data[slot];req[slot]->ahir_Std.io_Length=n*4;
   req[slot]->ahir_Std.io_Offset=0;req[slot]->ahir_Frequency=32040;
   req[slot]->ahir_Type=AHIST_S16S;req[slot]->ahir_Volume=0x10000;
   req[slot]->ahir_Position=0x8000;
   if(active[other]&&CheckIO((struct IORequest *)req[other]))audio_gaps++;
   req[slot]->ahir_Link=active[other]?req[other]:NULL;
   /* AHI can rewrite io_Length from bytes to samples; retain our count. */
   submitted_count[slot]=n;profile_send_io((struct IORequest *)req[slot]);
   active[slot]=1;audio_submitted+=n;slot=other;
   continue;
  }
  if(audio_stop==2&&!available){
   int busy=0;
   for(i=0;i<2;i++)if(active[i]){
    if(!CheckIO((struct IORequest *)req[i]))busy=1;
   }
   if(!busy)break;
  }
  if(audio_state_drain&&!available){
   int busy=0;
   for(i=0;i<2;i++)if(active[i]&&!CheckIO((struct IORequest *)req[i]))busy=1;
   if(!busy){
    for(i=0;i<2;i++)if(active[i]){
     profile_wait_io((struct IORequest *)req[i]);active[i]=0;
     if(req[i]->ahir_Std.io_Error)audio_error=1;
     else audio_completed+=submitted_count[i];
    }
    primed=0;audio_feedback_ready=0;audio_pending_samples=0;audio_state_drained=1;
   }
  }
  Wait(mask|(1UL<<port->mp_SigBit));
 }
finish:
 for(i=0;i<2;i++)if(active[i]){
  int aborted=!CheckIO((struct IORequest *)req[i]);
  if(aborted)AbortIO((struct IORequest *)req[i]);
  profile_wait_io((struct IORequest *)req[i]);
  if(!aborted&&!req[i]->ahir_Std.io_Error)audio_completed+=submitted_count[i];
  else if(!aborted)audio_error=1;
 }
 if(opened)CloseDevice((struct IORequest *)req[0]);
 for(i=0;i<2;i++){
  if(req[i])DeleteIORequest((struct IORequest *)req[i]);
  if(data[i])FreeVec(data[i]);
 }
 if(port)DeleteMsgPort(port);
 if(signal>=0)FreeSignal(signal);
 /* Return to the DOS process wrapper while still forbidden, so the parent
  * cannot unload our entry code before it returns. DOS owns process teardown. */
 Forbid();audio_worker_mask=0;audio_done=1;audio_started=1;
 Signal(audio_parent,1UL<<audio_parent_signal);
}
static void player_audio_close(void){
 if(audio_task){
  ULONG start=us();
  audio_stop=2;audio_wake(); /* Flush even a short stream below prebuffer. */
  while(!audio_done){
   if((ULONG)(us()-start)>2000000u){audio_stop=1;audio_wake();}
   /* Never unload live worker code or free buffers on a timeout. */
   Delay(1);
  }
  audio_task=NULL;
 }
 audio_opened=0;audio_feedback_ready=0;audio_pending_samples=0;
 if(audio_parent_signal>=0){SetSignal(0,1UL<<audio_parent_signal);FreeSignal(audio_parent_signal);audio_parent_signal=-1;}
 if(audio_queue){FreeVec(audio_queue);audio_queue=NULL;}
 printf("AUDIO_TASK queued=%u submitted=%u completed=%u highwater=%u error=%u stopped=%u\n",
  audio_samples,audio_submitted,audio_completed,audio_highwater,audio_error,audio_done);
}
static int player_audio_open(void){
 audio_parent=FindTask(NULL);audio_parent_signal=AllocSignal(-1);
 if(audio_parent_signal<0)goto fail;
 audio_queue=AllocVec(sizeof(*audio_queue)*AUDIO_SLOTS,MEMF_PUBLIC|MEMF_CLEAR);
 if(!audio_queue)goto fail;
 /* AHI may load preferences/drivers using DOS: use a Process, not an Exec
  * Task. No shell file handles or CLI are inherited by the child. */
 audio_task=(struct Task *)CreateNewProcTags(NP_Entry,(ULONG)audio_worker,
  NP_Name,(ULONG)"ZZSnes-Pocket audio",NP_Priority,5,NP_StackSize,32768,
  NP_Input,0,NP_Output,0,NP_CloseInput,FALSE,NP_CloseOutput,FALSE,TAG_DONE);
 if(!audio_task)goto fail;
 while(!audio_started)Wait(1UL<<audio_parent_signal);
 if(!audio_opened)goto fail;
 puts("AUDIO task: AHI unit 0, 32040 Hz stereo; 8 queue slots, 6-frame prebuffer");return 1;
fail:
 player_audio_close();puts("AHI unavailable: running muted; APU timing remains enabled");return 0;
}
static void player_audio_flush(void){audio_force=1;audio_wake();}
static int player_audio_submit(void){
#ifndef EM_AUDIO_EARLY
 audio_wake();
#endif
 return !audio_error;
}
static int player_audio_frame(void){
 ULONG n=ad_get(shared+MB_REPLY,40),start,index,available;
 if(n>1024||audio_error)return 0;
 if(!audio_opened||!n)return 1;
 start=us();
 for(;;){
  Forbid();available=audio_write-audio_read;Permit();
  if(available<AUDIO_SLOTS)break;
  events();if(quit)return 1;
  if(audio_done||audio_error||(ULONG)(us()-start)>500000u)return 0;
  if(!profile_pause(5,1000))return 0;
 }
 audio_wait_us+=us()-start;start=us();index=audio_write%AUDIO_SLOTS;
 syncio->pull(shared+NES_AUDIO,n*4,NULL);
 CopyMem((APTR)(shared+NES_AUDIO),audio_queue[index].data,n*4);
 if(player_muted)memset(audio_queue[index].data,0,n*4);
 audio_queue[index].count=n;audio_copy_us+=us()-start;
 Forbid();audio_write++;available=audio_write-audio_read;
 if(available>audio_highwater)audio_highwater=available;
 Permit();audio_samples+=n;
#ifdef EM_AUDIO_EARLY
 /* Complete PCM is public before the task can run. AHI completion wakes it
  * too; pause/state/stop retain their explicit short-batch flush. */
 if(available>=3)audio_wake();
#endif
 return 1;
}
