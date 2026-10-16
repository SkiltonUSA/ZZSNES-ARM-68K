#include "tiny.h"
alignas(16) static uint8_t heap[0x160000];
static size_t used;
extern "C" void ss_heap_reset(){used=0;}
extern "C" uint32_t ss_heap_used(){return used;}
void *operator new(size_t n){
    n=(n+15)&~(size_t)15;
    if(n>sizeof(heap)-used)ss_panic();
    void *p=heap+used;used+=n;memset(p,0,n);return p;
}
void *operator new[](size_t n){return ::operator new(n);}
void operator delete(void*) noexcept {}
void operator delete[](void*) noexcept {}
void operator delete(void*,size_t) noexcept {}
void operator delete[](void*,size_t) noexcept {}
extern "C" void __cxa_pure_virtual(){ss_panic();}
