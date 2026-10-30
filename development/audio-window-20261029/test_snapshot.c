/* Exercise the actual snapshot encoder against scaled private frames. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef uint8_t UBYTE;
typedef FILE *BPTR;
#define MEMF_PUBLIC 0
#define ACCESS_READ 0
#define UI_PATH 256
#define SNAPSHOT_SNES 1
static unsigned fc_picture_height,scale,fc_view,fc_snapshot_action,next_frame_us,fc_last_flip;
static int direct_locked;
static char fc_snapshot_path[UI_PATH]="snapshot-test.zzp";
static unsigned em_width(void){return 320*scale;}
static unsigned fv_width(unsigned opt){(void)opt;return 298;}
static uint32_t snes_hash2(const uint8_t*p,unsigned n){(void)p;return n;}
static void *AllocVec(unsigned n,unsigned flags){(void)flags;return malloc(n);}
static void FreeVec(void*p){free(p);}
static BPTR Lock(const char*p,int mode){(void)mode;return fopen(p,"rb");}
static void UnLock(BPTR p){fclose(p);}
static int DeleteFile(const char*p){return remove(p)==0;}
static int Rename(const char*a,const char*b){return rename(a,b)==0;}
#include "window_scale.h"
#include "snapshot_host.h"
int main(void){
 uint32_t logical[320*240];uint8_t expect[SS_BYTES],actual[SS_BYTES+1];
 for(unsigned y=0;y<240;y++)for(unsigned x=0;x<320;x++)logical[y*320+x]=0xff000000u|(x*1037u)^(y*11713u);
 for(unsigned region=0;region<2;region++){fc_picture_height=region?239:224;
 for(scale=1;scale<=3;scale++)for(unsigned format=3;format<=4;format++){
  unsigned pitch=320*scale*4+16,span=(240*scale-1)*pitch+320*scale*4;
  uint8_t*src=malloc(span);assert(src);
  for(unsigned y=0;y<240;y++)em_window_expand((uint32_t*)(src+y*scale*pitch),pitch/4,logical+y*320,scale);
  assert(ss_encode(expect,(uint8_t*)logical,sizeof(logical),1280,format,11,region?0:8,298,region?239:224,0,0,0));
  fc_snapshot_action=1;fc_snapshot_store(src,span,pitch,format);
  FILE*f=fopen(fc_snapshot_path,"rb");assert(f);assert(fread(actual,1,sizeof(actual),f)==SS_BYTES);fclose(f);
  assert(!memcmp(actual,expect,SS_BYTES));assert(fc_snapshot_action==0);free(src);
 }
 }
 DeleteFile(fc_snapshot_path);DeleteFile("snapshot-test.zzp.bak");
 puts("PASS actual SNES capture: 1x/2x/3x both pixel formats, intact 224/239-row crop and one normalization");
}
