#include "library_saves.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
static const char*blocked;
static int remove_one(const char*p){struct stat st;
 if(blocked&&!strcmp(p,blocked))return -1;
 if(lstat(p,&st))return errno==ENOENT?0:-1;
 if(!S_ISREG(st.st_mode))return -1;
 return unlink(p)?-1:1;
}
static void create(const char*p){FILE*f=fopen(p,"wb");assert(f);fputs("disposable save fixture",f);assert(!fclose(f));}
static int exists(const char*p){struct stat st;return !lstat(p,&st);}
int main(void){char tmp[]="/tmp/zz-delete-XXXXXX",main[256],bak[256],other[256],art[256];
 assert(mkdtemp(tmp));snprintf(main,sizeof(main),"%s/slot.zzs",tmp);snprintf(bak,sizeof(bak),"%s/slot.zzs.bak",tmp);
 snprintf(other,sizeof(other),"%s/other-slot.zzs",tmp);snprintf(art,sizeof(art),"%s/title.zzp",tmp);
 create(main);create(bak);create(other);create(art);
 blocked=main;assert(library_delete_pair(main,bak,remove_one)==-1);assert(exists(main)&&exists(bak));
 blocked=bak;assert(library_delete_pair(main,bak,remove_one)==-2);assert(!exists(main)&&exists(bak));
 blocked=NULL;assert(library_delete_pair(main,bak,remove_one)==1);assert(!exists(bak));
 assert(library_delete_pair(main,bak,remove_one)==1); /* already empty */
 create(main);create(bak);assert(library_delete_pair(main,bak,remove_one)==1);assert(!exists(main)&&!exists(bak));
 assert(exists(other)&&exists(art)); /* other saves and library art survive */
 assert(!mkdir(main,0700));create(bak);assert(library_delete_pair(main,bak,remove_one)==-1);assert(exists(bak));
 assert(!rmdir(main));assert(remove_one(bak)==1);assert(remove_one(other)==1);assert(remove_one(art)==1);assert(!rmdir(tmp));
 puts("PASS delete: save+backup, backup-only, absent files, permission failures, directory refusal, unrelated save/artwork preservation");
}
