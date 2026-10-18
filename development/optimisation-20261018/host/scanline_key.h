/* GPL-3.0-or-later. Session-only F7 cycle; ignore releases and auto-repeat. */
#ifndef EM_SCANLINE_KEY_H
#define EM_SCANLINE_KEY_H
static unsigned em_scanline_next(unsigned mode){
 switch(mode){case 0:return 2;case 2:return 3;case 3:return 1;default:return 0;}
}
static void em_scanline_key(unsigned code,unsigned repeat,unsigned current,int *pending){
 if(code!=0x56||repeat)return;
 *pending=(int)em_scanline_next(*pending<0?current:(unsigned)*pending);
}
#endif
