#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "ui_model.h"
#include "mock/dos/dosextens.h"
struct node{const char*path;int type;const char*children[8];};
static struct node nodes[]={
 {"System:",2,{NULL}},
 {"RAM Disk:",2,{"ROMs","Link","notes.txt",NULL}},
 {"RAM Disk:ROMs",2,{"Existing.nes","New.NES",NULL}},
 {"RAM Disk:ROMs/Existing.nes",-3,{NULL}},
 {"RAM Disk:ROMs/New.NES",-3,{NULL}},
 {"RAM Disk:Link",3,{NULL}},
 {"RAM Disk:notes.txt",-3,{NULL}},
 {"USB:",2,{"USB.nes",NULL}},
 {"USB:USB.nes",-3,{NULL}},
};
static unsigned locks,allocated,refreshes,writes;
static BPTR Lock(const char*p,int mode){(void)mode;if(!strcmp(p,"RAM:"))p="RAM Disk:";if(!strcmp(p,"PROGDIR:Existing.nes"))p="RAM Disk:ROMs/Existing.nes";
 for(unsigned i=0;i<sizeof(nodes)/sizeof(*nodes);i++)if(!strcasecmp(p,nodes[i].path)){locks++;return(BPTR)&nodes[i];}return 0;}
static void UnLock(BPTR p){assert(p&&locks);locks--;}
static struct FileInfoBlock*AllocDosObject(int type,void*args){(void)type;(void)args;allocated++;return calloc(1,sizeof(struct FileInfoBlock));}
static void FreeDosObject(int type,void*p){(void)type;assert(allocated);allocated--;free(p);}
static int Examine(BPTR p,struct FileInfoBlock*f){f->fib_DirEntryType=((struct node*)p)->type;f->cursor=0;return 1;}
static int NameFromLock(BPTR p,char*out,unsigned cap){return snprintf(out,cap,"%s",((struct node*)p)->path)<(int)cap;}
static int AddPart(char*p,const char*n,unsigned cap){size_t used=strlen(p);if(used+strlen(n)+2>=cap)return 0;if(used&&p[used-1]!=':')strcat(p,"/");strcat(p,n);return 1;}
static int ExNext(BPTR p,struct FileInfoBlock*f){struct node*n=(void*)p;const char*child=n->children[f->cursor++];char path[512];BPTR q;
 if(!child)return 0;strcpy(path,n->path);assert(AddPart(path,child,sizeof(path)));q=Lock(path,0);assert(q);f->fib_DirEntryType=((struct node*)q)->type;strcpy(f->fib_FileName,child);UnLock(q);return 1;}
static unsigned char names[][20]={{6,'S','y','s','t','e','m'},{8,'R','A','M',' ','D','i','s','k'},{3,'U','S','B'}};
static struct DosList volumes[4];
static struct DosList*LockDosList(int flags){(void)flags;return volumes;}
static struct DosList*NextDosEntry(struct DosList*d,int flags){(void)flags;return d->next;}
static void UnLockDosList(int flags){(void)flags;}
static struct ui_model model;
static int caption;
static void scan_caption(int active){caption=active;}
static void message(const char*s){(void)s;}
static void refresh(void){refreshes++;}
static int prefs_write(void){writes++;return 1;}
static int ui_extension(const char*s){unsigned n=strlen(s);return n>4&&!strcasecmp(s+n-4,".nes");}
/* SDK types already declared by the mock above. */
#define SCAN_TEST_TYPES
#include "library_scan.h"
static void finish(void){unsigned rounds=0;while(scan_active){scan_step();assert(++rounds<100);}assert(!locks&&!allocated&&!caption);}
int main(void){
 for(unsigned i=0;i<3;i++){volumes[i].next=&volumes[i+1];volumes[i+1].dol_Name=(BPTR)names[i];volumes[i+1].dol_Task=(void*)1;}
 volumes[3].dol_Task=NULL; // Remembered, unmounted USB volume: no request to insert it.
 ui_defaults(&model);assert(ui_add(&model,"PROGDIR:Existing.nes")==0);
 scan_begin(0);assert(scan_count==2);finish();assert(model.count==2&&writes==1);
 assert(!strcmp(model.entry[0].path,"PROGDIR:Existing.nes")); // Preserve old save key.
 scan_begin(0);assert(!scan_active);
 volumes[3].dol_Task=(void*)1;scan_begin(0);assert(scan_count==1);finish();assert(model.count==3&&writes==2);
 scan_begin(1);finish();assert(model.count==3&&writes==2);
 scan_begin(1);assert(scan_push("RAM Disk:"));scan_stop();assert(!locks&&!allocated);
 puts("PASS mounted/unmounted volumes, hot-plug discovery, PROGDIR deduplication, link exclusion, cancellation and DOS resource release");
}
