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
 {"RAM Disk:ROMs",2,{"Existing.nes","New.NES","Packed.zip","Other.zip",NULL}},
 {"RAM Disk:ROMs/Existing.nes",-3,{NULL}},
 {"RAM Disk:ROMs/New.NES",-3,{NULL}},
 {"RAM Disk:ROMs/Packed.zip",-3,{NULL}},
 {"RAM Disk:ROMs/Other.zip",-3,{NULL}},
 {"RAM Disk:Link",3,{NULL}},
 {"RAM Disk:notes.txt",-3,{NULL}},
 {"USB:",2,{"USB.nes",NULL}},
 {"USB:USB.nes",-3,{NULL}},
};
static unsigned locks,allocated,refreshes,writes;
static int resolve_failure;
static BPTR Lock(const char*p,int mode){(void)mode;if(!strcmp(p,"PROGDIR:"))p="RAM Disk:ROMs";if(!strcmp(p,"RAM:"))p="RAM Disk:";if(!strcmp(p,"PROGDIR:Existing.nes"))p="RAM Disk:ROMs/Existing.nes";
 for(unsigned i=0;i<sizeof(nodes)/sizeof(*nodes);i++)if(!strcasecmp(p,nodes[i].path)){locks++;return(BPTR)&nodes[i];}return 0;}
static void UnLock(BPTR p){assert(p&&locks);locks--;}
static struct FileInfoBlock*AllocDosObject(int type,void*args){(void)type;(void)args;allocated++;return calloc(1,sizeof(struct FileInfoBlock));}
static void FreeDosObject(int type,void*p){(void)type;assert(allocated);allocated--;free(p);}
static int Examine(BPTR p,struct FileInfoBlock*f){f->fib_DirEntryType=((struct node*)p)->type;f->cursor=0;return 1;}
static int NameFromLock(BPTR p,char*out,unsigned cap){if(resolve_failure){memset(out,'x',cap);return 0;}return snprintf(out,cap,"%s",((struct node*)p)->path)<(int)cap;}
static int AddPart(char*p,const char*n,unsigned cap){size_t used=strlen(p);if(used+strlen(n)+2>=cap)return 0;if(used&&p[used-1]!=':')strcat(p,"/");strcat(p,n);return 1;}
static int ExNext(BPTR p,struct FileInfoBlock*f){struct node*n=(void*)p;const char*child=n->children[f->cursor++];char path[512];BPTR q;
 if(!child)return 0;strcpy(path,n->path);assert(AddPart(path,child,sizeof(path)));q=Lock(path,0);assert(q);f->fib_DirEntryType=((struct node*)q)->type;strcpy(f->fib_FileName,child);UnLock(q);return 1;}
static struct ui_model model;
static int caption;
static void scan_caption(int active){caption=active;}
static void message(const char*s){(void)s;}
static void refresh(void){refreshes++;}
static int prefs_write(void){writes++;return 1;}
static int ui_extension(const char*s){unsigned n=strlen(s);return n>4&&(!strcasecmp(s+n-4,".nes")||!strcasecmp(s+n-4,".zip"));}
static int library_candidate(const char*s){if(strstr(s,"Other.zip"))return 0;return strstr(s,"Packed.zip")?2:1;}
/* SDK types already declared by the mock above. */
#define SCAN_TEST_TYPES
#include "library_scan.h"
static void finish(void){unsigned rounds=0;while(scan_active){scan_step();assert(++rounds<100);}assert(!locks&&!allocated&&!caption);}
int main(void){
 ui_defaults(&model);assert(!model.autoscan);assert(ui_add(&model,"PROGDIR:Existing.nes")==0);
 /* Idle ticks and new mounted volumes must never discover anything. */
 for(unsigned i=0;i<250;i++)scan_step();assert(model.count==1&&!writes&&!locks);
 scan_begin("RAM Disk:ROMs");finish();assert(model.count==3&&writes==1);
 assert(!strcmp(model.entry[0].path,"PROGDIR:Existing.nes"));
 scan_begin("PROGDIR:");finish();assert(model.count==3);
 scan_begin("RAM Disk:");finish();assert(model.count==3); /* dedup, links excluded */
 for(unsigned i=0;i<250;i++)scan_step();assert(model.count==3);
 scan_begin("USB:");finish();assert(model.count==4);
 scan_begin("Missing:");assert(!scan_active&&!locks&&!allocated);
 scan_begin("RAM Disk:notes.txt");assert(!scan_active&&!locks&&!allocated);
 scan_begin("");assert(!scan_active);
 scan_begin("RAM Disk:");scan_stop();scan_step();assert(!locks&&!allocated);
 resolve_failure=1;scan_existing_count=~0u;assert(scan_known("PROGDIR:Existing.nes")==1);resolve_failure=0;scan_stop();
 ui_dispose(&model);
 puts("PASS explicit directory scans, idle inactivity, recursion, dedup, ZIP filtering, invalid roots, cancellation and DOS resource release");
}
