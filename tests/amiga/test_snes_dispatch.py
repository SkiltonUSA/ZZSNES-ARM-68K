"""A32 direct handlers preserve register, flag, cycle and shutdown semantics."""
import ctypes,struct,subprocess,tempfile
from pathlib import Path
from unicorn import Uc,UC_ARCH_ARM,UC_MODE_ARM
from unicorn.arm_const import *
root=Path(__file__).resolve().parents[2]
class Layout(ctypes.Structure):
 _fields_=[(n,ctypes.c_uint32) for n in ['registers','flags','cpu','overclock','one_cycle','cycles','wait','zero','negative']]
with tempfile.TemporaryDirectory() as tmp:
 p=Path(tmp);(p/'wrap.c').write_text('#include "amiga/common/arm_jit/snes_dispatch_emit.h"\nunsigned emit(unsigned*p,const struct cj_snes_layout*l,unsigned reg,int delta,unsigned bits,unsigned timing,unsigned shutdown){return cj_emit_snes_dispatch(p,l,reg,delta,bits,timing,shutdown);}\n')
 subprocess.run(['clang','-shared','-fPIC','-I'+str(root),str(p/'wrap.c'),'-o',str(p/'emit.dylib')],check=True)
 lib=ctypes.CDLL(str(p/'emit.dylib'));lib.emit.argtypes=[ctypes.POINTER(ctypes.c_uint32),ctypes.POINTER(Layout),ctypes.c_uint,ctypes.c_int,ctypes.c_uint,ctypes.c_uint,ctypes.c_uint]
 count=0
 for base in [0x20000,0x12345000,0x18367000]:
  u=Uc(UC_ARCH_ARM,UC_MODE_ARM);u.ctl_set_cpu_model(UC_CPU_ARM_CORTEX_A9);u.mem_map(base,0x10000)
  l=Layout(base+0x200,base+0x300,base+0x400,base+0x500,base+0x504,16,20,5,6)
  for bits in [8,16]:
   for delta in [-1,1]:
    for timing,shutdown in [(0,0),(1,0),(0,1),(1,1)]:
     code=(ctypes.c_uint32*64)();n=lib.emit(code,ctypes.byref(l),2,delta,bits,timing,shutdown);assert 0<n<=64
     u.mem_write(base+0x1000,bytes(code)[:n*4]);u.ctl_remove_cache(base+0x1000,base+0x1100)
     values=range(256) if bits==8 else [0,1,127,128,255,256,32767,32768,65534,65535]
     for enabled,cycle in [(0,1),(1,3),(1,6),(1,255)]:
      for value in values:
       buf=bytearray([0x91]*0x600);buf[0x202:0x202+bits//8]=value.to_bytes(bits//8,'little');buf[0x500]=enabled;struct.pack_into('<I',buf,0x504,cycle);struct.pack_into('<I',buf,0x410,0x7ffffffe)
       expected=bytearray(buf);result=(value+delta)&((1<<bits)-1);expected[0x202:0x202+bits//8]=result.to_bytes(bits//8,'little');expected[0x305]=result if bits==8 else int(result!=0);expected[0x306]=result>>(bits-8)
       if timing:struct.pack_into('<I',expected,0x410,(0x7ffffffe+(cycle if enabled else 6))&0xffffffff)
       if shutdown:struct.pack_into('<I',expected,0x414,0)
       u.mem_write(base,bytes(buf));u.reg_write(UC_ARM_REG_LR,base+0x2000);u.reg_write(UC_ARM_REG_SP,base+0xf000)
       for i,r in enumerate(range(UC_ARM_REG_R4,UC_ARM_REG_R11+1)):u.reg_write(r,0x400+i)
       u.emu_start(base+0x1000,base+0x2000,count=64)
       assert u.reg_read(UC_ARM_REG_PC)==base+0x2000
       assert u.mem_read(base,len(buf))==expected,(bits,delta,timing,shutdown,enabled,cycle,value)
       assert u.reg_read(UC_ARM_REG_SP)==base+0xf000
       for i,r in enumerate(range(UC_ARM_REG_R4,UC_ARM_REG_R11+1)):assert u.reg_read(r)==0x400+i
       count+=1
 print('PASS',count,'direct-handler cases: relocation, wrap, cached flags, live cycle setting, wait reset, preserved registers')
