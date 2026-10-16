/* GPL-3.0-or-later. Included after fullscreen.h, with C0's existing lifecycle.
 * Initialise four images once, then PAN between them without pixel writes.
 * Core1 has initialised but receives no FRAME/PRESENT requests; no audio.
 * Uses the screen's real P96 bitmap and revalidates after every lock.
 * PAN ACK remains an observation, not proof of scanout completion. */
#include "scanout_pattern.h"
static int sp_lock(struct RenderInfo *ri){
 ULONG memory,span;
 if(!full_front() || !p96GetBitMapAttr(direct_bitmap,P96BMA_ISONBOARD))return 0;
 memset(ri,0,sizeof(*ri));full_origin_known=0;
 direct_lock=p96LockBitMap(direct_bitmap,(UBYTE *)ri,sizeof(*ri));direct_locked=1;
 memory=(ULONG)ri->Memory;
 if(!memory || (memory&3u) || ri->BytesPerRow<1280 || ri->BytesPerRow>4096 ||
    (ri->BytesPerRow&3) || (ULONG)ri->RGBFormat!=direct_native_format)return 0;
 span=960*(ULONG)ri->BytesPerRow;
 if(memory<(ULONG)full_board+0x10000u || memory>(ULONG)full_board+0x4000000u-span)return 0;
 full_origin=memory-(ULONG)full_board-0x10000u;full_pitch=ri->BytesPerRow;
 full_origin_known=1;return 1;
}
static int sp_run(void){
 struct RenderInfo ri;ULONG hash_before,hash_after,start,next,now,flips=0;
 if(!sp_lock(&ri))return 0;
 sp_fill(ri.Memory,ri.BytesPerRow,direct_format==ND_ARGB32);
 CacheClearE(ri.Memory,960*(ULONG)ri.BytesPerRow,CACRF_ClearD);
 hash_before=sp_hash(ri.Memory,ri.BytesPerRow);
 /* First deferred flip establishes one of the completed pattern images. */
 if(!full_queue() || !full_drain())return 0;
 puts("PATTERN static: 5 seconds; then diagonal motion for 20 seconds; Q/Esc quits");fflush(stdout);
 start=us();
 while(!quit && (ULONG)(us()-start)<5000000u){
  events();if(!full_front()){quit=1;break;}if(!quit && !pause_us(1000))return 0;
 }
 puts("PATTERN moving: immutable buffers; no SNES frames, ARM presentation or audio");fflush(stdout);
 start=next=us();
 while(!quit && (ULONG)(us()-start)<20000000u){
  events();if(!full_front()){quit=1;break;}if(quit)break;
  now=us();
  if((LONG)(now-next)<0){if(!pause_us(1000))return 0;continue;}
  if(!sp_lock(&ri) || !full_queue() || !full_drain())return 0;
  flips++;next=now+16800u; /* Nominal pacing only, not a scanout fence. */
 }
 /* Do not reacquire/pan over another screen after loss of foreground. */
 if(!full_front()){puts("PATTERN cancelled: foreground lost; integrity check skipped");return 1;}
 if(!sp_lock(&ri))return 0;
 CacheClearE(ri.Memory,960*(ULONG)ri.BytesPerRow,CACRF_ClearD);
 hash_after=sp_hash(ri.Memory,ri.BytesPerRow);full_unlock();
 printf("PATTERN_END flips=%u before=%08x after=%08x immutable=%u cancelled=%u\n",
  flips,hash_before,hash_after,(ULONG)(hash_before==hash_after),(ULONG)quit);
 return hash_before==hash_after;
}
