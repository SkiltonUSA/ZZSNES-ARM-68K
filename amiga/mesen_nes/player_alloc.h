// GPL-3.0-or-later. Bounded per-cartridge storage; no system allocator on ARM.
#pragma once
#include <stddef.h>
#include <stdint.h>
extern "C" void *memset(void *,int,size_t);
extern "C" void *memcpy(void *,const void *,size_t);
void *player_alloc(size_t);
void player_reset_alloc();
void *player_memmove(void *,const void *,size_t);
inline void *operator new(size_t,void *p) noexcept { return p; }
template<class T> class PlayerPtr { T *p=nullptr; public: void reset(T *v){p=v;} T *get(){return p;} T *operator->(){return p;} };
inline int player_abs(int n){return n<0?-n:n;}
