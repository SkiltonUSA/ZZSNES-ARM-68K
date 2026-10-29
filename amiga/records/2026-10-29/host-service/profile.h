#include "cost.h"
static int hc_active;
static struct hc_cost hc_costs[6];
static ULONG hc_send_lock_changed;
static ULONG hc_lock(struct BitMap *bitmap,UBYTE *ri,ULONG size){
 int active=hc_active;ULONG start=us(),result=p96LockBitMap(bitmap,ri,size);
 if(active)hc_add(&hc_costs[0],start,us());return result;
}
static void hc_unlock(struct BitMap *bitmap,ULONG lock){
 int active=hc_active;ULONG start=us();p96UnlockBitMap(bitmap,lock);
 if(active)hc_add(&hc_costs[1],start,us());
}
static void hc_send(struct IORequest *r){
 int active=hc_active,locked=direct_locked;ULONG start=us();SendIO(r);
 if(active){hc_add(&hc_costs[locked?2:3],start,us());if(locked!=direct_locked)hc_send_lock_changed++;}
}
static LONG hc_wait(struct IORequest *r){
 int active=hc_active;ULONG start=us();LONG result=WaitIO(r);
 if(active)hc_add(&hc_costs[4],start,us());return result;
}
static void hc_pcm_copy(APTR src,APTR dst,ULONG n){
 int active=hc_active;ULONG start=us();CopyMem(src,dst,n);
 if(active)hc_add(&hc_costs[5],start,us());
}
static void hc_print(void){
 unsigned i,j;
 for(i=0;i<6;i++){
  struct hc_cost *c=&hc_costs[i];
  printf("HOST_COST kind=%u calls=%u total_us=%u maximum_us=%u bins=",i,c->calls,c->total,c->maximum);
  for(j=0;j<8;j++)printf("%s%u",j?",":"",c->bins[j]);puts("");
 }
 printf("HOST_COST send_lock_changed=%u\n",hc_send_lock_changed);
}
#undef p96LockBitMap
#undef p96UnlockBitMap
#define p96LockBitMap hc_lock
#define p96UnlockBitMap hc_unlock
/* Wrappers above use the original SDK macros. Only later host/audio code uses
 * these aliases; do not instrument the profiler's own calls recursively. */
#undef SendIO
#undef WaitIO
#define SendIO hc_send
#define WaitIO hc_wait
