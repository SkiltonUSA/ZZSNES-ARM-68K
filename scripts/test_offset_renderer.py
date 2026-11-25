#!/usr/bin/env python3
"""Compare mode-2/4 offset rendering and game output under native sanitizers."""
import argparse,os,subprocess
from pathlib import Path
from compatibility_native import build
ROOT=Path(__file__).resolve().parents[1]

def main():
 p=argparse.ArgumentParser(description=__doc__)
 p.add_argument('--rom',type=Path,action='append',default=[])
 p.add_argument('--frames',type=int,default=1200)
 a=p.parse_args()
 if a.frames<1:p.error('--frames must be positive')
 out=ROOT/'.context/offset-validation';out.mkdir(exist_ok=True)
 s=(ROOT/'development/ppu-20261029/tests/raster_fixture.c').read_text()
 s=s.replace('void DrawOBJS(bool8_32, uint8);','void DrawOBJS(bool8_32, uint8);\nvoid DrawBackgroundOffset(uint32,uint32,uint8,uint8);\nextern SLineData LineData[240];')
 needle='        fprintf(f, "%u %08x %08x\\n", test,'
 assert s.count(needle)==1
 s=s.replace(needle,'''        /* Exercise all four specializations, tilemap page crossings, scroll
           wrap, priority/flips and partial colour-window spans. DrawOBJS sets
           the tile format and renderer (including colour-math combinations). */
        BG.TileSize = (test & 1) ? 16 : 8;
        unsigned layer = (test >> 2) & 1;
        for (unsigned b = 0; b < 3; b++) {
            PPU.BG[b].SCBase = b * 0x1000;
            PPU.BG[b].SCSize = (test >> (b + 3)) & 3;
        }
        IPPU.Clip[0].Count[layer] = IPPU.Clip[0].Count[4];
        for (unsigned c = 0; c < 2; c++) {
            IPPU.Clip[0].Left[c][layer] = IPPU.Clip[0].Left[c][4];
            IPPU.Clip[0].Right[c][layer] = IPPU.Clip[0].Right[c][4];
        }
        for (unsigned y = 0; y < 240; y++) for (unsigned b = 0; b < 3; b++) {
            LineData[y].BG[b].HOffset = random_word() & 1023;
            LineData[y].BG[b].VOffset = random_word() & 1023;
        }
        DrawBackgroundOffset((test & 2) ? 4 : 2, layer, 2, 6);
''' + needle)
 fixture=out/'fixture.c';fixture.write_text(s)
 env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0')
 for mode in ('rows','offset'):
  exe=build(fixture,out/mode,fx3=True,expanded=True,ppu=mode,clip_fix=True)
  path=out/(mode+'.trace');subprocess.run([str(exe),str(path)],check=True,env=env,stdout=subprocess.DEVNULL)
  if mode=='offset':
   assert path.read_bytes()==(out/'rows.trace').read_bytes()
   print('PASS 2048 offset-background cases: modes 2/4, tiles 8/16, scroll/bank wrap, clipping, depth and blending',flush=True)
  if a.rom:
   exe=build(ROOT/'development/gsu-counter-20261111/game_trace.c',out/mode,fx3=True,expanded=True,ppu=mode,clip_fix=True)
   for i,rom in enumerate(a.rom):
    path=out/f'{i}-{mode}.trace';subprocess.run([str(exe),str(rom.resolve()),str(a.frames),str(path)],check=True,env=env,stdout=subprocess.DEVNULL)
    if mode=='offset':
     assert path.read_bytes()==(out/f'{i}-rows.trace').read_bytes(),rom
     print('PASS video/stereo PCM/GSU:',rom.name,a.frames,flush=True)
if __name__=='__main__':main()
