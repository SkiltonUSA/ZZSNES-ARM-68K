/* GPL-3.0-or-later. Local library thumbnail; never an emulator save state. */
#ifndef ZZ_SNAPSHOT_FORMAT_H
#define ZZ_SNAPSHOT_FORMAT_H
#include <stdint.h>
#include <string.h>
#define SS_WIDTH 128u
#define SS_HEIGHT 112u
#define SS_HEADER 32u
#define SS_BYTES (SS_HEADER+SS_WIDTH*SS_HEIGHT*2u)
static uint32_t ss_get(const uint8_t*p,unsigned n){return (uint32_t)p[n]<<24|(uint32_t)p[n+1]<<16|(uint32_t)p[n+2]<<8|p[n+3];}
static void ss_put(uint8_t*p,unsigned n,uint32_t v){p[n]=v>>24;p[n+1]=v>>16;p[n+2]=v>>8;p[n+3]=v;}
static uint32_t ss_hash(const uint8_t*p,unsigned n){uint32_t h=2166136261u;for(unsigned i=0;i<n;i++)h=(h^p[i])*16777619u;return h;}
static uint32_t ss_crc(const uint8_t*p){uint32_t h=2166136261u;for(unsigned i=0;i<SS_BYTES;i++)h=(h^(i>=28&&i<32?0:p[i]))*16777619u;return h;}
static int ss_valid(const uint8_t*p,unsigned n,uint32_t id,uint32_t id2,unsigned bytes){return n==SS_BYTES&&!memcmp(p,"ZZSHOT1",8)&&ss_get(p,8)==SS_WIDTH&&ss_get(p,12)==SS_HEIGHT&&ss_get(p,16)==id&&ss_get(p,20)==id2&&ss_get(p,24)==bytes&&ss_get(p,28)==ss_crc(p);}
/* Source is an acknowledged private host copy, not live video memory.
 * Both supported 32-bit P96 orders; crop console surround and normalize the
 * physical game aspect to 4:3 (128x96 inside the shared 128x112 preview). */
static int ss_encode(uint8_t*out,const uint8_t*src,unsigned size,unsigned pitch,unsigned format,
 unsigned x0,unsigned y0,unsigned width,unsigned height,uint32_t id,uint32_t id2,unsigned bytes){
 if(!src||!out||(format!=3&&format!=4)||pitch<1280||pitch>4096||(pitch&3)||
    !width||!height||x0>=320||y0>=240||width>320-x0||height>240-y0||size<239*pitch+1280)return 0;
 memset(out,0,SS_BYTES);memcpy(out,"ZZSHOT1",8);ss_put(out,8,SS_WIDTH);ss_put(out,12,SS_HEIGHT);ss_put(out,16,id);ss_put(out,20,id2);ss_put(out,24,bytes);
 for(unsigned y=0;y<96;y++){for(unsigned x=0;x<128;x++){
  const uint8_t*p=src+(y0+y*height/96)*pitch+(x0+x*width/128)*4;unsigned r,g,b;
  if(format==3){b=p[0];g=p[1];r=p[2];}else{r=p[1];g=p[2];b=p[3];}
  unsigned c=((r&248)<<8)|((g&252)<<3)|(b>>3),at=SS_HEADER+((y+8)*128+x)*2;out[at]=c>>8;out[at+1]=c;
 }}
 ss_put(out,28,ss_crc(out));return 1;
}
static int ss_nonblank(const uint8_t*p){unsigned first=SS_HEADER+8*128*2;for(unsigned i=first+2;i<SS_HEADER+104*128*2;i+=2)if(p[i]!=p[first]||p[i+1]!=p[first+1])return 1;return 0;}
#endif
