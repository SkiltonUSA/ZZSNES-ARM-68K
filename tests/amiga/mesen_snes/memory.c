/* GPL-3.0-or-later. Exhaustive alignment/tail/canary test, including null n=0. */
#include <stdint.h>
#include <stddef.h>
#include <assert.h>
#include <stdio.h>
void *ss_memcpy(void*,const void*,size_t);
void *ss_memset(void*,int,size_t);
int main(void){
    _Alignas(16) unsigned char src[288],dst[288];
    assert(ss_memcpy(NULL,NULL,0)==NULL&&ss_memset(NULL,1,0)==NULL);
    for(unsigned i=0;i<sizeof(src);i++)src[i]=(unsigned char)(i*37+19);
    for(unsigned n=0;n<=257;n++)for(unsigned d=0;d<8;d++)for(unsigned s=0;s<8;s++){
        for(unsigned i=0;i<sizeof(dst);i++)dst[i]=0xac;
        assert(ss_memcpy(dst+8+d,src+8+s,n)==dst+8+d);
        for(unsigned i=0;i<sizeof(dst);i++)assert(dst[i]==(i>=8+d&&i<8+d+n?src[8+s+i-8-d]:0xac));
        for(int v=-1;v<=256;v++){
            if(s)break; // fills have no source alignment
            for(unsigned i=0;i<sizeof(dst);i++)dst[i]=0xac;
            assert(ss_memset(dst+8+d,v,n)==dst+8+d);
            for(unsigned i=0;i<sizeof(dst);i++)assert(dst[i]==(i>=8+d&&i<8+d+n?(unsigned char)v:0xac));
        }
    }
    for(unsigned i=0;i<sizeof(src);i++)assert(src[i]==(unsigned char)(i*37+19));
    puts("PASS memory: 0..257 bytes, 8x8 alignments, all byte values, canaries, null zero length");
}
