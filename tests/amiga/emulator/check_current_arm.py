"""Relocated ARM pixel/store test. No claim of physical cache/scanout proof."""
import sys,struct,subprocess,re,importlib.util,os
from pathlib import Path
from unicorn import Uc,UC_ARCH_ARM,UC_MODE_ARM,UC_HOOK_CODE
from unicorn.arm_const import *
ROOT=Path(__file__).resolve().parents[3]
name="ZZSNES";snes=name=='ZZSNES';snap=ROOT/'development/beta-20261015'
new=ROOT/os.environ.get('CURRENT_ARM_DIR','development/borders-20261016/player');old=ROOT/'development/beta-20261012/build-inputs/player'
elf='pocket.elf'if snes else'fceux.elf';block=0x2000000 if snes else 0x400000;control=0x180000 if snes else 0x80000
spec=importlib.util.spec_from_file_location('pack',snap/'source/core/elf.py');pack=importlib.util.module_from_spec(spec);spec.loader.exec_module(pack);unpack=pack.unpack
raw=bytes(((i*31+i//71)&255 for i in range(122880)))
def engine(path,base):
 image,rel,_=unpack(path.read_bytes())
 for off in rel:struct.pack_into('<I',image,off,struct.unpack_from('<I',image,off)[0]+base)
 sym={f[2]:int(f[0],16)+base for line in subprocess.check_output([__import__('os').environ.get('LLVM_NM','llvm-nm'),path],text=True).splitlines()if len(f:=line.split())==3}
 u=Uc(UC_ARCH_ARM,UC_MODE_ARM);u.ctl_set_cpu_model(UC_CPU_ARM_CORTEX_A9);u.mem_map(base&~4095,block+8192);u.mem_write(base,bytes(image))
 source=base+block-0x60000;u.mem_write(source,raw)
 symbol='nes_pixels'if snes else'_Z10nes_pixelsv'
 counts={'neon':0}
 # Same Unicorn A9 CPACR-read correction as the DOSBox model test.
 # Real firmware policy is checked on hardware; subsequent SIMD instructions run.
 if 'db_neon_available'in sym:
  def cpacr(uc,address,*a):uc.reg_write(UC_ARM_REG_R1,uc.reg_read(UC_ARM_REG_C1_C0_2));uc.reg_write(UC_ARM_REG_PC,address+4)
  u.hook_add(UC_HOOK_CODE,cpacr,begin=sym['db_neon_available'],end=sym['db_neon_available'])
 if 'em_copy_neon'in sym:
  def neon(uc,*a):counts['neon']+=1
  u.hook_add(UC_HOOK_CODE,neon,begin=sym['em_copy_neon'],end=sym['em_copy_neon'])
 def pixels(uc,*a):uc.reg_write(UC_ARM_REG_R0,source);uc.reg_write(UC_ARM_REG_PC,uc.reg_read(UC_ARM_REG_LR))
 u.hook_add(UC_HOOK_CODE,pixels,begin=sym[symbol],end=sym[symbol])
 addr=0x02fff800;mapping=addr&~4095;u.mem_map(mapping,0x200000)
 l1=(base+(0x100000 if snes else 0x40000)+16383)&~16383
 u.mem_write(l1,bytes(0xc000));u.mem_write(base+control+0x2000+60,struct.pack('<I',l1))
 def call(symbol,*args):
  sp=(base+block-0x1000)&~15;end=base+block-0x100;u.reg_write(UC_ARM_REG_SP,sp);u.reg_write(UC_ARM_REG_LR,end)
  for r,v in zip([UC_ARM_REG_R0,UC_ARM_REG_R1,UC_ARM_REG_R2,UC_ARM_REG_R3],args):u.reg_write(r,v)
  for i,v in enumerate(args[4:]):u.mem_write(sp+i*4,struct.pack('<I',v))
  u.emu_start(sym[symbol],end,count=20000000);assert u.reg_read(UC_ARM_REG_PC)==end,(symbol,hex(u.reg_read(UC_ARM_REG_PC)))
  return u.reg_read(UC_ARM_REG_R0)
 return u,call,addr,mapping,l1,counts
for base in [0x12000040,0x150020c0]:
 prior,oldcall,addr,mapping,l1,_=engine(old/elf,base);u,call,_,_,_,counts=engine(new/elf,base)
 for fmt,opt in [(3,16),(4,17),(3,0),(4,256 if snes else 80)]:
  pitch=1296;span=239*pitch+1280;nonce=0x12345678
  for uc in [prior,u]:uc.mem_write(base+control+0x800+32,struct.pack('>I',opt))
  prior.mem_write(mapping,b'\xa5'*0x100000);prior.mem_write(addr,struct.pack('>I',nonce));prior.mem_write(addr+span-4,struct.pack('>I',nonce^0x31415926))
  assert oldcall('nes_present',base,addr,pitch,fmt,nonce)==0
  baseline=bytes(prior.mem_read(mapping,0x100000))
  for available in [False,True]:
   u.reg_write(UC_ARM_REG_C1_C0_2,0xf00000);u.ctl_flush_tb();u.reg_write(UC_ARM_REG_FPEXC,0x40000000)
   regs=[UC_ARM_REG_D0+i for i in range(32)]
   values=[(i+19)*0x0102030405060708&0xffffffffffffffff for i in range(32)]
   for r,v in zip(regs,values):u.reg_write(r,v)
   u.reg_write(UC_ARM_REG_FPSCR,0x1000000)

   if not available:u.reg_write(UC_ARM_REG_C1_C0_2,0);u.ctl_flush_tb()
   for mode in range(4):
    for fast in [0,1]:
     u.mem_write(base+control+0x800+36,struct.pack('>II',mode,fast));u.mem_write(mapping,b'\xa5'*0x100000)
     u.mem_write(addr,struct.pack('>I',nonce));u.mem_write(addr+span-4,struct.pack('>I',nonce^0x31415926))
     assert call('nes_present',base,addr,pitch,fmt,nonce)==0
     wanted=bytearray(baseline)
     for y in range(240):
      level=8 if mode==0 else (0 if y%2 else 8)if mode==1 else(5 if y%2 else 8)if mode==2 else[8,6,4,6][y%4]
      a=addr-mapping+y*pitch
      wanted[a:a+1280]=bytes(v*level//8 for v in baseline[a:a+1280])
     assert bytes(u.mem_read(mapping,0x100000))==wanted,(name,fmt,opt,available,mode,fast,'pixels')
     assert bytes(u.mem_read(l1,16384))==bytes(16384),'mapping not revoked'
     u.reg_write(UC_ARM_REG_C1_C0_2,0xf00000)
     assert [u.reg_read(r)for r in regs]==values,'SIMD registers changed'
     assert u.reg_read(UC_ARM_REG_FPSCR)==0x1000000
     if not available:u.reg_write(UC_ARM_REG_C1_C0_2,0);u.ctl_flush_tb()
  # Bad nonce and cancelled row leave target untouched and map revoked.
  for stop,challenge,expected in [(0,nonce+1,7),(1,nonce,2)]:
   u.mem_write(base+control+128,struct.pack('>I',stop));u.mem_write(addr,struct.pack('>I',nonce));u.mem_write(addr+span-4,struct.pack('>I',nonce^0x31415926));before=bytes(u.mem_read(mapping,0x100000))
   assert call('nes_present',base,addr,pitch,fmt,challenge)==expected
   assert bytes(u.mem_read(mapping,0x100000))==before and bytes(u.mem_read(l1,16384))==bytes(16384)
  u.mem_write(base+control+128,bytes(4))
  print('PASS',name,hex(base),'format',fmt,'view',opt,'baseline pixels, all scanlines, NEON/fallback, SIMD restoration, cancellation',flush=True)
  if os.environ.get('TEST_THIN_SCANLINES'):
   # Every old unfiltered row survives, twice as wide, followed by a black row.
   # The chosen address/pitch straddles THREE 1 MiB mapping sections.
   hpitch=2576;hspan=479*hpitch+2560
   for fast in (0,1):
    u.mem_write(base+control+0x800+36,struct.pack('>II',4,fast))
    u.mem_write(mapping,b'\xa5'*0x200000)
    u.mem_write(addr,struct.pack('>I',nonce));u.mem_write(addr+hspan-4,struct.pack('>I',nonce^0x31415926))
    assert call('nes_present',base,addr,hpitch,fmt,nonce)==0
    wanted=bytearray(b'\xa5'*0x200000)
    for y in range(240):
     source=addr-mapping+y*pitch;target=addr-mapping+y*2*hpitch
     row=b''.join(baseline[source+x*4:source+x*4+4]*2 for x in range(320))
     wanted[target:target+2560]=row
     wanted[target+hpitch:target+hpitch+2560]=bytes(2560)
    assert bytes(u.mem_read(mapping,0x200000))==wanted,'Thin erased or altered a game row / crossed a row boundary'
    assert bytes(u.mem_read(l1,16384))==bytes(16384),'Thin mapping not revoked'
   for stop,challenge,expected in ((0,nonce+1,7),(1,nonce,2)):
    u.mem_write(base+control+128,struct.pack('>I',stop))
    u.mem_write(addr,struct.pack('>I',nonce));u.mem_write(addr+hspan-4,struct.pack('>I',nonce^0x31415926))
    before=bytes(u.mem_read(mapping,0x200000))
    assert call('nes_present',base,addr,hpitch,fmt,challenge)==expected
    assert bytes(u.mem_read(mapping,0x200000))==before
    assert bytes(u.mem_read(l1,16384))==bytes(16384)
   u.mem_write(base+control+128,bytes(4))
   print('PASS Thin: every logical row preserved, black separators, row guards, three-section mapping revoked, cancellation',flush=True)

 assert counts['neon']>0,'NEON path did not execute'
 print('NEON row calls:',counts['neon'],flush=True)
