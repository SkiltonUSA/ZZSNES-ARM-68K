#include <assert.h>
#include <stdio.h>
#include "native_height.h"
int main(void) {
 for(unsigned height=224;height<=239;height+=15)for(unsigned mask=0;mask<=16;mask+=8){
  unsigned seen[240]={0},count=0,opt=(height==239?256u:0u)|((mask/8)<<6);
  for(unsigned y=0;y<242;y++){
   int row=ps_native_y(y,opt);
   if(row<0)continue;
   assert(row<240);assert(!seen[row]++);count++;
   /* Expected placement independently expressed as output row intervals. */
   if(height==224)assert(row==(int)y && y>=8+mask && y<232-mask);
   else assert(row==(int)y && y>=mask && y<239-mask);
  }
  assert(count==height-2*mask);
  for(unsigned row=0;row<240;row++){
   unsigned start=(height==224?8:0)+mask,end=(height==224?232:239)-mask;
   assert(seen[row]==(unsigned)(row>=start&&row<end));
  }
 }
 puts("PASS native 224/239 rows exactly once; centered placement and 0/8/16 overscan masks");
}
