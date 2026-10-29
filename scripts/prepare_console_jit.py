"""Prepare isolated register-JIT source overlay; never edits the upstream core."""
import shutil,re
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def prepare(core,stage,snes):
 for p in (ROOT/'amiga/common/arm_jit').glob('*.h'):
  shutil.copy2(p,stage/p.name)
  if p.name=='cache_size.h':shutil.copy2(p,core/p.name)
 begin=0x3400000 if snes else 0x600000
 for p in (core/'mmu_plan.h',stage/'mmu_plan.h'):
  if not p.exists():continue
  s=p.read_text();s='#include "cache_size.h"\n'+s;s=s.replace('if(page>=base+MB_TABLE_BEGIN)attr|=1u;', 'if(page>=base+MB_TABLE_BEGIN && !(page>=((base+CJ_BEGIN+4095u)&~4095u) && page<((base+CJ_BEGIN+4095u)&~4095u)+CJ_BYTES))attr|=1u;')
  s=s.replace('#define MB_TABLE_END 0x11c000u','#define MB_TABLE_END 0x128000u').replace('#define MB_WITNESS 0x120000u','#define MB_WITNESS 0x12c000u')
  p.write_text(s)
 p=core/'worker.cpp';s=p.read_text();s=s.replace('extern "C" void zz_worker', 'extern "C" int console_jit_init(uintptr_t);\nextern "C" void zz_worker');s=s.replace(' for(unsigned i=0;i<256;i++)', ' if(!console_jit_init((uintptr_t)base))return;\n for(unsigned i=0;i<256;i++)');p.write_text(s)
 p=core/('src/cpuexec.c' if snes else 'x6502.cpp');s=p.read_text()
 if snes:
  marker='void S9xMainLoop';at=s.index(marker);s=s[:at]+'#include "snes_adapter.h"\n'+s[at:]
  s,count=re.subn(r'\(\*ICPU\.S9xOpcodes\[\*CPU\.PC\+\+\]\.S9xOpcode\)\s*\(\);', '{unsigned opcode=*CPU.PC++;if(!console_jit_op(opcode))(*ICPU.S9xOpcodes[opcode].S9xOpcode)();}',s)
  assert count==4,count
 else:
  s=s.replace('void X6502_Run(int32 cycles)', '#include "nes_adapter.h"\nvoid X6502_Run(int32 cycles)')
  assert s.count('   switch(b1)')==1;s=s.replace('   switch(b1)','   if(!console_jit_op(b1))switch(b1)')
 p.write_text(s)
 if not snes:
  p=core/'payload.ld';p.write_text(p.read_text().replace('0x3d0000','0x5d0000'))
  p=core/'elf.py';p.write_text(p.read_text().replace('0x1b0000','0x3b0000'))
 return begin
