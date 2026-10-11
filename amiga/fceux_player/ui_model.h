/* GPL-3.0-or-later. Bounded, host-testable library/preferences model.
 * Hex-encoded paths permit spaces, quotes and tabs without shell ambiguity. */
#ifndef FC_UI_MODEL_H
#define FC_UI_MODEL_H
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#define UI_MAX 128
#define UI_PATH 512
struct ui_entry {char path[UI_PATH];unsigned favorite,played;};
struct ui_model {unsigned count,clock,monitor,theme,mask,vmask,slot,mute,fire;struct ui_entry entry[UI_MAX];};
static void ui_defaults(struct ui_model *m){memset(m,0,sizeof(*m));m->mask=1;}
static int ui_path_ok(const char *p){unsigned n=strlen(p);return n>0&&n<UI_PATH&&!strchr(p,'\n')&&!strchr(p,'\r');}
static int ui_add(struct ui_model *m,const char *p){unsigned i;if(!ui_path_ok(p))return -1;
 for(i=0;i<m->count;i++)if(!strcmp(m->entry[i].path,p))return (int)i;
 if(m->count==UI_MAX)return -1;strcpy(m->entry[i].path,p);m->count++;return (int)i;}
static unsigned ui_hash(const char *p){unsigned h=2166136261u;while(*p)h=(h^(unsigned char)*p++)*16777619u;return h;}
static void ui_hex(FILE *f,const char *p){while(*p)fprintf(f,"%02x",(unsigned char)*p++);}
static int ui_unhex(char *out,const char *p){unsigned n=0;while(p[0]&&p[0]!='\n'&&p[0]!='\r'){
 unsigned a,b;char c=p[0],d=p[1];if(!d||n>=UI_PATH-1)return 0;
 a=c>='0'&&c<='9'?c-'0':c>='a'&&c<='f'?c-'a'+10:99;
 b=d>='0'&&d<='9'?d-'0':d>='a'&&d<='f'?d-'a'+10:99;
 if(a>15||b>15||!(a*16+b))return 0;out[n++]=(char)(a*16+b);p+=2;
 }out[n]=0;return ui_path_ok(out);}
static int ui_write(FILE *f,const struct ui_model *m){unsigned i;
 fprintf(f,"ZZNESUI1\n%u %u %u %u %u %u %u %u\n",m->clock,m->monitor,m->theme,m->mask,m->vmask,m->slot,m->mute,m->fire);
 for(i=0;i<m->count;i++){fprintf(f,"%u %u ",m->entry[i].favorite,m->entry[i].played);ui_hex(f,m->entry[i].path);fputc('\n',f);}return !ferror(f);}
static int ui_read(FILE *f,struct ui_model *m){char line[1100],path[UI_PATH];unsigned fav,played;int pos,id;
 ui_defaults(m);if(!fgets(line,sizeof(line),f)||strcmp(line,"ZZNESUI1\n"))return 0;
 if(!fgets(line,sizeof(line),f)||sscanf(line,"%u %u %u %u %u %u %u %u",&m->clock,&m->monitor,&m->theme,&m->mask,&m->vmask,&m->slot,&m->mute,&m->fire)!=8||
 m->monitor>1||m->theme>4||m->mask>2||m->vmask>2||m->slot>4||m->mute>1||m->fire>1)return 0;
 while(fgets(line,sizeof(line),f)){
  pos=0;if(sscanf(line,"%u %u %n",&fav,&played,&pos)!=2||!pos||fav>1||played>m->clock||!ui_unhex(path,line+pos))return 0;
  id=ui_add(m,path);if(id<0)return 0;m->entry[id].favorite=fav;m->entry[id].played=played;
 }return !ferror(f);
}
/* AmigaDOS quotes use '*' as escape. Always quote even apparently safe paths. */
static int ui_quote(char *out,unsigned cap,const char *p){unsigned n=0;if(!ui_path_ok(p)||cap<3)return 0;out[n++]='"';
 while(*p){if(n+4>=cap)return 0;if(*p=='*'||*p=='"')out[n++]='*';out[n++]=*p++;}out[n++]='"';out[n]=0;return 1;}
#endif
