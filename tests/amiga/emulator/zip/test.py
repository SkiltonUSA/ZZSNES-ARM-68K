#!/usr/bin/env python3
"""Stored/deflate ZIP32 loader, big bounds, CRC, ambiguity and plain-ROM parity."""
import io,os,random,struct,subprocess,tempfile,zipfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[4]
HEADER=Path(os.environ.get('ROM_ZIP_INCLUDE',ROOT/'amiga/emulator/zip'))
with tempfile.TemporaryDirectory()as t:
 t=Path(t);exe=t/'read'
 subprocess.run([os.environ.get('CC','cc'),'-std=c99','-O1','-g','-fsanitize=address,undefined','-Wall','-Wextra','-Wno-unused-function','-Wno-misleading-indentation','-I'+str(HEADER),str(Path(__file__).with_name('read.c')),str(HEADER/'puff.c'),'-o',str(exe)],check=True)
 data=bytes(random.Random(734).randrange(256)for _ in range(40000)) # repeat data also exercises distance copies
 varied=random.Random(19).randbytes(65536);expected=t/'expected';runs=0
 def archive(entries,method=8,comment=b''):
  b=io.BytesIO()
  with zipfile.ZipFile(b,'w',compression=method)as z:
   for n,d in entries:z.writestr(n,d)
   z.comment=comment
  return b.getvalue()
 def check(blob,code=0,raw=data,snes=0,cap=100000,suffix='.zip'):
  global runs
  p=t/('game'+suffix);p.write_bytes(blob);expected.write_bytes(raw)
  out=subprocess.check_output([str(exe),str(p),str(cap),str(snes),str(expected)],text=True)
  assert int(out.split()[0])==code,(code,out);runs+=1
  return out
 for method in [0,8]:
  for raw in [data,varied,b'\0'*100000]:
   check(archive([('sub/Game (Europe).NES',raw),('ReadMe.txt',b'hello')],method),raw=raw)
 check(archive([('Game.smc',data)]),snes=1)
 check(archive([('Game.sfc',data)]),snes=1)
 check(archive([('__MACOSX/._Game.nes',b'junk'),('Game.nes',data)]))
 check(archive([('../../outside/Game.nes',data)]));assert not (t.parent/'outside').exists()
 check(archive([('Game.nes',data)],comment=b'x'*65535))
 check(archive([('Game.nes',data)]),3,cap=39999)
 check(data,suffix='.nes');check(data,3,suffix='.nes',cap=39999)
 check(archive([('Game.sfc',data)]),6)
 check(archive([('Game.nes',data)]),6,snes=1)
 check(archive([('One.nes',data),('Two.nes',data)]),7)
 check(archive([('Only.txt',data)]),6)
 check(archive([('x'*512+'.nes',data)]),5)
 check(archive([('Game.nes',data)],zipfile.ZIP_BZIP2),5)
 base=archive([('Game.nes',data)]);cd=base.index(b'PK\x01\x02');end=base.index(b'PK\x05\x06')
 def change(at,fmt,n):
  b=bytearray(base);struct.pack_into(fmt,b,at,n);return bytes(b)
 check(change(cd+8,'<H',1),5) # encrypted
 check(change(end+4,'<H',1),5) # split archive
 check(change(end+10,'<H',65535),5)
 check(change(cd+24,'<I',0xffffffff),5)
 check(change(cd+24,'<I',100001),3)
 check(change(cd+20,'<I',1000000),3)
 check(change(cd+42,'<I',cd),4)
 check(change(end+16,'<I',0xffffffff),5)
 check(change(end+16,'<I',end+100),4)
 check(change(cd+28,'<H',65535),4)
 check(change(26,'<H',1),4)
 check(change(14,'<I',123),4) # central/local CRC mismatch
 b=bytearray(base);struct.pack_into('<I',b,14,123);struct.pack_into('<I',b,cd+16,123);check(b,9)
 b=bytearray(base);b[30+len('Game.nes')]=255;check(b,8)
 for n in [0,1,20,len(base)-1,len(base)-22]:check(base[:n],4)
 # A non-seekable writer uses a data descriptor and zero local sizes.
 class NonSeek(io.BytesIO):
  def seekable(self):return False
  def seek(self,*args):raise io.UnsupportedOperation()
 b=NonSeek()
 with zipfile.ZipFile(b,'w',compression=8)as z:z.writestr('Game.nes',data)
 check(b.getvalue())
 print('PASS',runs,'ZIP/plain cases under ASan/UBSan; output guards, exact bytes, CRC, bounds and rejection paths')
