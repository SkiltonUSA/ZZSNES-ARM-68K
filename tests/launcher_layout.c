#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../development/scalable-launcher-20261019/source/library_layout.h"
int main(void){
 const unsigned screens[][2]={{640,480},{800,600},{1024,768},{1920,800},{640,256}};
 unsigned i;
 for(i=0;i<sizeof(screens)/sizeof(screens[0]);i++){
  struct ui_layout l=ui_layout_for_screen(screens[i][0],screens[i][1],20);
  assert(l.width+32<=screens[i][0]);assert(l.height+60<=screens[i][1]);
  assert(l.width<=940&&l.height<=500);
  assert(l.compact==(screens[i][0]<=800||screens[i][1]<480));
 }
 {char text[]="one two three four\nfive six seven";
  ui_wrap_text(text,9);assert(!strcmp(text,"one two\nthree\nfour\nfive six\nseven"));
 }
 puts("PASS: launcher screen budgets and compact selection");return 0;
}
