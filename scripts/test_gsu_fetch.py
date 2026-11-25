#!/usr/bin/env python3
"""Check fetch boundaries and video/PCM/GSU state against released semantics."""
import argparse,os,shutil,subprocess,tempfile
from pathlib import Path
from prepare_gsu_fetch import prepare
ROOT=Path(__file__).resolve().parents[1]

def main():
 p=argparse.ArgumentParser(description=__doc__)
 p.add_argument('--rom',type=Path,action='append',default=[])
 p.add_argument('--loop',action='store_true',help='Test ordinary pipeline specialization instead of derived mask')
 p.add_argument('--frames',type=int,default=1200)
 a=p.parse_args()
 if a.frames<1:p.error('--frames must be positive')
 with tempfile.TemporaryDirectory() as name:
  core=Path(name)
  shutil.copytree(ROOT/'development/expanded-20261019/core/src',core/'src')
  prepare(core)
  macros='\n'.join(x for x in (core/'src/fxinst.h').read_text().splitlines() if x.startswith(('#define FX_BIND_FETCH_MASK','#define PRGBANK')))
  source=r'''
#include <stdint.h>
#include <assert.h>
#include <stdio.h>
typedef uint32_t uint32;
static uint32 fx_prg_mask;
static unsigned Fx3Active;
static uint8_t bytes[65536];
static struct { uint32 vPrgBankReg; uint8_t *pvPrgBank; } GSU;
'''+macros+r'''
int main(void) {
 GSU.pvPrgBank=bytes;
 for(unsigned i=0;i<65536;i++)bytes[i]=(i*13+(i>>8))&255;
 for(unsigned fx=0;fx<2;fx++)for(unsigned bank=0;bank<256;bank++) {
  Fx3Active=fx;GSU.vPrgBankReg=bank;fx_prg_mask=0;FX_BIND_FETCH_MASK();
  for(uint32 i=0;i<131072;i++) {
   uint32 expected=(uint16_t)i & (fx && bank<0x40 ? 0x7fff:0xffff);
   assert(&PRGBANK(i)==bytes+expected);
   assert(PRGBANK(i)==bytes[expected]);
  }
  assert(&PRGBANK(UINT32_MAX)==bytes+(fx && bank<0x40?0x7fff:0xffff));
 }
 puts("PASS 67108864 fetch addresses, all banks/FX modes, 16-bit wrap and high-bit boundary");
}
'''
  f=core/'test.c';f.write_text(source)
  subprocess.run(['clang','-std=c99','-O2','-Wall','-Wextra','-Werror','-fsanitize=address,undefined',str(f),'-o',str(core/'test')],check=True)
  subprocess.run([str(core/'test')],check=True)
 if a.rom:
  from compatibility_native import build
  out=ROOT/('.context/gsu-fetch-loop-validation' if a.loop else '.context/gsu-fetch-validation');out.mkdir(exist_ok=True)
  for label,enabled in [('reference',False),('candidate',True)]:
   exe=build(ROOT/'development/gsu-counter-20261111/game_trace.c',out/label,fx3=True,expanded=True,ppu='rows',clip_fix=True,gsu_fetch=enabled and not a.loop,gsu_fetch_loop=enabled and a.loop)
   for index,rom in enumerate(a.rom):
    trace=out/f'{index}-{label}.trace'
    subprocess.run([str(exe),str(rom.resolve()),str(a.frames),str(trace)],check=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'))
    if enabled:
     assert trace.read_bytes()==(out/f'{index}-reference.trace').read_bytes(),rom
     print('PASS video/stereo PCM/GSU state:',rom.name,a.frames,flush=True)
if __name__=='__main__':main()
