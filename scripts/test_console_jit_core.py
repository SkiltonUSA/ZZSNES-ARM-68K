#!/usr/bin/env python3
"""Compare isolated linked interpreter/register-JIT payloads in an A9 model.
No physical cache coherency or performance claim. ROM remains local.
"""
import argparse,importlib.util,struct,subprocess,hashlib,json
from pathlib import Path
from unicorn import Uc,UC_ARCH_ARM,UC_MODE_ARM,UC_HOOK_BLOCK
from unicorn.arm_const import *
class Machine:
 def __init__(self,elf,packer,block,base):
  s=importlib.util.spec_from_file_location('pack',packer);m=importlib.util.module_from_spec(s);s.loader.exec_module(m)
  image,rel,_=m.unpack(elf.read_bytes());self.base=base;self.block=block
  for at in rel:struct.pack_into('<I',image,at,struct.unpack_from('<I',image,at)[0]+base)
  self.syms={p[2]:int(p[0],16)+base for line in subprocess.check_output(['/opt/homebrew/opt/llvm/bin/llvm-nm',elf],text=True).splitlines() if len(p:=line.split())==3}
  self.u=Uc(UC_ARCH_ARM,UC_MODE_ARM);self.u.ctl_set_cpu_model(UC_CPU_ARM_CORTEX_A9);self.u.mem_map(base&~4095,block+8192);self.u.mem_write(base,bytes(image));self.stop=base+block-0x10000
  if 'console_jit_init' in self.syms:assert self.call('console_jit_init',base)==1
 def call(self,symbol,*args,limit=None):
  u=self.u;sp=self.base+self.block-0x2000
  u.reg_write(UC_ARM_REG_SP,sp);u.reg_write(UC_ARM_REG_LR,self.stop)
  for r,v in zip([UC_ARM_REG_R0,UC_ARM_REG_R1,UC_ARM_REG_R2,UC_ARM_REG_R3],args):u.reg_write(r,v)
  for i,v in enumerate(args[4:]):u.mem_write(sp+4*i,struct.pack('<I',v))
  u.emu_start(self.syms[symbol],self.stop,count=limit or (200000000 if symbol=='nes_init' else 50000000))
  assert u.reg_read(UC_ARM_REG_PC)==self.stop,(symbol,hex(u.reg_read(UC_ARM_REG_PC)))
  return u.reg_read(UC_ARM_REG_R0)
def main():
 p=argparse.ArgumentParser();p.add_argument('rom',type=Path);p.add_argument('--baseline',type=Path);p.add_argument('--candidate',type=Path);p.add_argument('--baseline-packer',type=Path);p.add_argument('--table-dispatch',action='store_true');p.add_argument('--candidate-mib',type=int);p.add_argument('--snes',action='store_true');p.add_argument('--frames',type=int,default=8);a=p.parse_args()
 repo=Path('.context')/('ZZSNES-ARM-68K' if a.snes else 'ZZNes-ARM-68K')
 ref=a.baseline or repo/'.context'/('expanded-build' if a.snes else 'optimisation-build')
 jit=a.candidate or repo/('.context/jit-dispatch32-build' if a.table_dispatch else '.context/jit-registers32-build')
 packer=a.baseline_packer or (ref/'core/elf.py' if a.snes else repo/'development/beta-20261015/source/core/elf.py')
 elf='pocket.elf' if a.snes else 'fceux.elf'
 machines=[Machine(ref/'player'/elf,packer,0x3000000 if a.snes else 0x400000,0x12000040),Machine(jit/'player'/elf,jit/'core/elf.py',(a.candidate_mib or (96 if a.snes else 48))*1024*1024,0x150020c0)]
 dispatch_calls=[0]
 if a.table_dispatch:
  assert a.snes
  m=machines[1];start=(m.base+0x3400000+4095)&~4095
  def entered(u,address,size,data):
   if (address-start)%256==0:dispatch_calls[0]+=1
  m.u.hook_add(UC_HOOK_BLOCK,entered,begin=start,end=start+2047)
  for wide in (0,1):
   for table in (('S9xOpcodesM1X0','S9xOpcodesM0X0') if wide else ('S9xOpcodesM1X1','S9xOpcodesM0X1')):
    for i,op in enumerate([0xe8,0xc8,0xca,0x88]):
     ptr=struct.unpack('<I',m.u.mem_read(m.syms[table]+op*4,4))[0]
     assert ptr==start+(wide*4+i)*256,(table,op,hex(ptr))
 rom=a.rom.read_bytes()
 for m in machines:
  offset=0x200000 if a.snes else 0x100000
  m.u.mem_write(m.base+offset,rom);assert m.call('nes_init' if a.snes else '_Z8nes_initPKhjPtS1_',m.base+offset,len(rom),m.base+0x90000,m.base+0xb0000)==0
 for frame in range(a.frames):
  rows=[]
  for m in machines:
   assert m.call('nes_frame' if a.snes else '_Z9nes_framehPFivE',0,0)==0
   if a.snes:
    count=m.call('nes_audio',m.base+0x1d0000);ptr=struct.unpack('<I',m.u.mem_read(m.base+0x1d0000,4))[0]
    rows.append([m.call('nes_hash'),m.call('nes_frames'),count,hashlib.sha256(m.u.mem_read(ptr,count*2)).hexdigest()])
   else:rows.append([m.call(n) for n in ['_Z10nes_cyclesv','_Z8nes_hashv','_Z13fc_video_hashv','_Z13fc_audio_hashv','_Z10fc_samplesv']]+[bytes(m.u.mem_read(m.syms['X'],32)).hex()])
  assert rows[0]==rows[1],(frame,rows)
  print('PASS frame',frame+1,rows[0],flush=True)
 m=machines[1]
 counts={k:struct.unpack('<I',m.u.mem_read(v,4))[0] for k,v in m.syms.items() if k.endswith(('cj_hits','cj_compiles'))}
 if a.table_dispatch:
  assert dispatch_calls[0]>0
  counts={'generated_handler_entries':dispatch_calls[0]}
 else:assert counts and all(counts.values()),counts
 print('PASS interpreter/JIT core endpoints',json.dumps(counts),hashlib.sha256(rom).hexdigest(),flush=True)
if __name__=='__main__':main()
