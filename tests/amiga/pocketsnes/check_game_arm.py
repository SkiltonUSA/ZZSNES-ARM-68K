#!/usr/bin/env python3
"""Differential external-ROM A9-model test; no board/timing claim."""
import sys,struct,subprocess,hashlib
from pathlib import Path
from unicorn import *
from unicorn.arm_const import *
sys.path.insert(0,str(Path(__file__).resolve().parent))
from check_arm import packer,ROOT
out=ROOT/'.context/amiga/pocketsnes-game-arm'
rom=(ROOT/'.context/amiga/pocketsnes-game/input.sfc').read_bytes()
assert hashlib.sha256(rom).hexdigest()=='0838e531fe22c077528febe14cb3ff7c492f1f5fa8de354192bdff7137c27f5b'
for base,cancel in [(0x12000040,False),(0x150020c0,False),(0x12000040,True)]:
 image,rel,_=packer.unpack((out/'pocket.elf').read_bytes())
 for off in rel:struct.pack_into('<I',image,off,struct.unpack_from('<I',image,off)[0]+base)
 symbols={}
 for line in subprocess.check_output(['/opt/homebrew/opt/llvm/bin/llvm-nm','-n',out/'pocket.elf'],text=True).splitlines():
  fields=line.split()
  if len(fields)==3:symbols[fields[2]]=int(fields[0],16)+base
 u=Uc(UC_ARCH_ARM,UC_MODE_ARM);u.ctl_set_cpu_model(UC_CPU_ARM_CORTEX_A9)
 u.mem_map(base&~4095,0x1802000);u.mem_write(base,bytes(image));u.mem_write(base+0x190000,rom)
 u.mem_write(symbols['_ZL6shared'],struct.pack('<I',base))
 calls=[]
 if cancel:
  u.mem_write(base+0x214000,bytes.fromhex('0000a0e31eff2fe1'))
  def stop(u,address,size,user):
   calls.append(address);u.reg_write(UC_ARM_REG_R0,int(len(calls)>=3))
  u.hook_add(UC_HOOK_CODE,stop,begin=base+0x214004,end=base+0x214004)
 for batch in range(1 if cancel else 10):
  for reg,value in [(UC_ARM_REG_SP,base+0x17ffff0),(UC_ARM_REG_LR,base+0x17f0000),(UC_ARM_REG_R0,1),(UC_ARM_REG_R1,base+0x215000),(UC_ARM_REG_R2,base+0x214000 if cancel else 0),(UC_ARM_REG_R3,0)]:u.reg_write(reg,value)
  u.emu_start(symbols['mesen_test'],base+0x17f0000,count=1500000000)
  assert u.reg_read(UC_ARM_REG_PC)==base+0x17f0000,'worker failed to return'
  r=struct.unpack('<11I',u.mem_read(base+0x215000,44));status=u.reg_read(UC_ARM_REG_R0)
  assert status==(2 if cancel else 0),(status,r)
  if not cancel:assert r[0]==60 and r[2]==(batch+1)*60 and not r[4],r
  print(f'PASS base={base:08x} cancel={cancel} frame={r[2]} video={r[3]:08x} pcm={r[9]:08x} samples={r[10]}',flush=True)
print('PASS two relocated 600-frame active-audio runs and cancellation')
