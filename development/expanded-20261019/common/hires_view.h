#pragma once
/* 512-wide SNES image scaled into the same bezel as the 320-wide output.
 * Both hosts use this mapping for independent framebuffer verification. */
static int ps_hires_x(unsigned x,unsigned opt){
 unsigned w=fv_width(opt)*2,left=(640-w)/2,m=((opt>>4)&3)*16;
 if(x<left||x>=left+w)return -1;
 unsigned sx=(x-left)*512/w;return sx<m||sx>=512-m?-1:(int)sx;
}
static int ps_hires_y(unsigned y,unsigned opt){
 unsigned h=opt&256?478:448,m=((opt>>6)&3)*16;
 if(y<24||y>=456)return -1;
 unsigned sy=(y-24)*h/432;return sy<m||sy>=h-m?-1:(int)sy+(h==448?16:0);
}
