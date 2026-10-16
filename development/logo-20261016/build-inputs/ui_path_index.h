/* GPL-3.0-or-later. Owned, case-insensitive path index for growing libraries. */
#ifndef UI_PATH_INDEX_H
#define UI_PATH_INDEX_H
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <limits.h>
struct ui_path_slot {char *path;unsigned index;};
struct ui_path_index {struct ui_path_slot *slots;unsigned capacity,count;};
static unsigned ui_path_hash(const char*p){unsigned h=2166136261u;
 while(*p)h=(h^(unsigned)tolower((unsigned char)*p++))*16777619u;return h;}
static void ui_index_free(struct ui_path_index*t){
 for(unsigned i=0;i<t->capacity;i++)free(t->slots[i].path);
 free(t->slots);memset(t,0,sizeof(*t));
}
static int ui_index_find(const struct ui_path_index*t,const char*p){
 if(!t->capacity)return -1;unsigned at=ui_path_hash(p)&(t->capacity-1);
 while(t->slots[at].path){if(!strcasecmp(p,t->slots[at].path))return (int)t->slots[at].index;at=(at+1)&(t->capacity-1);}return -1;
}
static int ui_index_add(struct ui_path_index*t,const char*p,unsigned index){
 int found=ui_index_find(t,p);if(found>=0)return found;
 if(t->count>=t->capacity/2){
  size_t max=(size_t)-1/sizeof(*t->slots);
  if(t->capacity>UINT_MAX/2||t->capacity>max/2)return -1;
  unsigned capacity=t->capacity?t->capacity*2:256;
  struct ui_path_slot*slots=calloc(capacity,sizeof(*slots));if(!slots)return -1;
  for(unsigned i=0;i<t->capacity;i++)if(t->slots[i].path){
   unsigned at=ui_path_hash(t->slots[i].path)&(capacity-1);
   while(slots[at].path)at=(at+1)&(capacity-1);slots[at]=t->slots[i];}
  free(t->slots);t->slots=slots;t->capacity=capacity;
 }
 char*copy=malloc(strlen(p)+1);if(!copy)return -1;strcpy(copy,p);
 unsigned at=ui_path_hash(p)&(t->capacity-1);while(t->slots[at].path)at=(at+1)&(t->capacity-1);
 t->slots[at].path=copy;t->slots[at].index=index;t->count++;return (int)index;
}
#endif
