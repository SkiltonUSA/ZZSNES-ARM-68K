#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
typedef uint8_t UBYTE;
typedef int BPTR;
#define UI_PATH 512
#define MEMF_PUBLIC 0
#define ACCESS_READ 0
static char fc_snapshot_path[UI_PATH];
static unsigned fc_view,next_frame_us,fc_last_flip;
static int fc_snapshot_action,direct_locked,fail_rename;
static unsigned fv_width(unsigned v){return v&1?240:320;}
static void*AllocVec(unsigned n,int flags){(void)flags;return malloc(n);}
static void FreeVec(void*p){free(p);}
static BPTR Lock(const char*p,int mode){(void)mode;return access(p,F_OK)==0;}
static void UnLock(BPTR p){(void)p;}
static int DeleteFile(const char*p){return !remove(p);}
static int Rename(const char*a,const char*b){if(fail_rename&&strstr(a,".tmp")){fail_rename=0;return 0;}return !rename(a,b);}
static void mock_puts(const char*s){(void)s;}
static void mock_printf(const char*s,...){(void)s;}
#define puts mock_puts
#define printf mock_printf
#include "snapshot_host.h"
#undef puts
#undef printf
static uint8_t frame[1280*240],saved[SS_BYTES];
static void read_saved(void){FILE*f=fopen(fc_snapshot_path,"rb");assert(f);assert(fread(saved,1,sizeof(saved),f)==sizeof(saved));assert(fgetc(f)==EOF);fclose(f);}
int main(void){char dir[]="/tmp/zzshot-XXXXXX",bak[UI_PATH+8],tmp[UI_PATH+8];uint8_t rom[]={1,2,3,4};assert(mkdtemp(dir));snprintf(fc_snapshot_path,sizeof(fc_snapshot_path),"%s/preview.zzp",dir);
 snprintf(bak,sizeof(bak),"%s.bak",fc_snapshot_path);snprintf(tmp,sizeof(tmp),"%s.tmp",fc_snapshot_path);
 fc_snapshot_init(rom,sizeof(rom));assert(!fc_snapshot_due(299));assert(fc_snapshot_due(300));
 for(unsigned i=0;i<sizeof(frame);i++)frame[i]=i/4;fc_snapshot_store(frame,sizeof(frame),1280,3);assert(!fc_snapshot_due(600));read_saved();assert(ss_valid(saved,sizeof(saved),ss_hash(rom,4),0,4));
 fc_snapshot_auto_done=0;fc_snapshot_init(rom,4);assert(!fc_snapshot_due(300)); /* Keep existing preview. */
 memset(frame,0,sizeof(frame));fc_snapshot_action=1;assert(fc_snapshot_due(1));fail_rename=1;fc_snapshot_store(frame,sizeof(frame),1280,3);read_saved();assert(ss_nonblank(saved)); /* rollback retained old shot */
 fc_snapshot_action=1;direct_locked=1;fc_snapshot_store(frame,sizeof(frame),1280,3);read_saved();assert(ss_nonblank(saved));direct_locked=0;
 fc_snapshot_action=1;fc_snapshot_store(frame,sizeof(frame),1280,3);read_saved();assert(!ss_nonblank(saved));assert(!access(bak,F_OK));
 remove(fc_snapshot_path);fc_snapshot_auto_done=0;fc_snapshot_store(frame,sizeof(frame),1280,3);assert(access(fc_snapshot_path,F_OK)); /* auto skips uniform blank */
 remove(bak);remove(tmp);rmdir(dir);puts("PASS snapshot scheduling, keep existing, manual replacement, blank skip, locked refusal and rename rollback");
}
