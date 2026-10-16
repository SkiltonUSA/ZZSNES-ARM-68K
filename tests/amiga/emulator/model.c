#include <assert.h>
#include <stdio.h>
#include "ui_model.h"
#include "display_math.h"
static struct ui_model a,b;
int main(void){
 ui_defaults(&a);assert(a.optimise&&a.autoscan);assert(ui_add(&a,"USB0:Games/Test.nes")==0);
 assert(ui_add(&a,"usb0:games/test.nes")==0&&a.count==1);
 a.windowed=1;a.scanlines=3;a.fps=1;a.optimise=0;
 FILE*f=tmpfile();assert(f&&ui_write(f,&a));rewind(f);assert(ui_read(f,&b));fclose(f);
 assert(b.count==1&&b.windowed==1&&b.scanlines==3&&b.fps==1&&!b.optimise);
 f=tmpfile();fputs(UI_CONFIG_PREFIX "2\n0 0 0 0 0 0 0 0 0 0\n",f);rewind(f);assert(ui_read(f,&b));fclose(f);assert(b.optimise&&b.windowed==0&&b.scanlines==0);
 for(unsigned value=0;value<256;value++)for(unsigned level=0;level<=8;level++){
 uint32_t p=value*0x01010101u;assert(em_scan_dim(p,level)==((value*level)>>3)*0x01010101u);}
 for(unsigned mode=0;mode<4;mode++)for(unsigned row=0;row<240;row++)assert(em_scan_level(mode,row)<=8);
 for(unsigned fps=0;fps<1000;fps++)for(unsigned y=0;y<12;y++)for(unsigned x=0;x<44;x++){int v=em_fps_ink(x,y,fps);assert(v>=-1&&v<=1);}
 ui_dispose(&a);ui_dispose(&b);
 puts("PASS settings v2/v3 migration, bounded scanline arithmetic and FPS glyph bounds");return 0;
}
