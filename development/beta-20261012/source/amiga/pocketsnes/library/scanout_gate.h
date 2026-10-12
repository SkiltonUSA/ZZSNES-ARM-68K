/* GPL-3.0-or-later. Experimental host-observed blanking gate, NOT a DMA fence.
 * 0: no delay. 1: finish current blank if ACK was observed in blank.
 * 2: observe an active->blank->active sequence after ACK (one extra blank).
 * All waits bounded independently of frame-rate assumptions. */
#pragma once
#include <stdint.h>
struct sg_gate { unsigned phase; uint32_t start; };
static inline void sg_begin(struct sg_gate *g,unsigned mode,int blank,uint32_t now){
 g->start=now;
 g->phase=mode==2?(blank?1:2):mode==1&&blank?3:0;
}
static inline int sg_poll(struct sg_gate *g,int blank,uint32_t now){
 if(!g->phase)return 1;
 if((uint32_t)(now-g->start)>=50000u)return -1;
 if(g->phase==1){if(!blank)g->phase=2;}
 else if(g->phase==2){if(blank)g->phase=3;}
 else if(g->phase==3){if(!blank)g->phase=0;}
 else return -1;
 return g->phase?0:1;
}
