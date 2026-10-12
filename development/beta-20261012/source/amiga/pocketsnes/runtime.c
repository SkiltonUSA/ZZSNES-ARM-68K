#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>
#include <stdio.h>
#include <time.h>
/* Single-worker bounded arena; retained across benchmark requests so upstream
 * persistent caches remain valid. Every launch starts with zeroed owned BSS. */
static unsigned char heap[0x1200000] __attribute__((aligned(64)));
extern void fb_heap_init(void*,size_t);
void pocket_heap_init(void){static int done;if(!done){fb_heap_init(heap,sizeof(heap));done=1;}}
int printf(const char *fmt,...){return 0;}
int fprintf(FILE *stream,const char*fmt,...){return 0;}
int puts(const char*s){return 0;}
int fflush(FILE*s){return 0;}
void exit(int code){for(;;)__asm__ volatile("wfe");}
void abort(void){exit(99);}
time_t time(time_t*t){if(t)*t=0;return 0;}

int errno;
