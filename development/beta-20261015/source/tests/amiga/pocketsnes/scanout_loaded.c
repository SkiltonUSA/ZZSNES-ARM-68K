/* Actual loaded-pattern helpers with mock OS; no physical-scanout claim. */
#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "scanout_pattern.h"
typedef uintptr_t ULONG;typedef intptr_t LONG;typedef uint8_t UBYTE;
struct RenderInfo {void *Memory;LONG BytesPerRow;unsigned RGBFormat;};
enum {ND_ARGB32=1,P96BMA_ISONBOARD=1,CACRF_ClearD=1};
static uint8_t arena[0x10000+960*1280];
static UBYTE *full_board=arena;
static int direct_locked,direct_lock,direct_bitmap,full_origin_known;
static ULONG full_origin,full_pitch;
static unsigned direct_native_format=1,direct_format=ND_ARGB32;
static int quit,front=1,fail_queue,bad_pitch;
static ULONG clock_us,calls;static uint32_t hash;
static unsigned p96GetBitMapAttr(int b,int a){(void)b;(void)a;return 1;}
static int full_front(void){return front;}
static int p96LockBitMap(int b,UBYTE *p,unsigned size){
 struct RenderInfo *r=(void *)p;(void)b;(void)size;assert(!direct_locked);
 r->Memory=arena+0x10000;r->BytesPerRow=bad_pitch?1279:1280;r->RGBFormat=1;return 1;
}
static void CacheClearE(void *p,ULONG n,int flags){(void)p;(void)n;(void)flags;}
static void full_unlock(void){assert(direct_locked);direct_locked=0;}
static int full_queue(void){
 assert(direct_locked && full_origin_known);calls++;
 uint32_t current=sp_hash(arena+0x10000,1280);
 if(calls==1)hash=current;else assert(hash==current);
 return !fail_queue;
}
static int full_drain(void){if(direct_locked)full_unlock();return 1;}
static ULONG us(void){return clock_us;}
static void events(void){assert(!direct_locked);}
static int pause_us(ULONG delay){clock_us+=delay;assert(!direct_locked);return 1;}
static int target_printf(const char *format,...){(void)format;return 0;}
#define printf target_printf
#include "scanout_diagnostic.h"
#include "scanout_loaded.h"
#undef printf
static void reset(void){
 if(direct_locked)full_unlock();
 lp_before=lp_frames=lp_present_requests=lp_flips=lp_ready=0;
 quit=fail_queue=bad_pitch=calls=0;front=1;
}
int main(void){
 reset();assert(lp_init() && !direct_locked);
 for(unsigned n=0;n<500;n++){
  assert(lp_present() && direct_locked);
  /* Models Core1 FRAME completion while the preceding flip remains pending.
   * No display writes or graphics calls during this phase. */
  lp_frames++;
  assert(full_drain() && !direct_locked);
 }
 assert(lp_flips==500 && calls==501 && lp_finish() && !direct_locked);
 arena[0x10000+100]^=1;assert(!lp_finish() && !direct_locked);
 reset();assert(lp_init());lp_present_requests=1;assert(!lp_finish());
 reset();assert(lp_init());front=0;assert(!lp_present() && quit && !direct_locked);assert(lp_finish());
 reset();assert(lp_init());fail_queue=1;assert(!lp_present() && direct_locked);full_drain();
 reset();bad_pitch=1;assert(!lp_init() && direct_locked && !lp_ready);full_drain();
 puts("PASS loaded patterns: 500 immutable handoffs, lock retention, corruption/PRESENT detection, foreground and error paths");
}
