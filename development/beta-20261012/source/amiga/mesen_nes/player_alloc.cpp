#include "player_alloc.h"
alignas(16) static uint8_t arena[65536];
static size_t used;
void player_reset_alloc(){used=0;memset(arena,0,sizeof(arena));}
void *player_alloc(size_t n){n=(n+15)&~size_t(15);if(n>sizeof(arena)-used)__builtin_trap();void *p=arena+used;used+=n;return p;}
void *player_memmove(void *d,const void *s,size_t n){uint8_t *a=(uint8_t *)d;const uint8_t *b=(const uint8_t *)s;if(a<b){for(size_t i=0;i<n;i++)a[i]=b[i];}else{while(n){--n;a[n]=b[n];}}return d;}
extern "C" void __cxa_pure_virtual(){__builtin_trap();}
