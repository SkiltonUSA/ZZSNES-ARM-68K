/* GPL-3.0-or-later. Deterministic fake Exec/AHI exercising the actual worker.
 * Wait injects producer activity and device completion. This checks ownership
 * and failure cleanup, not physical OS scheduling or audio quality. */
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
typedef uint32_t ULONG;
typedef int32_t LONG;
typedef unsigned char UBYTE;
struct Task { int unused; };
struct MsgPort { unsigned mp_SigBit; };
struct IORequest { void *io_Device,*io_Unit,*io_Data; ULONG io_Length,io_Offset,io_Command; int io_Error; };
struct AHIRequest { struct IORequest ahir_Std; ULONG ahir_Version,ahir_Frequency,ahir_Type,ahir_Volume,ahir_Position; struct AHIRequest *ahir_Link; };
#define MEMF_PUBLIC 1
#define CMD_WRITE 1
#define AHINAME "ahi.device"
#define AHIST_S16S 1
static unsigned allocations,signals,opened_devices,fail_at,allocation_calls;
static unsigned active_count,aborts,sends,completions,forbid_depth,produce,total,burst;
static unsigned consumed_blocks,consumed_bytes;
static unsigned force_test;
static unsigned drain_test,drain_seen,unlink_next;
static unsigned fail_io,cancel_after,short_drain;
static ULONG audio_gaps;
static struct IORequest *pending[2];
static void *allocate(size_t size){ if(++allocation_calls==fail_at)return NULL;void *p=calloc(1,size);assert(p);allocations++;return p; }
static void release(void *p){assert(p&&allocations);allocations--;free(p);}
static LONG AllocSignal(int ignored){(void)ignored;signals++;return 2;}
static void FreeSignal(LONG ignored){(void)ignored;assert(signals);signals--;}
static struct MsgPort *CreateMsgPort(void){struct MsgPort *p=allocate(sizeof(*p));if(p)p->mp_SigBit=3;return p;}
static void DeleteMsgPort(struct MsgPort *p){release(p);}
static void *CreateIORequest(struct MsgPort *p,size_t n){assert(p);return allocate(n);}
static void DeleteIORequest(struct IORequest *r){release(r);}
static void *AllocVec(size_t n,int flags){(void)flags;return allocate(n);}
static void FreeVec(void *p){release(p);}
static int OpenDevice(const char *s,int unit,struct IORequest *r,int flags){(void)s;(void)flags;assert(!unit);if(fail_at==99)return 1;opened_devices++;r->io_Device=r;r->io_Unit=r;return 0;}
static void CloseDevice(struct IORequest *r){assert(r&&opened_devices);opened_devices--;}
static void Signal(struct Task *t,ULONG m){(void)t;assert(m);}
static void Forbid(void){forbid_depth++;}
static void Permit(void){assert(forbid_depth);forbid_depth--;}
static void CopyMem(const void *s,void *d,size_t n){memcpy(d,s,n);}
static int CheckIO(struct IORequest *r){for(unsigned i=0;i<2;i++)if(pending[i]==r)return 0;return 1;}
static void WaitIO(struct IORequest *r){assert(CheckIO(r));}
static void AbortIO(struct IORequest *r){for(unsigned i=0;i<2;i++)if(pending[i]==r){pending[i]=NULL;active_count--;aborts++;r->io_Error=-1;return;}assert(0);}
static void SendIO(struct IORequest *r){
 assert(CheckIO(r));assert(active_count<2);assert(r->io_Length>=4&&r->io_Length<=12288);
 for(unsigned i=0;i<r->io_Length;i++){
  assert(((unsigned char *)r->io_Data)[i]==(unsigned char)consumed_blocks);
  if(++consumed_bytes==4*(1+(consumed_blocks*131)%1024)){consumed_bytes=0;consumed_blocks++;}
 }
 assert(((struct AHIRequest *)r)->ahir_Frequency==32040);assert(((struct AHIRequest *)r)->ahir_Type==AHIST_S16S);
 if(sends&&!unlink_next)assert(((struct AHIRequest *)r)->ahir_Link);
 if(unlink_next){assert(!((struct AHIRequest *)r)->ahir_Link);unlink_next=0;}
 for(unsigned i=0;i<2;i++)if(!pending[i]){pending[i]=r;break;}
 active_count++;sends++;
}
static ULONG Wait(ULONG mask);
/* Generated verbatim from production header through audio_worker(). */
#include "audio-worker-under-test.h"
static ULONG Wait(ULONG mask){
 assert(!forbid_depth);assert(mask);
 for(unsigned i=0;i<2;i++)if(pending[i]){
  if(fail_io&&completions==2)pending[i]->io_Error=-1;
  pending[i]->io_Length/=4; /* Physical AHI mutates this field. */
  pending[i]=NULL;active_count--;completions++;break;
 }
 if(cancel_after&&sends>=cancel_after){audio_stop=1;return mask;}
#ifdef FC_STATE_AUDIO
 if(drain_test&&!drain_seen&&produce>=9){
  if(!audio_state_drained){audio_state_drain=audio_force=1;return mask;}
  ULONG samples=0;for(unsigned i=0;i<produce;i++)samples+=1+(i*131)%1024;
  assert(audio_completed==samples&&audio_submitted==samples&&!active_count);
  audio_state_drain=0;drain_seen=unlink_next=1;
 }
#endif
 for(unsigned n=0;n<burst&&produce<total&&audio_write-audio_read<AUDIO_SLOTS;n++){
  struct audio_block *b=&audio_queue[audio_write%AUDIO_SLOTS];
  b->count=1+(produce*131)%1024;
  memset(b->data,(unsigned char)produce,b->count*4);
  produce++;audio_write++;
 }
 if(force_test && produce==4)audio_force=1;
 if(produce==total)audio_stop=2;
 return mask;
}
static void run(unsigned count,unsigned batch,unsigned failure,unsigned cancel,unsigned ioerror){
 allocations=signals=opened_devices=allocation_calls=active_count=aborts=sends=completions=forbid_depth=produce=0;
 consumed_blocks=consumed_bytes=0;fail_at=failure;total=count;burst=batch;cancel_after=cancel;fail_io=ioerror;short_drain=count<3;
 audio_write=audio_read=audio_worker_mask=audio_started=audio_done=audio_stop=audio_error=audio_force=0;
 audio_submitted=audio_completed=audio_gaps=0;audio_opened=0;audio_parent_signal=1;
 drain_seen=unlink_next=0;
#ifdef FC_STATE_AUDIO
 audio_state_drain=audio_state_drained=0;
#endif
 memset(pending,0,sizeof pending);
 audio_queue=calloc(AUDIO_SLOTS,sizeof(*audio_queue));assert(audio_queue);
 audio_worker();
 assert(audio_done&&audio_started&&forbid_depth==1);forbid_depth=0;
 assert(!allocations&&!signals&&!opened_devices&&!active_count);
 if(!failure&&!cancel&&!ioerror){
  assert(consumed_blocks==count&&!consumed_bytes&&sends<=count&&audio_read==audio_write&&!audio_error&&!aborts);
  ULONG samples=0;for(unsigned i=0;i<count;i++)samples+=1+(i*131)%1024;
  assert(audio_submitted==samples&&audio_completed==samples);
 }
 if(ioerror)assert(audio_error);
 if(failure)assert(!sends);
 free(audio_queue);audio_queue=NULL;
}
int main(void){
 (void)audio_task;(void)audio_highwater;
 run(1,1,0,0,0);run(2,1,0,0,0);run(3,1,0,0,0);run(601,8,0,0,0); /* Stop must flush below the startup prebuffer threshold. */
 run(600,8,0,0,0);run(600,1,0,0,0); /* Wrap, bursts, starvation, exact PCM. */
 force_test=1;run(31,1,0,0,0);force_test=0;
 for(unsigned i=1;i<=5;i++)run(6,3,i,0,0);run(6,3,99,0,0);
 run(100,8,0,5,0);run(100,8,0,0,1);
#ifdef FC_STATE_AUDIO
 drain_test=1;run(61,1,0,0,0);assert(drain_seen);drain_test=0;
 puts("PASS save-state AHI drain reaps both requests, resumes without stale link, preserves every PCM sample");
#endif
 puts("PASS actual audio worker: short flush, 1838 ordered blocks and forced partial flush, ring wrap, allocation/open failures, cancellation, device error, resource cleanup");
}
