/* GPL-3.0-or-later. Pure controller decoders; no Amiga or USB access. */
#pragma once
#include <stdint.h>
#include <string.h>
struct pad_field {unsigned offset,size,page,usage,id;int32_t min,max;};
struct pad_hid {struct pad_field fields[64];unsigned count,bits[256],ids;};
struct pad_globals {unsigned page,size,count,id;int32_t min,max;};
static int32_t pad_signed(uint32_t v,unsigned bits){
 if(bits<32&&(v&(1u<<(bits-1))))v|=~((1u<<bits)-1);return (int32_t)v;
}
static int pad_hid_parse(struct pad_hid*h,const unsigned char*d,unsigned len){
 struct pad_globals g={0},stack[4];unsigned sp=0,depth=0,gamepad=0,usage[32],nusage=0,umin=0,umax=0,range=0;
 memset(h,0,sizeof(*h));
 for(unsigned at=0;at<len;){
  unsigned b=d[at++],size=b&3,type=(b>>2)&3,tag=b>>4;uint32_t v=0;
  if(b==254)return 0;if(size==3)size=4;if(at+size>len)return 0;
  for(unsigned i=0;i<size;i++)v|=(uint32_t)d[at++]<<(8*i);
  if(type==1){
   switch(tag){case 0:g.page=v;break;
   case 1:if(!size)return 0;g.min=pad_signed(v,size*8);break;
   case 2:if(!size)return 0;g.max=g.min<0?pad_signed(v,size*8):(int32_t)v;break;
   case 7:g.size=v;break;case 8:if(!v||v>255)return 0;g.id=v;h->ids=1;break;
   case 9:g.count=v;break;case 10:if(sp==4)return 0;stack[sp++]=g;break;
   case 11:if(!sp)return 0;g=stack[--sp];break;default:break;}
  }else if(type==2){
   if(tag==0){if(nusage==32)return 0;usage[nusage++]=v;}
   else if(tag==1){umin=v;range=1;}else if(tag==2)umax=v;
  }else if(type==0){
   if(tag==10){if(!depth&&g.page==1&&nusage&&(usage[0]==4||usage[0]==5))gamepad=1;if(++depth>16)return 0;}
   else if(tag==12){if(!depth)return 0;depth--;}
   else if(tag==8){
    if(!depth||!g.size||g.size>32||g.count>512||h->bits[g.id]+g.size*g.count>512)return 0;
    for(unsigned i=0;i<g.count;i++){
     unsigned u=range?(umin+i<=umax?umin+i:0):(i<nusage?usage[i]:0);
     if(!(v&1)&&((g.page==9&&u<=32&&u)||(g.page==1&&(u==0x30||u==0x31||u==0x39)))){
      if(!(v&2)||h->count==64||g.max<=g.min)return 0;
      struct pad_field*f=&h->fields[h->count++];
      f->offset=h->bits[g.id]+i*g.size;f->size=g.size;f->page=g.page;f->usage=u;f->id=g.id;f->min=g.min;f->max=g.max;
     }
    }h->bits[g.id]+=g.size*g.count;
   }
   nusage=range=umin=umax=0;
  }
 }
 return gamepad&&!depth&&!sp&&h->count&&(!h->ids||!h->bits[0]);
}
static unsigned pad_sn30_button(unsigned usage){
 /* SN30 Pro D-input 2dc8:6001. Match Nintendo labels, not Xbox letters.
    HID usage numbers are one-based; L2/R2/L3/R3/Home are unused. */
 switch(usage){case 1:return 256;case 2:return 1;case 4:return 512;case 5:return 2;
 case 7:return 1024;case 8:return 2048;case 11:return 4;case 12:return 8;default:return 0;}
}
static int pad_sn30_profile(const struct pad_hid*h){
 unsigned buttons=0,axes=0,id=256;
 for(unsigned i=0;i<h->count;i++){
  const struct pad_field*f=&h->fields[i];
  unsigned button=f->page==9?pad_sn30_button(f->usage):0;
  unsigned axis=f->page==1?(f->usage==0x30?1:f->usage==0x31?2:f->usage==0x39?4:0):0;
  if(!button&&!axis)continue;
  if(id==256)id=f->id;else if(id!=f->id)return 0;
  buttons|=button;axes|=axis;
 }
 return buttons==0xf0f&&((axes&3)==3||(axes&4))&&
        (h->bits[id]+7)/8+h->ids<=64;
}
static int pad_hid_read(const struct pad_hid*h,const unsigned char*d,unsigned len,unsigned*out){
 unsigned id=0,bits=0;static const unsigned hats[]={16,16|128,128,128|32,32,32|64,64,64|16};
 if(h->ids){if(!len)return 0;id=*d++;len--;}
 if(!h->bits[id]||len<(h->bits[id]+7)/8)return 0;
 for(unsigned i=0;i<h->count;i++){
  const struct pad_field*f=&h->fields[i];uint32_t v=0;if(f->id!=id)continue;
  for(unsigned j=0;j<f->size;j++)v|=(uint32_t)((d[(f->offset+j)/8]>>((f->offset+j)&7))&1)<<j;
  int32_t n=f->min<0?pad_signed(v,f->size):(int32_t)v;
  if(f->page==9){if(n)bits|=pad_sn30_button(f->usage);}
  else if(f->usage==0x39){int64_t hat=(int64_t)n-f->min;if(hat>=0&&hat<8)bits|=hats[hat];}
  else{int64_t range=(int64_t)f->max-f->min,pos=(int64_t)n-f->min;
   if(pos<range/4)bits|=f->usage==0x30?64:16;
   else if(pos>range*3/4)bits|=f->usage==0x30?128:32;}
 }
 *out=bits;return 1;
}
static unsigned pad_sega(unsigned normal,unsigned low,unsigned identify,unsigned extra,unsigned end){
 unsigned bits=0;if((low&12)!=12)return 0;
 if(normal&1)bits|=16;if(normal&2)bits|=32;if(normal&4)bits|=64;if(normal&8)bits|=128;
 if(normal&16)bits|=1;if(normal&32)bits|=256;if(low&16)bits|=2;if(low&32)bits|=8;
 if((identify&15)==15&&(end&15)==0){
  if(extra&4)bits|=512;if(extra&2)bits|=1024;if(extra&1)bits|=2048;if(extra&8)bits|=4;
 }return bits;
}
