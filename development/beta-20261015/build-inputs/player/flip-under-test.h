/* GPL-3.0-or-later. Independent implementation of the XX19c deferred-PAN
 * handoff demonstrated by XANI's ZZPicoDriveMD. See zzpicodrive-review.md.
 * Included by host.c after its timer helpers. No PicoDrive source reused. */
#include "flip_plan.h"
static struct Screen *full_screen;
static struct BitMap *full_allocation;
static volatile UBYTE *full_board;
static struct nf_slots full_slots={0,1,-1,0,0,3}; /* Core init precedes screen open. */
static ULONG full_pitch,full_origin,full_pending_us,full_wait_us;
static UWORD *full_pointer;
static int full_fault,full_attached,full_usedpan,full_origin_known;
static ULONG trial_ack_blank,trial_ack_active;
static void full_write(unsigned reg,UWORD v){*(volatile UWORD *)(full_board+reg)=v;}
static UWORD full_ack_count(void){return *(volatile UWORD *)(full_board+0x46);}
static int full_front(void){return full_screen && IntuitionBase->FirstScreen==full_screen;}
static void full_pan(ULONG offset,int deferred){
 /* Keep unrelated graphics tasks out of this short register transaction.
  * Bitmap lock prevents screen switches while a deferred flip is pending. */
 Forbid();
 full_write(0x44,deferred?1:2);
 full_write(0x10,0);full_write(0x12,0);
 full_write(0x14,(UWORD)(full_pitch/4));full_write(0x30,2);
 full_write(0x0a,(UWORD)(offset>>16));full_write(0x0c,(UWORD)offset);
 Permit();
}
static void full_unlock(void){
 if(direct_locked){p96UnlockBitMap(direct_bitmap,direct_lock);direct_locked=0;}
}
static int full_poll(void){
 int status;
 if(full_slots.pending<0)return !full_fault;
 status=nf_ack(&full_slots,full_ack_count());
 if(status==1){if((*(volatile UWORD *)(full_board+0x4c)&0x20u)!=0)trial_ack_blank++;else trial_ack_active++;fc_flip_sample();full_wait_us+=us()-full_pending_us;full_unlock();return 1;}
 if(status<0 || (ULONG)(us()-full_pending_us)>100000u){
  /* Cancellation clears pending before restoring slot zero. Lock is still
   * held: the screen and origin cannot change underneath this transaction. */
  full_pan(full_origin,0);full_slots.pending=-1;full_fault=1;full_unlock();
  puts("FULLSCREEN flip ACK failed; pending cancelled");return 0;
 }
 return 1;
}
static int full_drain(void){
 while(full_slots.pending>=0){if(!full_poll())return 0;/* Bounded full_poll deadline; do not sleep under bitmap lock. */}
 return !full_fault;
}
static int full_queue(void){
 if(!direct_locked || !full_attached || !nf_queue(&full_slots,full_ack_count()))return 0;
 full_pending_us=us();
 full_usedpan=1;
 full_pan(full_origin+(ULONG)full_slots.pending*240*full_pitch,1);
 return 1;
}
