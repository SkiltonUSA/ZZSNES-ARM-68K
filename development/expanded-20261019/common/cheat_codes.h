#pragma once
#include <string.h>
#define PS_CHEAT_MAX 32
#define PS_CHEAT_BYTES 321
/* Semicolon-separated 8 hex PAR / XXXX-XXXX Game Genie codes. */
static int ps_cheat_next(const char **cursor,char code[10]){
 const char*p=*cursor;unsigned n=0;while(*p==' '||*p==';')p++;
 if(!*p){*cursor=p;return 0;}
 while(*p&&*p!=';'&&*p!=' '){if(n==9)return -1;code[n++]=*p++;}
 code[n]=0;*cursor=p;return 1;
}
static int ps_cheats_valid(const char*s){
 char code[10];unsigned count=0;int r;if(strlen(s)>=PS_CHEAT_BYTES)return 0;
 while((r=ps_cheat_next(&s,code))>0){unsigned n=strlen(code);if(++count>PS_CHEAT_MAX||(n!=8&&n!=9)||(n==9&&code[4]!='-'))return 0;
 for(unsigned i=0;i<n;i++){char c=code[i];if(n==9&&i==4)continue;if(!((c>='0'&&c<='9')||(c>='a'&&c<='f')||(c>='A'&&c<='F')))return 0;}}
 return r==0;
}
