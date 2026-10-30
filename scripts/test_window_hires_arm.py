import importlib.util,struct,inspect
from pathlib import Path
r=Path(__file__).resolve().parents[1]
def load(n,p):
 s=importlib.util.spec_from_file_location(n,p);m=importlib.util.module_from_spec(s);s.loader.exec_module(m);return m
m=load('engine',r/'tests/amiga/emulator/check_current_arm.py');m.block=0x6000000;m.unpack=load('pack',r/'.context/ax-release-build/core/elf.py').unpack
exec(inspect.getsource(m.engine).replace('count=20000000','count=160000000'),m.__dict__)
for base in [0x12000040,0x150020c0]:
 u,call,addr,mapping,l1,_=m.engine(r/'.context/ax-release-build/player/pocket.elf',base);u.mem_map(mapping+0x200000,0x200000)
 full=call('ps_full_pixels');pixels=[(i*17+i//512*3)&65535 for i in range(512*480)];u.mem_write(full,struct.pack('<%dH'%len(pixels),*pixels));cmd=base+m.control+0x800;u.mem_write(cmd+80,struct.pack('>I',32));u.mem_write(cmd+32,bytes(4))
 for scale in [1,2,3]:
  for fmt in [3,4]:
   pitch=1280*scale+16;span=(240*scale-1)*pitch+1280*scale;nonce=0x12345678
   u.mem_write(mapping,b'\xa5'*0x400000);u.mem_write(addr,struct.pack('>I',nonce));u.mem_write(addr+span-4,struct.pack('>I',nonce^0x31415926));u.mem_write(cmd+36,struct.pack('>III',2,1,scale))
   assert call('nes_present',base,addr,pitch,fmt,nonce)==0
   actual=bytes(u.mem_read(addr,span))
   for y in range(240*scale):
    row=bytearray();hy=y*2//scale
    for x in range(320*scale):
     hx=x*2//scale
     if hx<22 or hx>=618 or hy<16 or hy>=464:c=0xff000000
     else:
      sx=(hx-22)*512//596;sy=hy;v=pixels[sy*512+sx];red=(v>>11)&31;green=(v>>5)&63;blue=v&31
      c=0xff000000|((red<<3|red>>2)<<16)|((green<<2|green>>4)<<8)|(blue<<3|blue>>2)
     b=c.to_bytes(4,'little'if fmt==3 else'big');level=5 if (y//scale)%2 else 8;row.extend(v*level//8 for v in b)
    assert actual[y*pitch:y*pitch+1280*scale]==row,(hex(base),scale,fmt,y)
   assert bytes(u.mem_read(l1,16384))==bytes(16384)
   print('PASS high-res source',hex(base),scale,fmt,flush=True)
