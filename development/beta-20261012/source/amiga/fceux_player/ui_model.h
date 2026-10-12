/* GPL-3.0-or-later. Bounded, host-testable library/preferences model.
 * Hex-encoded paths permit spaces, quotes and tabs without shell ambiguity. */
#ifndef FC_UI_MODEL_H
#define FC_UI_MODEL_H
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <limits.h>
#define UI_MAX 128
#define UI_PATH 512
#define UI_TITLE 128
#ifndef UI_CONFIG_PREFIX
#define UI_CONFIG_PREFIX "ZZNESUI"
#endif
struct ui_entry {char path[UI_PATH],title[UI_TITLE],art[UI_PATH];unsigned favorite,played;};
struct ui_model {unsigned count,clock,monitor,theme,mask,vmask,slot,mute,fire,last,filter;struct ui_entry entry[UI_MAX];};
static void ui_defaults(struct ui_model *m){memset(m,0,sizeof(*m));
#ifdef LIBRARY_SNES
 m->theme=2;
#else
 m->mask=1;
#endif
}
static int ui_path_ok(const char *p){unsigned n=strlen(p);return n>0&&n<UI_PATH&&!strchr(p,'\n')&&!strchr(p,'\r');}
static int ui_add(struct ui_model *m,const char *p){unsigned i;if(!ui_path_ok(p))return -1;
 for(i=0;i<m->count;i++)if(!strcmp(m->entry[i].path,p))return (int)i;
 if(m->count==UI_MAX)return -1;memset(&m->entry[i],0,sizeof(m->entry[i]));strcpy(m->entry[i].path,p);m->count++;return (int)i;}
static unsigned ui_hash(const char *p){unsigned h=2166136261u;while(*p)h=(h^(unsigned char)*p++)*16777619u;return h;}
static void ui_hex(FILE *f,const char *p){while(*p)fprintf(f,"%02x",(unsigned char)*p++);}
static int ui_unhex(char *out,const char *p){unsigned n=0;while(p[0]&&p[0]!='\n'&&p[0]!='\r'){
 unsigned a,b;char c=p[0],d=p[1];if(!d||n>=UI_PATH-1)return 0;
 a=c>='0'&&c<='9'?c-'0':c>='a'&&c<='f'?c-'a'+10:99;
 b=d>='0'&&d<='9'?d-'0':d>='a'&&d<='f'?d-'a'+10:99;
 if(a>15||b>15||!(a*16+b))return 0;out[n++]=(char)(a*16+b);p+=2;
 }out[n]=0;return ui_path_ok(out);}
static int ui_write(FILE *f,const struct ui_model *m){unsigned i;
 fprintf(f,UI_CONFIG_PREFIX "2\n%u %u %u %u %u %u %u %u %u %u\n",m->clock,m->monitor,m->theme,m->mask,m->vmask,m->slot,m->mute,m->fire,m->last,m->filter);
 for(i=0;i<m->count;i++){fprintf(f,"%u %u ",m->entry[i].favorite,m->entry[i].played);ui_hex(f,m->entry[i].path);fputc(' ',f);
  if(*m->entry[i].title)ui_hex(f,m->entry[i].title);else fputc('-',f);fputc(' ',f);
  if(*m->entry[i].art)ui_hex(f,m->entry[i].art);else fputc('-',f);fputc('\n',f);}return !ferror(f);}
static int ui_read(FILE *f,struct ui_model *m){char line[2400],path[UI_PATH],ph[1024],th[256],ah[1024],decoded[UI_PATH];unsigned fav,played;int pos,id,v2;
 ui_defaults(m);if(!fgets(line,sizeof(line),f))return 0;
 v2=!strcmp(line,UI_CONFIG_PREFIX "2\n");if(!v2&&strcmp(line,UI_CONFIG_PREFIX "1\n"))return 0;
 if(!fgets(line,sizeof(line),f)||sscanf(line,"%u %u %u %u %u %u %u %u %u %u",&m->clock,&m->monitor,&m->theme,&m->mask,&m->vmask,&m->slot,&m->mute,&m->fire,&m->last,&m->filter)!=(v2?10:8)||m->last>UI_MAX||m->filter>2||
 m->monitor>1||m->theme>6||m->mask>2||m->vmask>2||m->slot>4||m->mute>1||m->fire>1)return 0;
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
 if(n>4&&(!strcasecmp(out+n-4,".nes")||!strcasecmp(out+n-4,".sfc")||!strcasecmp(out+n-4,".smc")))out[n-4]=0;
 for(n=0;out[n];n++)if(out[n]=='_')out[n]=' ';
}
static int ui_contains(const char *s,const char *q){if(!*q)return 1;for(;*s;s++){const char*a=s,*b=q;while(*a&&*b&&tolower((unsigned char)*a)==tolower((unsigned char)*b)){a++;b++;}if(!*b)return 1;}return 0;}
static unsigned ui_visible(const struct ui_model*m,unsigned filter,const char*q,int*out){unsigned i,j,n=0;char a[UI_TITLE],b[UI_TITLE];
 for(i=0;i<m->count;i++){ui_title(&m->entry[i],a);if((filter!=1||m->entry[i].played)&&(filter!=2||m->entry[i].favorite)&&(ui_contains(a,q)||ui_contains(m->entry[i].path,q)))out[n++]=i;}
 for(i=1;i<n;i++){int x=out[i];ui_title(&m->entry[x],a);j=i;while(j){int y=out[j-1],before;ui_title(&m->entry[y],b);
   before=filter==1&&m->entry[x].played!=m->entry[y].played?m->entry[x].played>m->entry[y].played:strcasecmp(a,b)<0;
   if(!before)break;out[j]=out[j-1];j--;}out[j]=x;}return n;
}
static void ui_played(struct ui_model*m,unsigned selected){unsigned i;if(m->clock==UINT_MAX){for(i=0;i<m->count;i++)m->entry[i].played=(m->entry[i].played+0u)/2;m->clock/=2;}m->entry[selected].played=++m->clock;}
/* AmigaDOS quotes use '*' as escape. Always quote even apparently safe paths. */
static int ui_quote(char *out,unsigned cap,const char *p){unsigned n=0;if(!ui_path_ok(p)||cap<3)return 0;out[n++]='"';
 while(*p){if(n+4>=cap)return 0;if(*p=='*'||*p=='"')out[n++]='*';out[n++]=*p++;}out[n++]='"';out[n]=0;return 1;}
#endif
