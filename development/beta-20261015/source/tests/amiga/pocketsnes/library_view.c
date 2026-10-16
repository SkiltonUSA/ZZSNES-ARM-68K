#include <stdio.h>
#include <stdlib.h>
#include "view.h"
int main(int argc,char**argv){if(argc!=3)return 1;unsigned f=atoi(argv[1]),o=atoi(argv[2]);if((f!=3&&f!=4)||!fv_valid(o))return 1;uint16_t src[61440];uint32_t row[320];if(fread(src,2,61440,stdin)!=61440)return 1;for(unsigned y=0;y<240;y++){for(unsigned x=0;x<320;x++){uint32_t c=fv_color(src,x,y,o);row[x]=f==3?c:__builtin_bswap32(c);}if(fwrite(row,4,320,stdout)!=320)return 1;}return 0;}
