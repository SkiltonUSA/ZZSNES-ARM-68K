/* GPL-3.0-or-later. Software scanlines from ZZDosBox pc/scanline_plan.h.
 * Identical 0/8, 5/8 and 8/8,6/8,4/8,6/8 levels; no FPGA dependency. */
#pragma once
#include <stdint.h>
static inline unsigned em_scan_level(unsigned mode,unsigned row){
 if(mode==4)return (row&1)?0:8;
 if(mode==1)return (row&1)?0:8;
 if(mode==2)return (row&1)?5:8;
 if(mode==3){const unsigned levels[4]={8,6,4,6};return levels[row&3];}
 return 8;
}
static inline uint32_t em_scan_dim(uint32_t p,unsigned level){
 uint32_t lo=(((p&0x00ff00ffu)*level)>>3)&0x00ff00ffu;
 uint32_t hi=(((((p>>8)&0x00ff00ffu)*level)>>3)&0x00ff00ffu)<<8;
 return lo|hi;
}
static inline int em_fps_ink(unsigned x,unsigned y,unsigned fps){
 /* Compact F P S and three digits, a black 40x9 backing panel. */
 static const uint16_t digits[]={0x7b6f,0x2492,0x73e7,0x73cf,0x5bc9,0x79cf,0x79ef,0x7249,0x7bef,0x7bcf};
 static const uint16_t labels[]={0x79a4,0x7be4,0x79cf};
 if(x<2||x>=42||y<2||y>=11)return -1;
 x-=4;y-=4;if(y>=5||x>=28)return 0;
 unsigned cell=x/4,col=x%4;uint16_t bits;
 if(col==3||cell==3)return 0;
 if(cell<3)bits=labels[cell];
 else {unsigned v=cell==4?(fps/100)%10:cell==5?(fps/10)%10:fps%10;bits=digits[v];}
 return (bits>>(14-(y*3+col)))&1;
}
static inline uint32_t em_effect(uint32_t color,unsigned x,unsigned y,unsigned scan,unsigned fps){
 unsigned level=em_scan_level(scan,y);if(level!=8)color=em_scan_dim(color,level);
 if(fps){int v=em_fps_ink(x,y,fps-1);if(v>=0)color=v?0xffffffffu:0xff000000u;}
 return color;
}
