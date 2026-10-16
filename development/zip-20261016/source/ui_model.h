/* GPL-3.0-or-later. Bounded, host-testable library/preferences model.
 * Hex-encoded paths permit spaces, quotes and tabs without shell ambiguity. */
#ifndef FC_UI_MODEL_H
#define FC_UI_MODEL_H
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <limits.h>
#include "ui_path_index.h"
#define UI_PATH 512
#define UI_TITLE 128
#ifndef UI_CONFIG_PREFIX
#define UI_CONFIG_PREFIX "ZZNESUI"
#endif
struct ui_entry {char path[UI_PATH],title[UI_TITLE],art[UI_PATH];unsigned favorite,played;};
struct ui_model {unsigned count,clock,monitor,theme,mask,vmask,slot,mute,fire,last,filter,windowed,scanlines,fps,optimise,autoscan;unsigned capacity;struct ui_entry *entry;struct ui_path_index paths;};
/* Models are zero-initialised before first use. Reset releases owned storage. */
static void ui_dispose(struct ui_model*m){free(m->entry);ui_index_free(&m->paths);memset(m,0,sizeof(*m));}
static void ui_defaults(struct ui_model *m){ui_dispose(m);m->optimise=1;m->autoscan=1;
#ifdef LIBRARY_SNES
 m->theme=2;
#else
 m->mask=1;
#endif
}
static int ui_path_ok(const char *p){unsigned n=strlen(p);return n>0&&n<UI_PATH&&!strchr(p,'\n')&&!strchr(p,'\r');}
static int ui_add(struct ui_model *m,const char *p){int found;unsigned capacity;struct ui_entry*entries;
 if(!ui_path_ok(p))return -1;
 if(m->paths.count!=m->count){ui_index_free(&m->paths);
  for(unsigned i=0;i<m->count;i++)if(ui_index_add(&m->paths,m->entry[i].path,i)<0)return -1;}
 found=ui_index_find(&m->paths,p);if(found>=0)return found;
 if(m->count==m->capacity){
  size_t max=(size_t)-1/sizeof(*entries);if(max>INT_MAX)max=INT_MAX;
  if(m->capacity>=max)return -1;capacity=m->capacity?m->capacity:64;
  capacity=capacity>max/2?(unsigned)max:capacity*2;
  entries=realloc(m->entry,(size_t)capacity*sizeof(*entries));if(!entries)return -1;
  m->entry=entries;m->capacity=capacity;
 }
 if(ui_index_add(&m->paths,p,m->count)<0)return -1;
 memset(&m->entry[m->count],0,sizeof(*m->entry));strcpy(m->entry[m->count].path,p);return (int)m->count++;
}
static void ui_remove(struct ui_model*m,unsigned index){
 memmove(&m->entry[index],&m->entry[index+1],(m->count-index-1)*sizeof(*m->entry));m->count--;
 ui_index_free(&m->paths); /* Rebuild indices before the next addition. */
}
static unsigned ui_hash(const char *p){unsigned h=2166136261u;while(*p)h=(h^(unsigned char)*p++)*16777619u;return h;}
static void ui_hex(FILE *f,const char *p){while(*p)fprintf(f,"%02x",(unsigned char)*p++);}
static int ui_unhex(char *out,const char *p){unsigned n=0;while(p[0]&&p[0]!='\n'&&p[0]!='\r'){
 unsigned a,b;char c=p[0],d=p[1];if(!d||n>=UI_PATH-1)return 0;
 a=c>='0'&&c<='9'?c-'0':c>='a'&&c<='f'?c-'a'+10:99;
 b=d>='0'&&d<='9'?d-'0':d>='a'&&d<='f'?d-'a'+10:99;
 if(a>15||b>15||!(a*16+b))return 0;out[n++]=(char)(a*16+b);p+=2;
 }out[n]=0;return ui_path_ok(out);}
static int ui_write(FILE *f,const struct ui_model *m){unsigned i;
 fprintf(f,UI_CONFIG_PREFIX "3\n%u %u %u %u %u %u %u %u %u %u %u %u %u %u %u\n",m->clock,m->monitor,m->theme,m->mask,m->vmask,m->slot,m->mute,m->fire,m->last,m->filter,m->windowed,m->scanlines,m->fps,m->optimise,m->autoscan);
 for(i=0;i<m->count;i++){fprintf(f,"%u %u ",m->entry[i].favorite,m->entry[i].played);ui_hex(f,m->entry[i].path);fputc(' ',f);
  if(*m->entry[i].title)ui_hex(f,m->entry[i].title);else fputc('-',f);fputc(' ',f);
  if(*m->entry[i].art)ui_hex(f,m->entry[i].art);else fputc('-',f);fputc('\n',f);}return !ferror(f);}
