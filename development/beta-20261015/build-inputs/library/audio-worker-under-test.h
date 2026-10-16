static volatile int audio_state_drain,audio_state_drained;
/* GPL-3.0-or-later. 68k-only producer/consumer; no PicoDrive code reused.
 * The worker exclusively owns AHI ports, requests and DMA buffers. It never
 * touches ARM shared memory, graphics, libc or the main task's timer.
 * It is a DOS Process because device initialization may use DOS internally.
 * Ring slots remain immutable until copied by the worker. On this single 68k
 * CPU, short Forbid/Permit sections publish indices and guard task lifetime. */

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
  Forbid();available=audio_write-audio_read;Permit();
  if(!available)audio_force=0;
  if(!primed&&(available>=AUDIO_PREBUFFER||audio_stop==2||audio_force))primed=1;
  /* Keep the previous request intact until its linked successor has been
   * submitted. Reap a slot only when it is about to be reused. */
  if(primed&&available&&(available>=3||audio_stop==2||audio_force)&&(!active[slot]||CheckIO((struct IORequest *)req[slot]))){
   if(active[slot]){
    WaitIO((struct IORequest *)req[slot]);active[slot]=0;
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
   submitted_count[slot]=n;SendIO((struct IORequest *)req[slot]);
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
     WaitIO((struct IORequest *)req[i]);active[i]=0;
     if(req[i]->ahir_Std.io_Error)audio_error=1;
     else audio_completed+=submitted_count[i];
    }
    primed=0;audio_state_drained=1;
   }
  }
  Wait(mask|(1UL<<port->mp_SigBit));
 }
finish:
 for(i=0;i<2;i++)if(active[i]){
  int aborted=!CheckIO((struct IORequest *)req[i]);
  if(aborted)AbortIO((struct IORequest *)req[i]);
  WaitIO((struct IORequest *)req[i]);
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
