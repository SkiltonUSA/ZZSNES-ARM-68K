/* GPL-3.0-or-later. Occasional screenshot from our completed offscreen frame.
 * Called only after ARM ACK and flip drain; no filesystem I/O under P96 lock. */
#include "snapshot_format.h"
static uint32_t fc_snapshot_id,fc_snapshot_id2;
static unsigned fc_snapshot_rom_bytes;
static int fc_snapshot_auto_done;
static void fc_snapshot_init(const UBYTE*rom,unsigned n){FILE*f;UBYTE*p;unsigned count;
 fc_snapshot_id=ss_hash(rom,n);fc_snapshot_rom_bytes=n;fc_snapshot_id2=0;
#ifdef SNAPSHOT_SNES
 fc_snapshot_id2=snes_hash2(rom,n);
#endif
 if(!*fc_snapshot_path){fc_snapshot_auto_done=1;return;}
 p=AllocVec(SS_BYTES+1,MEMF_PUBLIC);if(!p)return;
 f=fopen(fc_snapshot_path,"rb");if(f){count=fread(p,1,SS_BYTES+1,f);fc_snapshot_auto_done=!ferror(f)&&ss_valid(p,count,fc_snapshot_id,fc_snapshot_id2,n);fclose(f);}FreeVec(p);
}
static int fc_snapshot_due(unsigned frame){return *fc_snapshot_path&&(fc_snapshot_action||(!fc_snapshot_auto_done&&frame>=300));}
static void fc_snapshot_store(const UBYTE*source,unsigned span,unsigned pitch,unsigned format){
 UBYTE*p=NULL,*check=NULL;FILE*f;char temp[UI_PATH+8],backup[UI_PATH+8];unsigned width=fv_width(fc_view),y=0,height=240,n;int ok=0,had=0,manual=fc_snapshot_action;
 /* One automatic attempt; F9 always permits replacement. No recurring reads. */
 fc_snapshot_auto_done=1;fc_snapshot_action=0;
 if(direct_locked){puts("SNAPSHOT refused while display locked");return;}
#ifdef SNAPSHOT_SNES
 y=12;height=216;
#endif
 p=AllocVec(SS_BYTES,MEMF_PUBLIC);check=AllocVec(SS_BYTES+1,MEMF_PUBLIC);if(!p||!check)goto end;
 if(!ss_encode(p,source,span,pitch,format,(320-width)/2,y,width,height,fc_snapshot_id,fc_snapshot_id2,fc_snapshot_rom_bytes))goto end;
 if(!manual&&!ss_nonblank(p)){puts("SNAPSHOT automatic frame was blank; use F9 at the title screen");goto end;}
 snprintf(temp,sizeof(temp),"%s.tmp",fc_snapshot_path);snprintf(backup,sizeof(backup),"%s.bak",fc_snapshot_path);
 f=fopen(temp,"wb");if(!f)goto end;ok=fwrite(p,1,SS_BYTES,f)==SS_BYTES;if(fclose(f))ok=0;if(!ok)goto end;
 f=fopen(temp,"rb");if(!f){ok=0;goto end;}n=fread(check,1,SS_BYTES+1,f);ok=!ferror(f)&&ss_valid(check,n,fc_snapshot_id,fc_snapshot_id2,fc_snapshot_rom_bytes)&&!memcmp(p,check,SS_BYTES);fclose(f);if(!ok)goto end;
 {BPTR lock=Lock(fc_snapshot_path,ACCESS_READ);if(lock){had=1;UnLock(lock);}}
 if(had){DeleteFile(backup);if(!Rename(fc_snapshot_path,backup)){ok=0;goto end;}}
 if(!Rename(temp,fc_snapshot_path)){if(had)Rename(backup,fc_snapshot_path);ok=0;goto end;}
 printf("SNAPSHOT saved %s\n",fc_snapshot_path);
end:
 if(!ok)puts("SNAPSHOT not saved; existing preview retained");if(p)FreeVec(p);if(check)FreeVec(check);
 next_frame_us=0;fc_last_flip=0;fflush(stdout);
}
