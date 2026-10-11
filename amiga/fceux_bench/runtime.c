/* GPL-3.0-or-later. Cortex-A9 soft integer runtime, no VFP/NEON state. */
#include <stdint.h>
#include <stddef.h>
void *memmove(void *d,const void *s,size_t n){unsigned char *a=d;const unsigned char *b=s;if((uintptr_t)a<(uintptr_t)b){while(n--)*a++=*b++;}else{a+=n;b+=n;while(n--)*--a=*--b;}return d;}
int memcmp(const void *a,const void *b,size_t n){const unsigned char *x=a,*y=b;while(n--){if(*x!=*y)return *x-*y;x++;y++;}return 0;}
int printf(const char *s,...){(void)s;return 0;}
static uint64_t divmod(unsigned n,unsigned d){unsigned q=0,r=0;if(!d)return 0;for(int i=31;i>=0;i--){unsigned c=r>>31;r=(r<<1)|((n>>i)&1u);if(c||r>=d){r-=d;q|=1u<<i;}}return ((uint64_t)r<<32)|q;}
unsigned __aeabi_uidiv(unsigned a,unsigned b){return (unsigned)divmod(a,b);}
uint64_t __aeabi_uidivmod(unsigned a,unsigned b){return divmod(a,b);}
int __aeabi_idiv(int a,int b){unsigned q=(unsigned)divmod(a<0?0u-(unsigned)a:(unsigned)a,b<0?0u-(unsigned)b:(unsigned)b);return (int)(((a<0)!=(b<0))?0u-q:q);}
uint64_t __aeabi_idivmod(int a,int b){uint64_t v=divmod(a<0?0u-(unsigned)a:(unsigned)a,b<0?0u-(unsigned)b:(unsigned)b);unsigned q=v,r=v>>32;if((a<0)!=(b<0))q=0u-q;if(a<0)r=0u-r;return ((uint64_t)r<<32)|q;}
