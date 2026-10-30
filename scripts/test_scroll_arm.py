"""Compare real A9 conversion with an independent scalar host reference."""
import importlib.util,struct,subprocess
from pathlib import Path
from unicorn.arm_const import *
r=Path(__file__).resolve().parents[1];o=r/'.context/scroll-model';o.mkdir(parents=True,exist_ok=True)
spec=importlib.util.spec_from_file_location('engine',r/'tests/amiga/emulator/check_current_arm.py');m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m);m.block=0x6000000
spec=importlib.util.spec_from_file_location('pack',r/'.context/ax-release-build/core/elf.py');pack=importlib.util.module_from_spec(spec);spec.loader.exec_module(pack);m.unpack=pack.unpack
c=r'''#include <cstdio>
#include <cstdlib>
#include <stdint.h>
#include "view.h"
int main(int argc,char**argv){if(argc!=2)return 1;unsigned opt=atoi(argv[1]);uint16_t src[256*240];for(unsigned i=0;i<256*240;i++)src[i]=(i*17+i/256*137)&65535;for(unsigned y=0;y<240;y++)for(unsigned x=0;x<320;x++){uint32_t c=fv_color(src,x,y,opt);unsigned char p[4]={(unsigned char)c,(unsigned char)(c>>8),(unsigned char)(c>>16),(unsigned char)(c>>24)};fwrite(p,1,4,stdout);}return 0;}
'''
(o/'reference.cpp').write_text(c);subprocess.run(['/usr/bin/clang++','-O2','-std=c++14','-iquote'+str(r/'.context/ax-release-build/player'),str(o/'reference.cpp'),'-o',str(o/'reference')],check=True)
m.raw=struct.pack('<61440H',*[(i*17+i//256*137)&65535 for i in range(61440)])
for base in (0x12000040,0x150020c0):
 u,call,addr,mapping,l1,counts=m.engine(r/'.context/ax-release-build/player/pocket.elf',base);cmd=base+m.control+0x800
 for opt in (0,80,256,512,592,768):
  reference=subprocess.check_output([str(o/'reference'),str(opt)])
  for fmt in (3,4):
   for neon in (False,True):
    u.reg_write(UC_ARM_REG_C1_C0_2,0xf00000);u.ctl_flush_tb();u.reg_write(UC_ARM_REG_FPEXC,0x40000000)
    vals=[(i+19)*0x0102030405060708&0xffffffffffffffff for i in range(32)]
    for i,v in enumerate(vals):u.reg_write(UC_ARM_REG_D0+i,v)
    u.reg_write(UC_ARM_REG_FPSCR,0x1000000)
    if not neon:u.reg_write(UC_ARM_REG_C1_C0_2,0);u.ctl_flush_tb()
    pitch=1296;span=239*pitch+1280;nonce=0x12983746
    u.mem_write(mapping,b'\xa5'*0x200000);u.mem_write(addr,struct.pack('>I',nonce));u.mem_write(addr+span-4,struct.pack('>I',nonce^0x31415926))
    u.mem_write(cmd+32,struct.pack('>IIII',opt,0,1,0));assert call('nes_present',base,addr,pitch,fmt,nonce)==0
    actual=bytes(u.mem_read(addr,span));expected=reference if fmt==3 else b''.join(reference[i:i+4][::-1]for i in range(0,len(reference),4))
    for y in range(240):assert actual[y*pitch:y*pitch+1280]==expected[y*1280:(y+1)*1280],(hex(base),opt,fmt,neon,y)
    assert bytes(u.mem_read(l1,16384))==bytes(16384)
    u.reg_write(UC_ARM_REG_C1_C0_2,0xf00000);assert [u.reg_read(UC_ARM_REG_D0+i)for i in range(32)]==vals;assert u.reg_read(UC_ARM_REG_FPSCR)==0x1000000
  print('PASS',hex(base),'view',opt,'scalar pixels, formats, NEON/fallback and SIMD/MMU restoration',flush=True)
 assert counts['blend']>0
