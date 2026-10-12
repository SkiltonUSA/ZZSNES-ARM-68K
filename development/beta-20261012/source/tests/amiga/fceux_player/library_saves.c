#include "library_saves.h"
#include <assert.h>
#include <stdio.h>
static uint8_t data[LIB_STATE_CAP+1];
#ifndef LIBRARY_SNES
static void put(unsigned at,uint32_t x){for(unsigned i=0;i<4;i++)data[at+i]=x>>(8*i);}
#endif
int main(int argc,char**argv){unsigned n;
 if(argc==2){FILE*f=fopen(argv[1],"rb");assert(f);n=fread(data,1,sizeof(data),f);assert(!ferror(f));fclose(f);
#ifdef LIBRARY_SNES
  assert(library_state_valid(data,n,0xae8466a1u,0x8aa41dc9u,0x80000));
#else
  assert(library_state_valid(data,n,0x3e0961dbu,0,0x6010));
#endif
  puts("PASS existing local save fixture (read only; not a player resume test)");return 0;}
#ifdef LIBRARY_SNES
 n=PS_PAYLOAD_OFFSET+32;memcpy(data,"ZZSNES2",8);ps_put(data,8,2);ps_put(data,12,n);
 ps_put(data,16,123);ps_put(data,20,456);ps_put(data,24,32768);ps_put(data,32,128);ps_put(data,36,112);ps_put(data,56,4);ps_put(data,28,ps_checksum(data,n));
#else
 n=64;memcpy(data,"ZZS1",4);put(4,1);put(8,123);put(12,32768);put(16,n-32);put(20,bytes_hash(data+32,n-32));put(24,0x20261011u);
#endif
 assert(library_state_valid(data,n,123,456,32768));
 assert(!library_state_valid(data,n,999,456,32768));assert(!library_state_valid(data,n,123,456,12345));
 for(unsigned i=0;i<n;i++)assert(!library_state_valid(data,i,123,456,32768));
 assert(!library_state_valid(data,LIB_STATE_CAP+1,123,456,32768));
 for(unsigned i=0;i<n;i++){data[i]^=1;assert(!library_state_valid(data,n,123,456,32768));data[i]^=1;}
#ifdef LIBRARY_SNES
 assert(!library_state_valid(data,n,123,999,32768));
 ps_put(data,56,8);ps_put(data,28,ps_checksum(data,n));assert(!library_state_valid(data,n,123,456,32768));
#endif
 {int32_t a[3]={20000,1,1},b[3]={19999,1439,2999};assert(library_newer(a,b));assert(!library_newer(b,a));assert(!library_newer(a,a));
  b[0]=20000;b[1]=1;b[2]=2;assert(library_newer(b,a));b[1]=2;b[2]=0;assert(library_newer(b,a));}
 puts("PASS save envelope: ROM identity, size, ABI, truncation, corruption; newest-file ordering");
}
