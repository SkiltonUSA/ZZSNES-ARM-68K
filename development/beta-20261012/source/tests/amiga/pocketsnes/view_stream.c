/* Native reference for the instruction-level guarded framebuffer test. */
#include <stdio.h>
#include <stdlib.h>
#include "view_fast.h"
int main(int argc,char **argv){
 uint16_t src[61440];uint32_t row[320];
 if(argc!=3 || fread(src,sizeof(src),1,stdin)!=1)return 1;
 unsigned format=(unsigned)atoi(argv[1]),opt=(unsigned)atoi(argv[2]);
 if(!fv_valid(opt)||(format!=3&&format!=4))return 1;
 for(unsigned y=0;y<240;y++){
  fv_row(row,src,y,opt,format);
  if(fwrite(row,sizeof(row),1,stdout)!=1)return 1;
 }
 return 0;
}
