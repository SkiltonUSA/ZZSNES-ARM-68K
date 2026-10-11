// Bounded freestanding C++ support for the diagnostic's known allocation set.
#pragma once
#include <stdint.h>
#include <stddef.h>
#include <limits.h>
extern "C" void *memcpy(void*,const void*,size_t);
extern "C" void *memset(void*,int,size_t);
extern "C" int memcmp(const void*,const void*,size_t);
extern "C" void ss_panic(void) __attribute__((noreturn));
#define assert(x) do { if(!(x))ss_panic(); } while(0)
void *operator new(size_t);
void *operator new[](size_t);
void operator delete(void*) noexcept;
void operator delete[](void*) noexcept;
inline void *operator new(size_t,void*p) noexcept {return p;}
namespace std {
template<class T> T min(T a,T b){return a<b?a:b;}
template<class T> T max(T a,T b){return a>b?a:b;}
inline int64_t abs(int64_t a){return a<0?-a:a;}
}
template<class T> class unique_ptr {
    T *p=nullptr;
public:
    unique_ptr()=default;
    explicit unique_ptr(T*q):p(q){}
    unique_ptr(unique_ptr &&q):p(q.release()){}
    template<class U> unique_ptr(unique_ptr<U> &&q):p(q.release()){}
    unique_ptr(const unique_ptr&)=delete;
    ~unique_ptr(){delete p;}
    unique_ptr &operator=(unique_ptr &&q){reset(q.release());return *this;}
    T* get()const{return p;} T* operator->()const{return p;}
    explicit operator bool()const{return p!=nullptr;}
    T* release(){T*r=p;p=nullptr;return r;}
    void reset(T*q=nullptr){if(p!=q){delete p;p=q;}}
};
// The fixture uses eight ROM pages and 32 WRAM pages; reserve 64 maximum.
template<class T> class vector {
    T items[64];unsigned n=0;
public:
    bool empty()const{return !n;} unsigned size()const{return n;}
    T &operator[](unsigned i){assert(i<n);return items[i];}
    void push_back(T&&v){assert(n<64);items[n++]=static_cast<T&&>(v);}
};
