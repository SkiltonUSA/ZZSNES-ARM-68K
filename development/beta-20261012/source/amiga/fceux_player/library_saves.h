/* GPL-3.0-or-later. Browser-level save envelope checks, not core deserialization. */
#ifndef ZZ_LIBRARY_SAVES_H
#define ZZ_LIBRARY_SAVES_H
#include <stdint.h>
#include <string.h>
#ifdef LIBRARY_SNES
#include "state_format.h"
#define LIB_STATE_CAP PS_STATE_MAX
#else
#define LIB_STATE_CAP 32768u
#endif
static uint32_t bytes_hash(const uint8_t*p,unsigned n){uint32_t h=2166136261u;while(n--)h=(h^*p++)*16777619u;return h;}
static uint32_t le32(const uint8_t*p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static int library_state_valid(const uint8_t*p,unsigned n,uint32_t rom,uint32_t rom2,unsigned bytes){
#ifdef LIBRARY_SNES
 return ps_valid(p,n)&&ps_get(p,16)==rom&&ps_get(p,20)==rom2&&ps_get(p,24)==bytes&&ps_get(p,56)==4;
#else
 (void)rom2;return n>=32&&n<=LIB_STATE_CAP&&!memcmp(p,"ZZS1",4)&&le32(p+4)==1&&le32(p+8)==rom&&le32(p+12)==bytes&&le32(p+16)==n-32&&le32(p+20)==bytes_hash(p+32,n-32)&&le32(p+24)==0x20261011u&&!le32(p+28);
#endif
}
static int library_newer(const int32_t*a,const int32_t*b){for(unsigned i=0;i<3;i++)if(a[i]!=b[i])return a[i]>b[i];return 0;}
/* remove_one: 1=removed, 0=absent, -1=error/non-file. If removing the
 * current save fails, retain the recovery copy. Never recurse into a directory.
 * Caller supplies only its selected slot path and that exact path + ".bak". */
static int library_delete_pair(const char*path,const char*backup,int(*remove_one)(const char*)){
 if(remove_one(path)<0)return -1;
 if(remove_one(backup)<0)return -2;
 return 1;
}
#endif
