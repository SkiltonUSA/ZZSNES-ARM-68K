#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "rom_archive.h"
int main(int argc,char**argv){
 assert(argc==5);unsigned cap=strtoul(argv[2],0,0),used=99;int snes=atoi(argv[3]);char name[512];
 unsigned char*out=malloc(cap+16);assert(out);memset(out+cap,0xa5,16);
 int e=em_rom_load(argv[1],out,cap,&used,snes,name,sizeof(name));
 for(unsigned i=0;i<16;i++)assert(out[cap+i]==0xa5);
 if(!e){FILE*f=fopen(argv[4],"rb");assert(f);unsigned i=0;int c;while((c=fgetc(f))!=EOF){assert(i<used&&out[i]==c);i++;}assert(i==used);fclose(f);}
 else assert(used==0);
 printf("%d %u %s\n",e,used,name);free(out);return 0;
}
