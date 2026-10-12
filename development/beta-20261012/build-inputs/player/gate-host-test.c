
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
typedef uint8_t UBYTE;typedef uint16_t UWORD;typedef uint32_t ULONG;
struct Screen {int dummy;};struct BitMap {int dummy;};
struct Intuition {struct Screen *FirstScreen;} intuition,*IntuitionBase=&intuition;
static int direct_locked,direct_lock;static struct BitMap *direct_bitmap;
static unsigned unlocks;static ULONG now;
static ULONG us(void){return now++;}
static void Forbid(void){}static void Permit(void){}
static void fc_flip_sample(void){}
static void p96UnlockBitMap(struct BitMap *b,int lock){(void)b;(void)lock;unlocks++;}
#include "gate-under-test.h"
static UWORD registers[256];
static void poll_blank(int blank){registers[0x4c/2]=blank?0x20:0;now+=100;assert(full_poll());}
int main(void){
 full_board=(UBYTE*)registers;full_attached=1;full_pitch=1280;full_origin=0x10000;
 nf_init(&full_slots);
 for(unsigned n=0;n<200000;n++){
  direct_locked=1;assert(full_queue());assert(!full_queue());
  assert(full_poll() && direct_locked);
  registers[0x46/2]++;registers[0x4c/2]=(n&1)?0x20:0;
  assert(full_poll());assert(full_slots.pending<0);
  if(SCANOUT_GATE_MODE==2 || (SCANOUT_GATE_MODE==1 && (n&1))){
   assert(direct_locked && full_gate.phase && !full_queue());
   poll_blank(0);
   if(SCANOUT_GATE_MODE==2){assert(direct_locked);poll_blank(1);assert(direct_locked);poll_blank(0);}
  }
  assert(!direct_locked && !full_gate.phase && full_slots.flips==n+1 && unlocks==n+1);
 }
 assert(trial_ack_blank==100000 && trial_ack_active==100000);
 assert(gate_count==(SCANOUT_GATE_MODE==2?200000:SCANOUT_GATE_MODE==1?100000:0));
 direct_locked=1;assert(full_queue());now=full_pending_us+100001;
 assert(!full_poll() && full_fault && !direct_locked && full_slots.pending<0);
 assert(registers[0x44/2]==2);
 full_fault=0;nf_init(&full_slots);direct_locked=1;assert(full_queue());registers[0x46/2]+=2;
 assert(!full_poll() && full_fault && !direct_locked && full_slots.pending<0);
 if(SCANOUT_GATE_MODE){
  full_fault=0;nf_init(&full_slots);direct_locked=1;assert(full_queue());
  registers[0x46/2]++;registers[0x4c/2]=0x20;assert(full_poll());assert(full_gate.phase && direct_locked);
  assert(!full_drain());assert(full_fault && !direct_locked && !full_gate.phase && gate_timeouts==1);
  full_fault=0;nf_init(&full_slots);direct_locked=1;assert(full_queue());
  registers[0x46/2]++;assert(full_poll());assert(full_gate.phase);registers[0x46/2]++;
  assert(!full_poll() && full_fault && !direct_locked && !full_gate.phase);
 }
 puts("PASS actual host gate: 200000 flips, locked until required edges, wrap, timeout, cancellation and unexpected ACK during gate");
}
