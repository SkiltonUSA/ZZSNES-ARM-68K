#!/usr/bin/env python3
"""Exercise the actual host battery transaction against a temporary directory."""
import subprocess,tempfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
s=(ROOT/'development/expanded-20261019/host/player_frontend.h').read_text()
s=s[s.index('static int player_load_save'):]
prefix=r'''
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <unistd.h>
typedef uint32_t ULONG;typedef unsigned char UBYTE;typedef void*APTR;typedef int BPTR;
#define MEMF_PUBLIC 0
#define MEMF_CLEAR 0
#define ACCESS_READ 0
#define NES_SAVE_LIMIT 131072
#define MB_REPLY 0
#define NES_SAVE 256
#define NES_SAVE_LOAD 8
#define NES_SAVE_STORE 9
static UBYTE memory[NES_SAVE_LIMIT+256],core[32],*shared=memory,*save_data;
static ULONG save_size,save_hash;static char save_path[1040];static const char*rom_path="Test.sfc";
static int fail_rename;
static void*AllocVec(unsigned n,int flags){(void)flags;return calloc(1,n);}
static void CopyMem(void*a,void*b,unsigned n){memcpy(b,a,n);}
static int Lock(const char*p,int mode){(void)mode;return !access(p,F_OK);}
static void UnLock(int p){(void)p;}
static int DeleteFile(const char*p){return !remove(p);}
static int Rename(const char*a,const char*b){if(fail_rename&&!strcmp(a,"Test.sfc.sav.tmp")){errno=EIO;return 0;}return !rename(a,b);}
static unsigned ad_get(void*p,unsigned at){(void)p;return at==44?32:0;}
static void sync_noop(void*p,unsigned n,void*x){(void)p;(void)n;(void)x;}
static struct {void(*push)(void*,unsigned,void*);void(*pull)(void*,unsigned,void*);} io={sync_noop,sync_noop},*syncio=&io;
static int request(unsigned op,unsigned n){assert(n==32);if(op==NES_SAVE_LOAD)memcpy(core,memory+NES_SAVE,n);else {assert(op==NES_SAVE_STORE);memcpy(memory+NES_SAVE,core,n);}return 1;}
static ULONG bytes_hash(const UBYTE*p,ULONG n){uint32_t h=2166136261u;while(n--)h=(h^*p++)*16777619u;return h;}
'''
main=r'''
static void reload(void){free(save_data);save_data=NULL;memset(core,0xcc,32);assert(player_load_save());}
static void check_file(const char*p,unsigned value){FILE*f=fopen(p,"rb");assert(f);for(int i=0;i<32;i++)assert(fgetc(f)==(int)value);assert(fgetc(f)==EOF);fclose(f);}
int main(void){memset(core,0x60,32);assert(player_load_save());assert(core[0]==0x60);assert(!Lock(save_path,0));
 memset(core,0x12,32);assert(player_store_save());check_file(save_path,0x12);reload();assert(core[31]==0x12);
 memset(core,0x34,32);assert(player_store_save());check_file(save_path,0x34);check_file("Test.sfc.sav.bak",0x12);
 memset(core,0x56,32);fail_rename=1;assert(!player_store_save());check_file(save_path,0x34);fail_rename=0;reload();assert(core[0]==0x34);
 FILE*f=fopen(save_path,"wb");assert(f);fputs("bad",f);fclose(f);free(save_data);save_data=NULL;assert(!player_load_save());
 free(save_data);puts("PASS battery startup defaults, persist/reload, backup rotation, rename rollback and malformed-file refusal");return 0;}
'''
with tempfile.TemporaryDirectory(prefix='zzsnes-battery-')as t:
 d=Path(t);(d/'test.c').write_text(prefix+s+main)
 subprocess.run(['cc','-std=c99','-g','-fsanitize=address,undefined',str(d/'test.c'),'-o',str(d/'test')],check=True)
 subprocess.run([str(d/'test')],cwd=d,check=True)
