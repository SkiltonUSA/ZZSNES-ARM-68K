"""Compare all eight generated handlers with the actual pinned core handlers."""
import argparse,struct
from pathlib import Path
from test_console_jit_core import Machine
p=argparse.ArgumentParser();p.add_argument('--baseline',type=Path,required=True);p.add_argument('--candidate',type=Path,required=True);a=p.parse_args()
count=0
for base in (0x11000040,0x150020c0):
 machines=[Machine(a.baseline/'player/pocket.elf',a.baseline/'core/elf.py',48*1024**2,0x10000040),Machine(a.candidate/'player/pocket.elf',a.candidate/'core/elf.py',96*1024**2,base)]
 for table in ('S9xOpcodesM1X0','S9xOpcodesM0X0','S9xOpcodesM1X1','S9xOpcodesM0X1'):
  for op in (0xe8,0xc8,0xca,0x88):
   for m in machines:m.syms['_test_handler']=struct.unpack('<I',m.u.mem_read(m.syms[table]+4*op,4))[0]
   for enabled,cycle in ((0,3),(1,3),(1,6),(1,255)):
    for seed in range(256):
     results=[]
     for m in machines:
      cpu=bytes([seed])*156;flags=bytes([seed^0x5a])*36
      m.u.mem_write(m.syms['CPU'],cpu);m.u.mem_write(m.syms['ICPU'],flags)
      m.u.mem_write(m.syms['overclock_cycles'],bytes([enabled]));m.u.mem_write(m.syms['one_c'],struct.pack('<I',cycle))
      m.call('_test_handler')
      results.append((bytes(m.u.mem_read(m.syms['CPU'],156)),bytes(m.u.mem_read(m.syms['ICPU'],36))))
     assert results[0]==results[1],(table,op,enabled,cycle,seed)
     count+=1
print('PASS',count,'linked-core handler comparisons at two load addresses, all four mode tables, live overclock settings and exact CPU/flag state')
