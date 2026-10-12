"""A9 relocation/strict-alignment, mapper execution and state replay oracle."""
from pathlib import Path
import sys,struct,json,subprocess
from unicorn import Uc,UC_ARCH_ARM,UC_MODE_ARM,UC_HOOK_MEM_READ,UC_HOOK_MEM_WRITE
from unicorn.arm_const import *
ROOT=Path(__file__).resolve().parents[3];sys.path.insert(0,str(ROOT/'amiga/fceux_bench'));from elf import unpack
out=ROOT/'.context/amiga/fceux-compat';rows=json.loads((ROOT/'.context/amiga/nes-rom-import/manifest.json').read_text());native=json.loads((out/'native-results.json').read_text())
for row in rows:
 name=row['filename'];rom=(ROOT/'.context/amiga/nes-rom-import/transfer'/row['transfer']).read_bytes();pal=int('(Europe)' in name or '(Australia)' in name);saved=None;ref=None
 for base in ([0x12000040,0x150020c0] if row['mapper'] in [4,5] else [0x12000040]):
  image,rel,_=unpack((out/'fceux.elf').read_bytes())
  for off in rel:struct.pack_into('<I',image,off,struct.unpack_from('<I',image,off)[0]+base)
  symbols={f[2]:int(f[0],16)+base for line in subprocess.check_output(['/opt/homebrew/opt/llvm/bin/llvm-nm',out/'fceux.elf'],text=True).splitlines() if len(f:=line.split())==3}
  u=Uc(UC_ARCH_ARM,UC_MODE_ARM);u.ctl_set_cpu_model(UC_CPU_ARM_CORTEX_A9);u.mem_map(base&~4095,0x402000);u.mem_write(base,bytes(image));u.mem_write(base+0x100000,rom)
  def access(u,kind,address,size,value,user):
   assert size<=1 or address%size==0,(name,hex(address),size,hex(u.reg_read(UC_ARM_REG_PC)))
  u.hook_add(UC_HOOK_MEM_READ|UC_HOOK_MEM_WRITE,access)
  def call(symbol,*args):
   sp=base+0x3feff0;end=base+0xf0000;u.reg_write(UC_ARM_REG_SP,sp);u.reg_write(UC_ARM_REG_LR,end)
   for r,v in zip([UC_ARM_REG_R0,UC_ARM_REG_R1,UC_ARM_REG_R2,UC_ARM_REG_R3],args):u.reg_write(r,v)
   u.emu_start(symbols[symbol],end,count=50000000);assert u.reg_read(UC_ARM_REG_PC)==end,(symbol,hex(u.reg_read(UC_ARM_REG_PC)));return u.reg_read(UC_ARM_REG_R0)
  def frame():
   assert not call('_Z9nes_framehPFivE',0,0)
   return (call('_Z13fc_video_hashv'),call('_Z13fc_audio_hashv'),call('_Z9fc_cyclesv'),call('_Z10fc_samplesv'))
  call('_Z13fc_set_regionj',pal);assert not call('_Z8nes_initPKhjPtS1_',base+0x100000,len(rom),base+0x90000,base+0xb0000)
  if saved is None:
   for i in range(10):v,a,cy,n=frame()
   oracle=f'FRAME n=10 video={v:08x} audio={a:08x} cycles={cy} samples={n}'
   assert oracle in next(r['output'] for r in native if r['rom']==name and r['mode']=='fixed'),oracle
   n=call('_Z14fc_state_storePhj',base+0x202000,122880);assert 32<n<=122880;saved=bytes(u.mem_read(base+0x202000,n));ref=[frame() for _ in range(3)]
  else:n=len(saved);u.mem_write(base+0x202000,saved)
  assert call('_Z13fc_state_loadPKhj',base+0x202000,n)==1;assert [frame() for _ in range(3)]==ref
  print(f'PASS {name} PAL={pal} base={base:08x} native video/audio/cycles; aligned accesses; state replay {n} bytes',flush=True)
print('PASS all supplied mapper/region A9 models; no physical execution implied')
