"""Pinned mapper integration; legacy mapper0/90 build stays unchanged."""
from pathlib import Path
import re,json,hashlib,shutil
import build_fceux_bench as b
ROOT=b.ROOT;SRC=ROOT/'amiga/fceux_player/compat';OUT=ROOT/'.context/amiga/fceux-compat'
def replace(s,a,z):assert a in s,a;return s.replace(a,z)
def prepare():
 b.OUT=OUT;sources=b.prepare()
 from fceux_states import prepare as states
 states(OUT)
 for n in ['cartridge.h']:shutil.copy2(SRC/n,OUT/n)
 pin=json.loads((SRC/'vendor/upstream.json').read_text())
 for n,h in pin['files'].items():
  raw=(SRC/'vendor'/n).read_bytes();assert hashlib.sha256(raw).hexdigest()==h
  s=raw.decode("latin1").replace('\r\n','\n')
  if n.endswith('.cpp'):
   s=re.sub(r'^#include .*\n','',s,flags=re.M);s='#include "platform.h"\n'+('#include "mmc3.h"\n' if n=='mmc3.cpp' else '')+s
   sources.append(OUT/n)
  if n=='mmc1.cpp':s=s.replace('FCEU_dwmemset(WRAM, 0, 8192)\n','FCEU_dwmemset(WRAM, 0, 8192);\n')
  if n=='mmc3.cpp':s='extern unsigned char *UNIFchrrama;\n'+s
  (OUT/n).write_text(s)
 # Save transfer lies after the maximum ROM, before BSS; no allocation growth.
 s=(ROOT/'amiga/fceux_player/state_core.h').read_text().replace('32769','122881').replace('32768','122880').replace('32736','122848')
 s=s.replace('state_put(p+28,0);','state_put(p+28,PAL);').replace('state_get(p+28)||','state_get(p+28)!=PAL||')
 (OUT/'state_core.h').write_text(s)
 s=(b.SRC/'core.cpp').read_text().replace('#include "core.h"','#include "core.h"\n#include "cartridge.h"\nvoid Mapper1_Init(CartInfo*);void Mapper4_Init(CartInfo*);void Mapper5_Init(CartInfo*);void UNROM_Init(CartInfo*);\nstatic unsigned chosen_pal;void fc_set_region(unsigned v){chosen_pal=v;}unsigned fc_get_region(){return PAL;}')
 s=s.replace('uint8 PAL=0,mmc5ABMode=0;','uint8 PAL=0;uint8 *UNIFchrrama=nullptr;').replace('void MMC5_hb(int){FC_Error=3;}','')
 start=s.index(' if(loaded||size<16');end=s.index(' loaded=1;',start)
 s=s[:start]+''' int id=nes_cartridge(rom,size);if(loaded||id<0||chosen_pal>1)return 1;
 unsigned prg=rom[4]*16384u,chr=rom[5]*8192u;PAL=chosen_pal;paldeemphswap=PAL;
'''+s[end:]
 s=s.replace('info.vidsys=GIV_NTSC','info.vidsys=PAL?GIV_PAL:GIV_NTSC').replace('FCEUPPU_SetVideoSystem(0)','FCEUPPU_SetVideoSystem(PAL)')
 s=s.replace(' SetupCartCHRMapping(0,(uint8 *)rom+16+prg,chr,0);', ''' if(chr)SetupCartCHRMapping(0,(uint8 *)rom+16+prg,chr,0);
 else {uint8*p=(uint8*)FCEU_gmalloc(8192);if(!p)return 11;SetupCartCHRMapping(0,p,8192,1);AddExState(p,8192,0,"CHRR");}''')
 s=s.replace(' if(id==90)Mapper90_Init(&cart);',''' cart.mirror=rom[6]&1;cart.battery=0;
 uint32 crc=~0u;for(unsigned i=16;i<size;i++){crc^=rom[i];for(unsigned k=0;k<8;k++)crc=(crc>>1)^((0u-(crc&1))&0xedb88320u);}cart.CRC32=~crc;
 if(id==1)Mapper1_Init(&cart);else if(id==2)UNROM_Init(&cart);else if(id==4)Mapper4_Init(&cart);else if(id==5)Mapper5_Init(&cart);else if(id==90)Mapper90_Init(&cart);
 if(FC_Error)return FC_Error;''')
 s=s.replace(' if(id==90)cart.Power();else',' if(id)cart.Power();else')
 (OUT/'compat_core.cpp').write_text(s)
 # Fixed-point ARM constants must use the same region as the floating native oracle.
 for name in ['sound.cpp','filter.cpp']:
  f=OUT/name;s=f.read_text()
  for expr,ntsc,pal in [('nesincsize',int((1<<17)*1789772.7272727272/(32000*16)),int((1<<17)*1662607.125/(32000*16))),('soundtsinc',int(1789772.7272727272*65536)//512000,int(1662607.125*65536)//512000),('mrratio',int(1789772.7272727272*65536)//32000,int(1662607.125*65536)//32000)]:
   s=s.replace(f'{expr}={ntsc};',f'{expr}=PAL?{pal}:{ntsc};')
  f.write_text(s)
 return sources+[OUT/'compat_core.cpp',SRC/'memory.cpp']
