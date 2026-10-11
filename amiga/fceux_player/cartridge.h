/* GPL-3.0-or-later. Exactly the bounded FCEUX core's cartridge contract. */
#pragma once
#include <stdint.h>
static inline int nes_cartridge(const uint8_t *r,uint32_t n){
 unsigned id,prg,chr;
 if(!r||n<16||r[0]!='N'||r[1]!='E'||r[2]!='S'||r[3]!=26||(r[7]&15)||
    r[8]||r[9]||r[10]||r[11]||r[12]||r[13]||r[14]||r[15]||(r[6]&14))return -1;
 id=(r[6]>>4)|(r[7]&240);prg=r[4]*16384u;chr=r[5]*8192u;
 if((id!=0&&id!=90)||!prg||!chr||n!=16+prg+chr||prg>524288||chr>262144||
    (prg&(prg-1))||(chr&(chr-1))||(id==0&&(prg>32768||chr!=8192)))return -1;
 return (int)id;
}
