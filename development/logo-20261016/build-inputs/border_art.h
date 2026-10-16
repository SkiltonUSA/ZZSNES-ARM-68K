/* Shared side-panel sampler. Coordinates are 320x240 presentation pixels.
 * Game viewport and overscan masks are independent of the artwork. */
#pragma once
#include "border_assets.h"
static inline uint32_t border_art(unsigned x,unsigned y,unsigned theme,unsigned width){
 unsigned left=(320-width)/2,side,u;uint16_t p;
 if(y>=240||x>=320||!left||theme<5||theme>6)return 0xff000000u;
 if(x<left){side=0;u=x*40/left;}
 else if(x>=320-left){side=1;u=(x-(320-left))*40/left;}
 else return 0xff000000u; /* covered game edges stay black */
 p=(theme==5?border_nes:border_snes)[side*9600+y*40+u];
 unsigned r=(p>>11)&31,g=(p>>5)&63,b=p&31;
 return 0xff000000u|(((r<<3)|(r>>2))<<16)|(((g<<2)|(g>>4))<<8)|(b<<3)|(b>>2);
}
