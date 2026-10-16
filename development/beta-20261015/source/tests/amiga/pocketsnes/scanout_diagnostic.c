/* Exercise the actual diagnostic with a fake clock/OS, including failures.
 * This does not simulate physical scanout or establish a DMA fence. */
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
static int quit,front=1,mode;
static ULONG clock_us,calls;static uint32_t hash;
static unsigned p96GetBitMapAttr(int b,int a){(void)b;(void)a;return 1;}
static int full_front(void){return front;}
static int p96LockBitMap(int b,UBYTE *p,unsigned size){
 struct RenderInfo *r=(void *)p;(void)b;(void)size;assert(!direct_locked);
 r->Memory=arena+0x10000;r->BytesPerRow=mode==4?1279:1280;r->RGBFormat=1;return 1;
}
static void CacheClearE(void *p,ULONG n,int flags){(void)p;(void)n;(void)flags;}
static void full_unlock(void){assert(direct_locked);direct_locked=0;}
static int full_queue(void){
 assert(direct_locked && full_origin_known);calls++;
 uint32_t current=sp_hash(arena+0x10000,1280);
 if(calls==1)hash=current;else assert(hash==current);
 return !(mode==3 && calls==3);
}
static int full_drain(void){full_unlock();return 1;}
static ULONG us(void){return clock_us;}
static void events(void){
 assert(!direct_locked);
 if(mode==1 && clock_us>=1000000)quit=1;
 if(mode==2 && clock_us>=6000000)front=0;
}
static int pause_us(ULONG delay){(void)delay;assert(!direct_locked);clock_us+=100000;return 1;}
/* Target ULONG is 32-bit; mock uses host-sized addresses. Do not interpret
 * target printf's 32-bit format specifiers with mock pointer-sized values. */
static int target_printf(const char *format,...){(void)format;return 0;}
#define printf target_printf
#include "scanout_diagnostic.h"
#undef printf
int main(void){
 for(mode=0;mode<=4;mode++){
  quit=clock_us=calls=direct_locked=0;front=1;
  int ok=sp_run();
  if(mode<3){assert(ok && !direct_locked);}
  else {assert(!ok && direct_locked);full_unlock(); /* Launcher cleanup after ARM stop. */}
  if(mode==0)assert(calls>190 && clock_us>=25000000);
  if(mode==1)assert(quit && calls==1);
  if(mode==2)assert(!front && calls>1);
  if(mode==3)assert(calls==3);
  if(mode==4)assert(calls==0);
 }
 puts("PASS diagnostic: immutable across flips, bounded finish, Q, foreground loss, queue failure, invalid pitch");
}
