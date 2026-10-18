/* GPL-3.0-or-later. Library-only duplicate removal and incremental verified copy. */
#ifndef ROM_MANAGER_H
#define ROM_MANAGER_H
#include <errno.h>
#include "ui_model.h"

/* keep[i] is the first identical entry, or i. Never alters any ROM/save file.
 * Build a replacement model so a failed preference write can restore the old one. */
static int rm_compact(const struct ui_model *old, const unsigned *keep,
                      struct ui_model *next, int *selection) {
 unsigned *indices=malloc((old->count?old->count:1)*sizeof(*indices));
 *next=*old;next->entry=NULL;memset(&next->paths,0,sizeof(next->paths));
 next->count=next->capacity=0;
 if(!indices)return 0;
 next->entry=malloc((old->count?old->count:1)*sizeof(*next->entry));
 if(!next->entry){free(indices);return 0;}
 next->capacity=old->count;
 for(unsigned i=0;i<old->count;i++){
  if(keep[i]>i||keep[keep[i]]!=keep[i]){free(indices);ui_dispose(next);return 0;}
  if(keep[i]==i){indices[i]=next->count;next->entry[next->count++]=old->entry[i];}
  else{
   struct ui_entry *entry=&next->entry[indices[keep[i]]];indices[i]=indices[keep[i]];
   entry->favorite|=old->entry[i].favorite;
   if(entry->played<old->entry[i].played)entry->played=old->entry[i].played;
   if(!*entry->title)strcpy(entry->title,old->entry[i].title);
   if(!*entry->art)strcpy(entry->art,old->entry[i].art);
  }
 }
 if(*selection>=0&&(unsigned)*selection<old->count)*selection=(int)indices[*selection];
 next->last=old->last&&old->last<=old->count?indices[old->last-1]+1:0;
 free(indices);return 1;
}

#define RM_COPY_BLOCK 32768
struct rm_copy {FILE *in,*out;int verifying;unsigned long bytes;
 char source[UI_PATH],target[UI_PATH],temporary[UI_PATH];
 unsigned char a[RM_COPY_BLOCK],b[RM_COPY_BLOCK];};
static void rm_copy_cancel(struct rm_copy*c){
 if(c->in)fclose(c->in);if(c->out)fclose(c->out);c->in=c->out=NULL;
 if(*c->temporary)remove(c->temporary);
}
static int rm_copy_begin(struct rm_copy*c,const char*source,const char*target,const char*temporary){
 FILE *exists;
 memset(c,0,sizeof(*c));
 if(strlen(source)>=UI_PATH||strlen(target)>=UI_PATH||strlen(temporary)>=UI_PATH)return 0;
 /* The caller owns a newly created backup directory. Never replace a backup. */
 errno=0;exists=fopen(target,"rb");if(exists){fclose(exists);return 0;}if(errno!=ENOENT)return 0;
 strcpy(c->source,source);strcpy(c->target,target);strcpy(c->temporary,temporary);
 c->in=fopen(source,"rb");if(!c->in)return 0;
 c->out=fopen(temporary,"wb");if(!c->out){rm_copy_cancel(c);return 0;}return 1;
}
/* 0 pending, 1 verified and renamed, -1 failure. One block per UI tick. */
static int rm_copy_step(struct rm_copy*c){
 size_t n=fread(c->a,1,sizeof(c->a),c->in);
 if(ferror(c->in))goto fail;
 if(c->verifying){
  size_t m=fread(c->b,1,sizeof(c->b),c->out);
  if(ferror(c->out)||n!=m||memcmp(c->a,c->b,n))goto fail;
  if(n)return 0;
  int a=fclose(c->in),b=fclose(c->out);c->in=c->out=NULL;
  if(a||b||rename(c->temporary,c->target))goto fail;
  c->temporary[0]=0;return 1;
 }
 if(n){if(fwrite(c->a,1,n,c->out)!=n)goto fail;c->bytes+=n;return 0;}
 {int a=fclose(c->in),b=fclose(c->out);c->in=c->out=NULL;if(a||b)goto fail;}
 c->in=fopen(c->source,"rb");c->out=fopen(c->temporary,"rb");
 if(!c->in||!c->out)goto fail;c->verifying=1;return 0;
fail:rm_copy_cancel(c);return -1;
}
#endif
