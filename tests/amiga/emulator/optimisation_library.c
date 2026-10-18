#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
static long fail_after=-1;
static int fail(void){if(fail_after<0)return 0;if(!fail_after)return 1;fail_after--;return 0;}
static void*test_malloc(size_t n){return fail()?NULL:malloc(n);}
static void*test_calloc(size_t n,size_t s){return fail()?NULL:calloc(n,s);}
static void*test_realloc(void*p,size_t n){return fail()?NULL:realloc(p,n);}
#define malloc test_malloc
#define calloc test_calloc
#define realloc test_realloc
#include "ui_model.h"
#undef malloc
#undef calloc
#undef realloc
static struct ui_model a,b;
int main(void){char path[UI_PATH];const unsigned total=20000;clock_t begin=clock();
 int*visible=malloc(total*sizeof(*visible));assert(visible);ui_defaults(&a);
 assert(a.windowed==0&&a.scanlines==2);
 /* Explicit saved choices still override fresh-install defaults. */
 a.windowed=1;a.scanlines=0;a.smooth=1;
 for(unsigned i=0;i<total;i++){snprintf(path,sizeof(path),"USB:ROMs/Game-%05u.nes",total-i);assert(ui_add(&a,path)==(int)i);a.entry[i].favorite=i%7==0;ui_played(&a,i);}
 assert(a.count==total);for(unsigned i=0;i<total;i++){snprintf(path,sizeof(path),"usb:roms/game-%05u.NES",total-i);assert(ui_add(&a,path)==(int)i);}assert(a.count==total);
 a.last=total;a.filter=2;strcpy(a.entry[total-1].title,"Renamed last game");
 assert(ui_visible(&a,0,"",visible)==total);fail_after=0;assert(ui_visible(&a,0,"",visible)==total);fail_after=-1;assert(visible[0]==(int)total-2);
 assert(ui_visible(&a,1,"",visible)==total&&visible[0]==(int)total-1);
 assert(ui_visible(&a,2,"",visible)==(total+6)/7);
 assert(ui_visible(&a,0,"rEnAmEd",visible)==1&&visible[0]==(int)total-1);
 FILE*f=tmpfile();assert(f&&ui_write(f,&a));rewind(f);assert(ui_read(f,&b));fclose(f);
 assert(b.windowed==1&&b.scanlines==0&&b.smooth==1);assert(b.count==total&&b.last==total&&b.filter==2);assert(!memcmp(a.entry,b.entry,total*sizeof(*a.entry)));
 ui_remove(&b,1000);assert(b.count==total-1);assert(ui_add(&b,"USB:ROMs/Game-19000.nes")==19999);assert(ui_add(&b,"USB:ROMs/Game-18999.nes")==1000);
 /* An allocation failure leaves all accepted entries intact and permits retry. */
 for(long point=0;point<5;point++){struct ui_model m={0};assert(ui_add(&m,"Keep.nes")==0);ui_remove(&m,0);
  fail_after=point;int n=ui_add(&m,"New.nes");fail_after=-1;
  if(n<0){assert(!m.count);assert(ui_add(&m,"New.nes")==0);}assert(m.count==1);ui_dispose(&m);}
 for(long point=0;point<3;point++){struct ui_model m={0};
  for(unsigned i=0;i<128;i++){snprintf(path,sizeof(path),"Keep-%u.nes",i);assert(ui_add(&m,path)==(int)i);}
  fail_after=point;assert(ui_add(&m,"Growth.nes")<0);fail_after=-1;
  assert(m.count==128&&!strcmp(m.entry[127].path,"Keep-127.nes"));assert(ui_add(&m,"Growth.nes")==128);ui_dispose(&m);}
 fail_after=0;assert(ui_add(&a,"USB:extra.nes")<0);fail_after=-1;assert(a.count==total);assert(ui_add(&a,"USB:extra.nes")==total);
 printf("PASS %u ROMs: growth, duplicate index, sort/search/filter, rename, persistence, deletion and allocation-failure recovery (%.3fs)\n",total,(double)(clock()-begin)/CLOCKS_PER_SEC);
 /* v3 remains readable and never inherits the new interpolation setting. */
 {FILE*f=tmpfile();struct ui_model old={0};assert(f);fputs(UI_CONFIG_PREFIX "3\n0 0 2 0 0 0 0 0 0 0 0 4 0 1 0\n",f);rewind(f);
  assert(ui_read(f,&old)&&old.smooth==0&&old.scanlines==4);fclose(f);ui_dispose(&old);}
 /* Token parser rejects overflow, malformed hex and extra fields. */
 const char *bad_rows[]={"4294967296 0 41 - -\n","0 4294967296 41 - -\n","-1 0 41 - -\n","0 0 4100 - -\n","0 0 4g - -\n","0 0 41 - - extra\n","0 0 41 -\n"};
 for(unsigned i=0;i<sizeof(bad_rows)/sizeof(bad_rows[0]);i++){struct ui_model empty={0},read={0};ui_defaults(&empty);FILE*f=tmpfile();assert(f&&ui_write(f,&empty));fputs(bad_rows[i],f);rewind(f);assert(!ui_read(f,&read));fclose(f);ui_dispose(&read);ui_dispose(&empty);}
 {struct ui_model empty={0},read={0};ui_defaults(&empty);FILE*f=tmpfile();assert(f&&ui_write(f,&empty));fputs(" \t0\t0\t4142\t-\t-\t\r\n",f);rewind(f);assert(ui_read(f,&read)&&read.count==1&&!strcmp(read.entry[0].path,"AB"));fclose(f);ui_dispose(&read);ui_dispose(&empty);}
 puts("PASS strict numeric/hex fields, missing/extra tokens, whitespace and allocation fallback");
 free(visible);ui_dispose(&a);ui_dispose(&b);return 0;
}