static int ui_read(FILE *f,struct ui_model *m){char line[2400],path[UI_PATH],ph[1024],th[256],ah[1024],decoded[UI_PATH];unsigned fav,played;int pos,id,v2,v3;
 ui_defaults(m);if(!fgets(line,sizeof(line),f))return 0;
 v3=!strcmp(line,UI_CONFIG_PREFIX "3\n");v2=v3||!strcmp(line,UI_CONFIG_PREFIX "2\n");if(!v2&&strcmp(line,UI_CONFIG_PREFIX "1\n"))return 0;
 if(!fgets(line,sizeof(line),f)||sscanf(line,"%u %u %u %u %u %u %u %u %u %u %u %u %u %u %u",&m->clock,&m->monitor,&m->theme,&m->mask,&m->vmask,&m->slot,&m->mute,&m->fire,&m->last,&m->filter,&m->windowed,&m->scanlines,&m->fps,&m->optimise,&m->autoscan)!=(v3?15:v2?10:8)||m->filter>2||
 m->windowed>1||m->scanlines>3||m->fps>1||m->optimise>1||m->autoscan>1||m->monitor>1||m->theme>6||m->mask>2||m->vmask>2||m->slot>4||m->mute>1||m->fire>1)return 0;
 while(fgets(line,sizeof(line),f)){
  pos=0;
  if(v2){if(sscanf(line,"%u %u %1023s %255s %1023s %n",&fav,&played,ph,th,ah,&pos)!=5||!pos||line[pos]||!ui_unhex(path,ph))return 0;}
  else if(sscanf(line,"%u %u %n",&fav,&played,&pos)!=2||!pos||!ui_unhex(path,line+pos))return 0;
  if(fav>1||played>m->clock)return 0;
  id=ui_add(m,path);if(id<0)return 0;m->entry[id].favorite=fav;m->entry[id].played=played;
  if(v2){if(strcmp(th,"-")){if(!ui_unhex(decoded,th)||strlen(decoded)>=UI_TITLE)return 0;strcpy(m->entry[id].title,decoded);}
   if(strcmp(ah,"-")){if(!ui_unhex(m->entry[id].art,ah))return 0;}}
 }return !ferror(f)&&m->last<=m->count;
}
/* Display strings must not interpret MUI escapes/control bytes from filenames. */
static void ui_clean(char *out,unsigned cap,const char *in){unsigned n=0;while(*in&&n+1<cap){unsigned char c=*in++;out[n++]=(c<32||c==127)?' ':c;}out[n]=0;}
static void ui_title(const struct ui_entry *e,char *out){const char *p=e->path,*name=p;unsigned n;
 if(*e->title){ui_clean(out,UI_TITLE,e->title);return;}
 while(*p){if(*p==':'||*p=='/')name=p+1;p++;}ui_clean(out,UI_TITLE,name);n=strlen(out);
 if(n>4&&(!strcasecmp(out+n-4,".nes")||!strcasecmp(out+n-4,".sfc")||!strcasecmp(out+n-4,".smc")||!strcasecmp(out+n-4,".zip")))out[n-4]=0;
 for(n=0;out[n];n++)if(out[n]=='_')out[n]=' ';
}
static int ui_contains(const char *s,const char *q){if(!*q)return 1;for(;*s;s++){const char*a=s,*b=q;while(*a&&*b&&tolower((unsigned char)*a)==tolower((unsigned char)*b)){a++;b++;}if(!*b)return 1;}return 0;}
/* MUI runs these comparisons on its single application task. */
static const struct ui_model *ui_sort_model;
static unsigned ui_sort_filter;
static int ui_compare(const void*left,const void*right){
 unsigned x=*(const int*)left,y=*(const int*)right;char a[UI_TITLE],b[UI_TITLE];
 if(ui_sort_filter==1&&ui_sort_model->entry[x].played!=ui_sort_model->entry[y].played)
  return ui_sort_model->entry[x].played>ui_sort_model->entry[y].played?-1:1;
 ui_title(&ui_sort_model->entry[x],a);ui_title(&ui_sort_model->entry[y],b);
 int order=strcasecmp(a,b);return order?order:(x>y)-(x<y);
}
static unsigned ui_visible(const struct ui_model*m,unsigned filter,const char*q,int*out){unsigned i,n=0;char title[UI_TITLE];
 for(i=0;i<m->count;i++){ui_title(&m->entry[i],title);if((filter!=1||m->entry[i].played)&&(filter!=2||m->entry[i].favorite)&&(ui_contains(title,q)||ui_contains(m->entry[i].path,q)))out[n++]=i;}
 ui_sort_model=m;ui_sort_filter=filter;if(n>1)qsort(out,n,sizeof(*out),ui_compare);return n;
}
static void ui_played(struct ui_model*m,unsigned selected){unsigned i;if(m->clock==UINT_MAX){for(i=0;i<m->count;i++)m->entry[i].played=(m->entry[i].played+0u)/2;m->clock/=2;}m->entry[selected].played=++m->clock;}
/* AmigaDOS quotes use '*' as escape. Always quote even apparently safe paths. */
static int ui_quote(char *out,unsigned cap,const char *p){unsigned n=0;if(!ui_path_ok(p)||cap<3)return 0;out[n++]='"';
 while(*p){if(n+4>=cap)return 0;if(*p=='*'||*p=='"')out[n++]='*';out[n++]=*p++;}out[n++]='"';out[n]=0;return 1;}
#endif
