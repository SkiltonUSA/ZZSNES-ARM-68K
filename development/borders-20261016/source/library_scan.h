/* GPL-3.0-or-later. Incremental read-only discovery below one user-selected directory.
 * Never follow directory links, mount disks, or launch discovered software. */
#include <dos/dosextens.h>
#define SCAN_DEPTH 24
static struct {BPTR lock;struct FileInfoBlock *fib;char path[UI_PATH];} scan_stack[SCAN_DEPTH];
static char scan_root_path[UI_PATH];
static unsigned scan_depth,scan_added,scan_steps,scan_reported,scan_skipped,scan_rejected;
static int scan_active,scan_dirty,scan_failed;
static struct ui_path_index scan_existing;
static unsigned scan_existing_count=~0u;
/* Canonical paths avoid duplicate PROGDIR entries without changing save keys. */
static int scan_known(const char*path){
 if(scan_existing_count!=model.count){ui_index_free(&scan_existing);
  for(unsigned i=0;i<model.count;i++){BPTR lock=Lock(model.entry[i].path,ACCESS_READ);char resolved[UI_PATH];
   strcpy(resolved,model.entry[i].path);
   if(lock){if(!NameFromLock(lock,resolved,UI_PATH))strcpy(resolved,model.entry[i].path);UnLock(lock);}
   if(ui_index_add(&scan_existing,resolved,i)<0)return -1;}
  scan_existing_count=model.count;
 }
 return ui_index_find(&scan_existing,path)>=0;
}
static void scan_pop(void){if(scan_depth){--scan_depth;UnLock(scan_stack[scan_depth].lock);FreeDosObject(DOS_FIB,scan_stack[scan_depth].fib);}}
static void scan_stop(void){int active=scan_active;while(scan_depth)scan_pop();scan_active=0;ui_index_free(&scan_existing);scan_existing_count=~0u;if(active)scan_caption(0);}
static int scan_push(const char *path){BPTR lock;struct FileInfoBlock*fib;unsigned n=scan_depth;
 if(n==SCAN_DEPTH||strlen(path)>=UI_PATH)return 0;
 lock=Lock(path,ACCESS_READ);if(!lock)return 0;fib=AllocDosObject(DOS_FIB,NULL);
 if(!fib||!Examine(lock,fib)||fib->fib_DirEntryType<=0){if(fib)FreeDosObject(DOS_FIB,fib);UnLock(lock);return 0;}
 scan_stack[n].lock=lock;scan_stack[n].fib=fib;strcpy(scan_stack[n].path,path);scan_depth++;return 1;
}
static void scan_begin(const char *path){
 if(scan_active)return;
 if(!path||!*path||strlen(path)>=UI_PATH||!scan_push(path)){message("Choose an accessible drive or directory.");return;}
 /* Resolve PROGDIR/assign aliases once, matching the existing-path index. */
 if(!NameFromLock(scan_stack[0].lock,scan_root_path,sizeof(scan_root_path))){scan_stop();message("Could not resolve the scan directory.");return;}
 strcpy(scan_stack[0].path,scan_root_path);scan_added=scan_steps=scan_reported=scan_skipped=scan_rejected=0;scan_failed=0;scan_dirty=1;
 scan_existing_count=~0u;scan_active=1;scan_caption(1);message("Scanning selected directory...");
}
static void scan_step(void){unsigned budget=24;
 while(scan_active&&budget--){
  if(!scan_depth){scan_stop();break;}
  unsigned n=scan_depth-1;struct FileInfoBlock*fib=scan_stack[n].fib;char path[UI_PATH];
  if(!ExNext(scan_stack[n].lock,fib)){scan_pop();continue;}
  strcpy(path,scan_stack[n].path);if(!AddPart(path,fib->fib_FileName,sizeof(path)))continue;
  if(fib->fib_DirEntryType==ST_USERDIR){scan_push(path);continue;}
  if(fib->fib_DirEntryType==ST_FILE&&ui_extension(path)){
   int candidate=library_candidate(path);if(candidate!=1)budget=0;
   if(!candidate){scan_rejected++;continue;}
   int known=scan_known(path);if(known>0){scan_skipped++;continue;}
   unsigned before=model.count;
   if(known<0||ui_add(&model,path)<0){scan_failed=1;scan_stop();message("Scan stopped: not enough memory. Games already found are retained.");break;}
   if(model.count>before){scan_added++;scan_dirty=1;
    if(ui_index_add(&scan_existing,path,before)<0){scan_failed=1;scan_stop();message("Scan stopped: not enough memory. Games already found are retained.");break;}
    scan_existing_count=model.count;}
  }
 }
 if(scan_active&&++scan_steps%20==0){char msg[180];
  if(scan_reported!=scan_added){refresh();scan_reported=scan_added;}
  snprintf(msg,sizeof(msg),"Scanning %.120s - %u games found",scan_root_path,scan_added);message(msg);
 }
 if(!scan_active&&scan_dirty){char msg[160];refresh();snprintf(msg,sizeof(msg),"Import complete: %u added, %u already listed, %u unsupported archives.",scan_added,scan_skipped,scan_rejected);
  if(!prefs_write())message("Games found; settings could not be saved.");else if(!scan_failed)message(msg);scan_dirty=0;}
}
