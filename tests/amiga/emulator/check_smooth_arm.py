"""Independent fixed-point colour oracle, executed through the relocated A9 presenter."""
import importlib.util,struct,re
from pathlib import Path
p=Path(__file__).with_name('check_current_arm.py');spec=importlib.util.spec_from_file_location('model',p);m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
def rgb(p):
 r=p>>11;g=(p>>5)&63;b=p&31
 return 0xff000000|(((r<<3)|(r>>2))<<16)|(((g<<2)|(g>>4))<<8)|((b<<3)|(b>>2))
for base in [0x12000040,0x150020c0]:
 u,call,addr,mapping,l1,counts=m.engine(m.new/m.elf,base)
 u.reg_write(m.UC_ARM_REG_C1_C0_2,0xf00000);u.reg_write(m.UC_ARM_REG_FPEXC,0x40000000)
 for fmt,opt in [(3,0),(3,16),(4,17),(3,80),(4,91),(4,161),(3,256),(4,417)]:
  pitch=1296;span=239*pitch+1280;nonce=0x12345678
  def present(view,scan,fast=1):
   active_pitch=2576 if scan==4 else pitch;active_span=479*active_pitch+2560 if scan==4 else span
   registers=[m.UC_ARM_REG_D0+i for i in range(32)];values=[(i+3)*0x0102030405060708&0xffffffffffffffff for i in range(32)]
   permission=u.reg_read(m.UC_ARM_REG_C1_C0_2);u.reg_write(m.UC_ARM_REG_C1_C0_2,0xf00000)
   for r,v in zip(registers,values):u.reg_write(r,v)
   u.reg_write(m.UC_ARM_REG_FPSCR,0x1000000);u.reg_write(m.UC_ARM_REG_C1_C0_2,permission)
   u.mem_write(base+m.control+0x800+32,struct.pack('>III',view,scan,fast));u.mem_write(mapping,b'\xa5'*0x200000)
   u.mem_write(addr,struct.pack('>I',nonce));u.mem_write(addr+active_span-4,struct.pack('>I',nonce^0x31415926))
   assert call('nes_present',base,addr,active_pitch,fmt,nonce)==0
   if view&512:
    expected_tag=0x4e454f4e if permission and fast else 0x53544d49 if fast else 0x42415345
    assert int.from_bytes(u.mem_read(base+m.control+0x900+72,4),'big')==expected_tag,'wrong scalar/NEON path'
   u.reg_write(m.UC_ARM_REG_C1_C0_2,0xf00000)
   assert [u.reg_read(r)for r in registers]==values and u.reg_read(m.UC_ARM_REG_FPSCR)==0x1000000,'SIMD state changed'
   u.reg_write(m.UC_ARM_REG_C1_C0_2,permission)
   assert bytes(u.mem_read(l1,16384))==bytes(16384)
   return bytes(u.mem_read(mapping,0x200000))
  sharp=present(opt,0)
  expected=bytearray(sharp);width=216 if opt&1 else 288;left=(320-width)//2;mask=((opt>>4)&3)*8;my=((opt>>6)&3)*8
  for y in range(12,228):
   h=239 if opt&256 else 224;sy=(y-12)*h//216
   if not my<=sy<h-my:continue
   if h==224:sy+=8
   for x in range(left,left+width):
    nearest=(x-left)*256//width
    if not mask<=nearest<256-mask:continue
    pos=max(mask*256,min((255-mask)*256,((x-left)*2+1)*32768//width-128));a=pos//256;b=min(255-mask,a+1);w=pos%256
    ca=rgb(struct.unpack_from('<H',m.raw,2*(sy*256+a))[0]);cb=rgb(struct.unpack_from('<H',m.raw,2*(sy*256+b))[0])
    c=0xff000000|sum(((((ca>>s)&255)*(256-w)+((cb>>s)&255)*w+128)//256)<<s for s in [0,8,16])
    k=addr-mapping+y*pitch+x*4;expected[k:k+4]=c.to_bytes(4,'little'if fmt==3 else'big')
  for available in [False,True]:
   u.reg_write(m.UC_ARM_REG_C1_C0_2,0xf00000 if available else 0);u.ctl_flush_tb()
   for scan in range(5):
    wanted=bytearray(expected)
    if scan==4:
     wanted=bytearray(b'\xa5'*0x200000)
     for y in range(240):
      source=addr-mapping+y*pitch;target=addr-mapping+y*2*2576
      wanted[target:target+2560]=b''.join(expected[source+x*4:source+x*4+4]*2 for x in range(320))
      wanted[target+2576:target+2576+2560]=bytes(2560)
    else:
     for y in range(240):
      level=8 if scan==0 else (0 if y%2 else 8)if scan==1 else (5 if y%2 else 8)if scan==2 else [8,6,4,6][y%4]
      k=addr-mapping+y*pitch;wanted[k:k+1280]=bytes(v*level//8 for v in expected[k:k+1280])
    for fast in [0,1]:
     actual=present(opt|512,scan,fast)
     mismatch=next((i for i,(a,b) in enumerate(zip(actual,wanted)) if a!=b),None)
     if mismatch is not None:
      print('SOURCE unchanged',bytes(u.mem_read(base+m.block-0x60000,len(m.raw)))==m.raw, 'row8', bytes(u.mem_read(base+m.block-0x60000+4096,16)).hex(),flush=True)
     assert mismatch is None,(base,fmt,opt,scan,available,fast,hex(mismatch),actual[mismatch:mismatch+16].hex(),wanted[mismatch:mismatch+16].hex())
  assert counts['blend']>0,'SIMD interpolation was not executed'
  print('PASS smooth A9 pixels, guard bytes, revoked mappings and all five scanline modes / PAL-height-safe interpolation',hex(base),fmt,opt,flush=True)

 # Exercise all weights and partial vector lengths through the real assembly.
 u.reg_write(m.UC_ARM_REG_C1_C0_2,0xf00000);u.ctl_flush_tb()
 a=base+m.block-0x60000;descriptors=a+4096;target=addr+128
 source=bytes((i*37+19)&255 for i in range(263*4));u.mem_write(a,source)
 table=bytearray();expected=bytearray()
 for group in range(80):
  offset=(group*3%249)*4
  ai=bytes(4*(i//4)+i%4 for i in range(16));bi=bytes(4*(i//4+1)+i%4 for i in range(16))
  weights=bytes(((group*4+i//4)*11)&255 for i in range(16))
  table+=struct.pack('<I',offset)+ai+bi+weights
  expected+=bytes((source[offset+x]*(256-w)+source[offset+y]*w+128)//256 for x,y,w in zip(ai,bi,weights))
 u.mem_write(descriptors,bytes(table))
 for n in [0,1,2,3,4,5,7,12,319,320]:
  u.mem_write(target-16,b'\xa5'*1312);before=[u.reg_read(m.UC_ARM_REG_D0+i)for i in range(8,16)]
  call('em_blend_neon',target,a,descriptors,n);length=(n//4)*16
  assert bytes(u.mem_read(target-16,1312))==b'\xa5'*16+expected[:length]+b'\xa5'*(1296-length)
  assert before==[u.reg_read(m.UC_ARM_REG_D0+i)for i in range(8,16)]
 print('PASS NEON all weights, short/tail lengths, guards and AAPCS callee-saved D8-D15',hex(base),flush=True)
