#include <assert.h>
#include <unistd.h>
#include "rom_manager.h"
static void put(const char*p,const void*b,size_t n){FILE*f=fopen(p,"wb");assert(f);assert(fwrite(b,1,n,f)==n);assert(!fclose(f));}
int main(void){
 struct ui_model old={0},next={0};unsigned *keep=malloc(20000*sizeof(*keep));int selected=19999;
 ui_defaults(&old);old.clock=900;
 for(unsigned i=0;i<20000;i++){char p[80];snprintf(p,sizeof(p),"USB:ROMs/%u.nes",i);assert(ui_add(&old,p)==(int)i);keep[i]=i<10000?i:i-10000;}
 old.entry[19999].favorite=1;old.entry[19999].played=900;strcpy(old.entry[19999].title,"Keep this title");old.last=20000;
 assert(rm_compact(&old,keep,&next,&selected));assert(old.count==20000&&next.count==10000&&selected==9999&&next.last==10000);
 assert(next.entry[9999].favorite&&next.entry[9999].played==900&&!strcmp(next.entry[9999].title,"Keep this title"));
 assert(!strcmp(next.entry[9999].path,"USB:ROMs/9999.nes"));ui_dispose(&next);
 keep[4]=5;assert(!rm_compact(&old,keep,&next,&selected));ui_dispose(&old);free(keep);
 char folder[]="/tmp/zz-rom-manager-XXXXXX",src[512],dest[512],temp[512];assert(mkdtemp(folder));
 snprintf(src,sizeof(src),"%s/source",folder);snprintf(dest,sizeof(dest),"%s/target",folder);snprintf(temp,sizeof(temp),"%s/Copy.part",folder);
 unsigned char data[100001];for(unsigned i=0;i<sizeof(data);i++)data[i]=(unsigned char)(i*23);
 put(src,data,sizeof(data));struct rm_copy*c=calloc(1,sizeof(*c));assert(c);
 assert(rm_copy_begin(c,src,dest,temp));int rc=0;unsigned steps=0;while(!rc){rc=rm_copy_step(c);assert(++steps<20);}assert(rc==1&&c->bytes==sizeof(data));
 FILE*f=fopen(dest,"rb");assert(f);unsigned char got[sizeof(data)];assert(fread(got,1,sizeof(got),f)==sizeof(got));assert(!memcmp(data,got,sizeof(data)));fclose(f);
 assert(!rm_copy_begin(c,src,dest,temp));assert(!remove(dest));
 assert(rm_copy_begin(c,src,dest,temp));assert(!rm_copy_step(c));rm_copy_cancel(c);assert(access(temp,F_OK)&&access(dest,F_OK));
 assert(rm_copy_begin(c,src,dest,temp));while(!c->verifying)assert(!rm_copy_step(c));
 /* Source mutation during the backup must fail verification, not publish it. */
 data[0]^=1;put(src,data,sizeof(data));assert(rm_copy_step(c)==-1);assert(access(temp,F_OK)&&access(dest,F_OK));
 put(src,"",0);assert(rm_copy_begin(c,src,dest,temp));while(!(rc=rm_copy_step(c))){}assert(rc==1);
 rm_copy_cancel(c);free(c);remove(src);remove(dest);assert(!rmdir(folder));
 puts("PASS 20,000-entry duplicate compaction, metadata and selection retention, invalid plans, verified/empty copies, cancellation, non-overwrite and source-change rejection");
}
