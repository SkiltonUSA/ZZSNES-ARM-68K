#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#define UI_CONFIG_PREFIX "ZZNESUI"
#include "ui_model.h"
#ifdef BORDER_NES
static uint32_t nes_player_palette[512];
#endif
#include "view.h"
int main(void){
 struct ui_model m={0},n={0};ui_defaults(&m);m.theme=7;FILE*f=tmpfile();assert(f&&ui_write(f,&m));rewind(f);assert(ui_read(f,&n)&&n.theme==7);fclose(f);ui_dispose(&m);ui_dispose(&n);
 unsigned colored=0;
 for(unsigned opt=0;opt<256;opt++){
  if(((opt>>1)&7)!=7||!fv_valid(opt))continue;
  unsigned base=opt&~14u;
  for(unsigned y=0;y<240;y++)for(unsigned x=0;x<320;x++){
   assert(fv_source(x,opt)==fv_source(x,base));
   unsigned c=fv_border(x,y,opt);assert((c>>24)==255);
   unsigned left=(320-fv_width(opt))/2;
   if(x>=left&&x<320-left)assert(c==0xff000000u);else colored+=c!=0xff000000u;
  }
 }
 assert(colored);assert(border_art(320,0,7,240)==0xff000000u);assert(border_art(0,240,7,240)==0xff000000u);
 puts("PASS EmuELEC theme persistence, valid geometry, panel bounds and opaque output");
}
