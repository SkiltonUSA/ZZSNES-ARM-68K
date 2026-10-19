"""Expanded payload relocation/presentation test; not physical cache proof."""
import importlib.util,struct,os
from pathlib import Path
R=Path(__file__).resolve().parents[3]
spec=importlib.util.spec_from_file_location('current',Path(__file__).with_name('check_current_arm.py'));m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
spec=importlib.util.spec_from_file_location('expanded_pack',R/'development/expanded-20261019/core/elf.py');pack=importlib.util.module_from_spec(spec);spec.loader.exec_module(pack)
legacy=m.unpack
def unpack(data):
 try:return legacy(data)
 except ValueError:return pack.unpack(data)
m.unpack=unpack;m.block=0x3000000;m.new=R/'.context/expanded-build/player'
m.main()
for base in [0x12000040,0x150020c0]:
 u,call,addr,mapping,l1,counts=m.engine(m.new/'pocket.elf',base)
 full=call('ps_full_pixels');pixels=[(i*17+i//512*3)&65535 for i in range(512*480)]
 u.mem_write(full,struct.pack('<%dH'%len(pixels),*pixels))
 cmd=base+m.control+0x800;u.mem_write(cmd+80,struct.pack('>I',32));u.mem_write(cmd+32,struct.pack('>I',0))
 for fmt in [3,4]:
  for scan in [0,2]:
   pitch=2576;span=479*pitch+2560;nonce=0x12345678
   u.mem_write(mapping,b'\xa5'*0x200000);u.mem_write(addr,struct.pack('>I',nonce));u.mem_write(addr+span-4,struct.pack('>I',nonce^0x31415926));u.mem_write(cmd+36,struct.pack('>II',scan,1))
   assert call('nes_present',base,addr,pitch,fmt,nonce)==0
   actual=bytes(u.mem_read(addr,span))
   for y in range(480):
    expected=bytearray()
    for x in range(640):
     if x<32 or x>=608 or y<24 or y>=456:c=0xff000000
     else:
      sx=(x-32)*512//576;sy=(y-24)*448//432+16;v=pixels[sy*512+sx]
      red=(v>>11)&31;green=(v>>5)&63;blue=v&31
      c=0xff000000|((red<<3|red>>2)<<16)|((green<<2|green>>4)<<8)|(blue<<3|blue>>2)
     b=c.to_bytes(4,'little'if fmt==3 else'big');level=5 if scan==2 and y%2 else 8
     expected.extend(v*level//8 for v in b)
    assert actual[y*pitch:y*pitch+2560]==expected,(hex(base),fmt,scan,y)
   assert bytes(u.mem_read(l1,16384))==bytes(16384)
   print('PASS expanded 640x480, 512 source columns, format',fmt,'scanlines',scan,'at',hex(base),flush=True)
print('PASS expanded relocated ARM presentation')
