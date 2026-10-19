#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* Index zero means unassigned; reserved emulator hotkeys are not assignable. */
static const unsigned ps_keys[]={128,0x31,0x32,0x60,0x44,0x20,0x21,0x33,0x34,0x4c,0x4d,0x4f,0x4e,0x30,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x36,0x15,0x16,0x17,0x18,0x11,0x12,0x13,0x14,0x40,0x61,0x2d,0x1d,0x2f,0x3d,0x3e,0x3f};
struct ps_controls {unsigned second,keys[2][12],joy[2][7];};
static void ps_controls_default(struct ps_controls*c){
 static const unsigned keys[]={1,2,3,4,9,10,11,12,5,6,7,8};
 static const unsigned joy[]={0,1,2,8,10,11,3};
 memset(c,0,sizeof(*c));memcpy(c->keys[0],keys,sizeof(keys));
 for(unsigned p=0;p<2;p++)memcpy(c->joy[p],joy,sizeof(joy));
}
static int ps_controls_valid(const struct ps_controls*c){
 if(c->second>2)return 0;
 for(unsigned p=0;p<2;p++){for(unsigned i=0;i<12;i++)if(c->keys[p][i]>=sizeof(ps_keys)/sizeof(ps_keys[0]))return 0;
 for(unsigned i=0;i<7;i++)if(c->joy[p][i]>12)return 0;}return 1;
}
static unsigned ps_key_mask(const struct ps_controls*c,unsigned p,unsigned raw){unsigned bits=0;
 for(unsigned i=0;i<12;i++)if(c->keys[p][i]&&ps_keys[c->keys[p][i]]==raw)bits|=1u<<i;return bits;}
static int ps_controls_write(FILE*f,const struct ps_controls*c){fprintf(f,"INPUT %u",c->second);
 for(unsigned p=0;p<2;p++){for(unsigned i=0;i<12;i++)fprintf(f," %u",c->keys[p][i]);for(unsigned i=0;i<7;i++)fprintf(f," %u",c->joy[p][i]);}fputc('\n',f);return !ferror(f);}
static int ps_controls_read(FILE*f,struct ps_controls*c){
 char line[512],*p,*end;unsigned *fields[39],n=0;struct ps_controls t;
 if(!fgets(line,sizeof(line),f)||strncmp(line,"INPUT ",6))return 0;p=line+6;fields[n++]=&t.second;
 for(unsigned j=0;j<2;j++){for(unsigned i=0;i<12;i++)fields[n++]=&t.keys[j][i];for(unsigned i=0;i<7;i++)fields[n++]=&t.joy[j][i];}
 for(unsigned i=0;i<n;i++){while(*p==' ')p++;if(*p<'0'||*p>'9')return 0;unsigned long v=strtoul(p,&end,10);if(v>128)return 0;*fields[i]=v;p=end;}
 while(*p==' '||*p=='\r'||*p=='\n')p++;if(*p||!ps_controls_valid(&t))return 0;*c=t;return 1;
}
