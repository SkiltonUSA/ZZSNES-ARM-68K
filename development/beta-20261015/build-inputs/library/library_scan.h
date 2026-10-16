/* GPL-3.0-or-later. Incremental read-only discovery on mounted DOS volumes.
 * Never follow directory links, mount disks, or launch discovered software. */
#include <dos/dosextens.h>
#define SCAN_DEPTH 24
#define SCAN_VOLUMES 64
static struct {BPTR lock;struct FileInfoBlock *fib;char path[UI_PATH];} scan_stack[SCAN_DEPTH];
static char scan_roots[SCAN_VOLUMES][UI_PATH],scan_seen[SCAN_VOLUMES][UI_PATH];
static unsigned scan_count,scan_root,scan_depth,scan_added,scan_seen_count,scan_ticks,scan_steps,scan_reported;
static int scan_active,scan_dirty;
static char scan_existing[UI_MAX][UI_PATH];
static unsigned scan_existing_count=~0u;
/* Resolve assignments such as PROGDIR without rewriting persisted save keys. */
static int scan_known(const char*path){
 if(scan_existing_count!=model.count){
  for(unsigned i=0;i<model.count;i++){BPTR lock=Lock(model.entry[i].path,ACCESS_READ);
   strcpy(scan_existing[i],model.entry[i].path);
   if(lock){char resolved[UI_PATH];if(NameFromLock(lock,resolved,UI_PATH))strcpy(scan_existing[i],resolved);UnLock(lock);}}
  scan_existing_count=model.count;
 }
 for(unsigned i=0;i<scan_existing_count;i++)if(!strcasecmp(path,scan_existing[i]))return 1;
 return 0;
}
static void scan_pop(void){if(scan_depth){--scan_depth;UnLock(scan_stack[scan_depth].lock);FreeDosObject(DOS_FIB,scan_stack[scan_depth].fib);}}
static void scan_stop(void){int active=scan_active;while(scan_depth)scan_pop();scan_active=0;if(active)scan_caption(0);}
static int scan_push(const char *path){BPTR lock;struct FileInfoBlock*fib;unsigned n=scan_depth;
 if(n==SCAN_DEPTH||strlen(path)>=UI_PATH)return 0;
 lock=Lock(path,ACCESS_READ);if(!lock)return 0;fib=AllocDosObject(DOS_FIB,NULL);
 if(!fib||!Examine(lock,fib)||fib->fib_DirEntryType<=0){if(fib)FreeDosObject(DOS_FIB,fib);UnLock(lock);return 0;}
 scan_stack[n].lock=lock;scan_stack[n].fib=fib;strcpy(scan_stack[n].path,path);scan_depth++;return 1;
}
static void scan_begin(int force){struct DosList*dl;unsigned count=0;char current[SCAN_VOLUMES][UI_PATH];
 if(scan_active)return;
 dl=LockDosList(LDF_VOLUMES|LDF_READ);
 if(dl){while((dl=NextDosEntry(dl,LDF_VOLUMES))&&count<SCAN_VOLUMES){
  const UBYTE*b=(const UBYTE*)BADDR(dl->dol_Name);unsigned len=b?b[0]:0;
  if(dl->dol_Task&&len&&len<UI_PATH-2){memcpy(current[count],b+1,len);current[count][len]=':';current[count][len+1]=0;count++;}}
  UnLockDosList(LDF_VOLUMES|LDF_READ);}
 scan_count=scan_root=scan_added=scan_steps=scan_reported=0;scan_existing_count=~0u;
 for(unsigned i=0;i<count;i++){unsigned j=0;for(;j<scan_seen_count;j++)if(!strcasecmp(current[i],scan_seen[j]))break;
  if(force||j==scan_seen_count)strcpy(scan_roots[scan_count++],current[i]);}
 /* Offer RAM results first; a mounted optical disc must not hide quick finds. */
 {BPTR ram=Lock("RAM:",ACCESS_READ);char root[UI_PATH],temp[UI_PATH];
  if(ram){if(NameFromLock(ram,root,sizeof(root)))for(unsigned i=0;i<scan_count;i++)if(!strcasecmp(root,scan_roots[i])){strcpy(temp,scan_roots[0]);strcpy(scan_roots[0],scan_roots[i]);strcpy(scan_roots[i],temp);break;}UnLock(ram);}}
 scan_seen_count=count;memcpy(scan_seen,current,count*UI_PATH);scan_active=scan_count!=0;scan_caption(scan_active);
 if(scan_active)message("Scanning mounted drives...");
}
static void scan_step(void){unsigned budget=24;
 while(scan_active&&budget--){
  if(!scan_depth){if(scan_root==scan_count){scan_active=0;scan_caption(0);break;}scan_push(scan_roots[scan_root++]);continue;}
  unsigned n=scan_depth-1;struct FileInfoBlock*fib=scan_stack[n].fib;char path[UI_PATH];
  if(!ExNext(scan_stack[n].lock,fib)){scan_pop();continue;}
  strcpy(path,scan_stack[n].path);if(!AddPart(path,fib->fib_FileName,sizeof(path)))continue;
  if(fib->fib_DirEntryType==ST_USERDIR){scan_push(path);continue;}
  if(fib->fib_DirEntryType==ST_FILE&&ui_extension(path)&&!scan_known(path)){
   unsigned before=model.count;if(ui_add(&model,path)>=0&&model.count>before){strcpy(scan_existing[before],path);scan_existing_count=model.count;scan_added++;scan_dirty=1;}
   if(model.count==UI_MAX){scan_stop();message("Library full. Remove entries before scanning again.");}
  }
 }
 if(scan_active&&++scan_steps%20==0){char msg[180];
  if(scan_reported!=scan_added){refresh();scan_reported=scan_added;}
  snprintf(msg,sizeof(msg),"Scanning %.120s - %u games found",scan_roots[scan_root?scan_root-1:0],scan_added);message(msg);
 }
 if(!scan_active&&scan_dirty){char msg[80];refresh();snprintf(msg,sizeof(msg),"Added %u games from mounted drives.",scan_added);
  message(prefs_write()?msg:"Games found; settings could not be saved.");scan_dirty=0;}
}
