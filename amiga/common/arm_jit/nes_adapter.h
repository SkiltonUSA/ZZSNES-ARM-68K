/* GPL-3.0-or-later. Included in FCEUX x6502.cpp after its register definitions. */
#include <stddef.h>
#include "cache.h"
extern "C" int console_jit_init(uintptr_t base){return cj_init(base);}
static int console_jit_op(unsigned op){
 unsigned src,dst,kind=1;int delta=0;
 switch(op){
 case 0xaa:src=offsetof(X6502,A);dst=offsetof(X6502,X);break;
 case 0x8a:src=offsetof(X6502,X);dst=offsetof(X6502,A);break;
 case 0xa8:src=offsetof(X6502,A);dst=offsetof(X6502,Y);break;
 case 0x98:src=offsetof(X6502,Y);dst=offsetof(X6502,A);break;
 case 0xba:src=offsetof(X6502,S);dst=offsetof(X6502,X);break;
 case 0x9a:src=offsetof(X6502,X);dst=offsetof(X6502,S);kind=0;break;
 case 0xe8:src=dst=offsetof(X6502,X);delta=1;break;
 case 0xc8:src=dst=offsetof(X6502,Y);delta=1;break;
 case 0xca:src=dst=offsetof(X6502,X);delta=-1;break;
 case 0x88:src=dst=offsetof(X6502,Y);delta=-1;break;
 default:return 0;
 }
 cj_execute((uint16_t)(_PC-1),op,&X,0,src,dst,delta,8,kind,offsetof(X6502,P),0,0);
 return 1;
}
