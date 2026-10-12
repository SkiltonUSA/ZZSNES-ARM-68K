/* GPL-3.0-or-later. Explicit first-player cartridge contract. */
#pragma once
#include <stdint.h>
static inline int nes_cartridge(const uint8_t *r,uint32_t n){
 unsigned id;
 if(!r||n<16||r[0]!='N'||r[1]!='E'||r[2]!='S'||r[3]!=26||!r[4]||
    (r[4]&(r[4]-1))||(r[5]&&(r[5]&(r[5]-1)))||(r[6]&4)||(r[7]&15)||
    r[8]>4||r[8]==3||r[9]||r[10]||r[11]||r[12]||r[13]||r[14]||r[15]||
    n!=16u+r[4]*16384u+r[5]*8192u)return -1;
 id=(r[6]>>4)|(r[7]&0xf0);
 if(id!=0&&id!=1&&id!=2&&id!=3&&id!=4&&id!=7&&id!=90)return -1;
 if((id==0&&(r[4]>2||r[5]>1))||(id==1&&(r[4]>32||r[5]>16))||
    (id==2&&(r[4]>16||r[5]))||(id==3&&(r[4]>2||r[5]>4))||
    (id==4&&(r[4]<2||r[4]>32||r[5]>32))||(id==7&&(r[4]<2||r[4]>16||r[5])))return -1;
 if(id==90&&(r[4]<2||r[4]>32||!r[5]||r[5]>32||(r[6]&10)||r[8]))return -1;
 if(id!=1&&r[8]>1)return -1;
 return (int)id;
}
