#include <assert.h>
#include "ui_model.h"
int main(void){struct ui_model m={0},r={0};
 for(unsigned v=0;v<64;v++)if(ps_options_valid(v)){
  ui_defaults(&m);m.core_options=v;assert(ui_add(&m,"SD032G:ROMS/Test.zip")==0);
  FILE*f=tmpfile();assert(f&&ui_write(f,&m));rewind(f);assert(ui_read(f,&r));fclose(f);
  assert(r.core_options==v&&r.count==1&&!strcmp(m.entry[0].path,r.entry[0].path));
 }
 FILE*f=tmpfile();assert(f);fputs(UI_CONFIG_PREFIX "4\n0 0 2 0 0 0 0 0 0 0 0 2 0 1 0 0\n",f);rewind(f);assert(ui_read(f,&r));fclose(f);assert(r.core_options==0&&r.scanlines==2);
 ui_dispose(&m);ui_dispose(&r);puts("PASS core-option round trips and v4 preference migration");return 0;}
