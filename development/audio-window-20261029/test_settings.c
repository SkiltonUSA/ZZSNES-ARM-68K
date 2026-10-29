#include <assert.h>
#include <stdio.h>
#include "ui_model.h"
#ifdef TEST_SNES
#include "preferences.h"
#endif
int main(void){
 struct ui_model a={0},b={0};ui_defaults(&a);ui_add(&a,"ROMs/Test.zip");
 for(unsigned size=0;size<4;size++){
  a.windowed=size;FILE*f=tmpfile();assert(f&&ui_write(f,&a));rewind(f);assert(ui_read(f,&b));fclose(f);assert(b.windowed==size&&b.count==1);
#ifdef TEST_SNES
  struct ps_preferences p={0},q={0};ps_controls_default(&p.controls);p.windowed=size;assert(ps_preferences_valid(&p));
  f=tmpfile();assert(f&&ps_preferences_write(f,&p));rewind(f);assert(ps_preferences_read(f,&q)&&q.windowed==size);fclose(f);
#endif
 }
 a.windowed=4;FILE*f=tmpfile();assert(f&&ui_write(f,&a));rewind(f);assert(!ui_read(f,&b));fclose(f);
 ui_dispose(&a);ui_dispose(&b);puts("PASS fullscreen/1x/2x/3x settings persist; invalid mode rejected");
}
