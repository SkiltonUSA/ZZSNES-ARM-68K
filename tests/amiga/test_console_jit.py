"""Execute emitted A32 on Cortex-A9 model and compare independent arithmetic."""
import ctypes,struct,subprocess,tempfile
from pathlib import Path
from unicorn import Uc,UC_ARCH_ARM,UC_MODE_ARM
from unicorn.arm_const import *
root=Path(__file__).resolve().parents[2]
with tempfile.TemporaryDirectory() as tmp:
 p=Path(tmp);(p/'wrap.c').write_text('#include "amiga/common/arm_jit/register_op.h"\nunsigned emit(unsigned *p,unsigned src,unsigned dst,int delta,unsigned bits,unsigned kind){return cj_emit(p,src,dst,delta,bits,kind,10,5,6);}\n')
 subprocess.run(['clang','-shared','-fPIC','-I'+str(root),str(p/'wrap.c'),'-o',str(p/'emit.dylib')],check=True)
 lib=ctypes.CDLL(str(p/'emit.dylib'));lib.emit.argtypes=[ctypes.POINTER(ctypes.c_uint),ctypes.c_uint,ctypes.c_uint,ctypes.c_int,ctypes.c_uint,ctypes.c_uint]
 u=Uc(UC_ARCH_ARM,UC_MODE_ARM);u.ctl_set_cpu_model(UC_CPU_ARM_CORTEX_A9)
 u.mem_map(0x10000,0x10000);u.mem_map(0x20000,0x10000)
 cases=0
 for bits in (8,16):
  for delta in (-1,0,1):
   for kind in (0,1,2): # no flags, packed NES flags, cached SNES flags
    code=(ctypes.c_uint*32)();n=lib.emit(code,2,4,delta,bits,kind);assert 0<n<=32
    u.mem_write(0x10000,bytes(code)[:n*4]);u.ctl_remove_cache(0x10000,0x10100)
    values=range(256) if bits==8 else [0,1,127,128,255,256,32767,32768,65534,65535]
    for v in values:
     for flags in (0,255,0x41,0x82):
      state=bytearray([0x91]*64);state[2:2+bits//8]=v.to_bytes(bits//8,'little');state[10]=flags
      expected=bytearray(state);result=(v+delta)&((1<<bits)-1);expected[4:4+bits//8]=result.to_bytes(bits//8,'little')
      other=bytearray([0x7d]*64);expect_other=bytearray(other)
      if kind==1:expected[10]=(flags&~0x82)|(0x80 if result&(1<<(bits-1)) else 0)|(2 if not result else 0)
      if kind==2:expect_other[5]=result if bits==8 else int(result!=0);expect_other[6]=result>>(bits-8)
      u.mem_write(0x20000,bytes(state));u.mem_write(0x20100,bytes(other))
      u.reg_write(UC_ARM_REG_R0,0x20000);u.reg_write(UC_ARM_REG_R1,0x20100);u.reg_write(UC_ARM_REG_LR,0x11000)
      u.emu_start(0x10000,0x11000,count=64)
      assert u.reg_read(UC_ARM_REG_PC)==0x11000
      assert u.mem_read(0x20000,64)==expected,(bits,delta,kind,v,flags)
      assert u.mem_read(0x20100,64)==expect_other,(bits,delta,kind,v,flags)
      cases+=1
 print('PASS A9 register translation:',cases,'8/16-bit arithmetic, NZ flags and untouched-state cases')
