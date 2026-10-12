#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
static const char *rom_path;
static int player_muted;
#include "session.h"
static void fixture(const char*name,const char*tail){FILE*f=fopen(name,"w");assert(f);
 fputs("ZZSESSION1\n16 1 0 0\n",f);ui_hex(f,"SD032G:ROMs/Example.nes");fputc('\n',f);ui_hex(f,"PROGDIR:States/example.zzs");fputc('\n',f);if(tail)fputs(tail,f);assert(!fclose(f));}
int main(void){char name[]="/tmp/zz-session-XXXXXX";int fd=mkstemp(name);assert(fd>=0);close(fd);
 fixture(name,NULL);assert(fc_session(name));assert(!*fc_snapshot_path);assert(player_muted);assert(!strcmp(rom_path,"SD032G:ROMs/Example.nes"));
 fixture(name,"50524f474449523a536e617073686f74732f746573742e7a7a70\n");assert(fc_session(name));assert(!strcmp(fc_snapshot_path,"PROGDIR:Snapshots/test.zzp"));
 fixture(name,"invalid hex\n");assert(!fc_session(name));
 fixture(name,NULL);assert(fc_session(name));assert(!*fc_snapshot_path);
 unlink(name);puts("PASS session: old files, optional snapshot path, malformed extension, old session after new session");return 0;}
