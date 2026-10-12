#!/usr/bin/env python3
"""Build host-only scheduling candidates around the exact verified ARM image."""
from pathlib import Path
import argparse,sys,shutil
ROOT=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser();p.add_argument('--mode',choices=['baseline','flip','buffered','batch4','paced'],required=True);a=p.parse_args()
mode=a.mode;out=ROOT/'.context/amiga/pocketsnes-schedule'/mode;out.mkdir(parents=True,exist_ok=True)
source=ROOT/'.context/amiga/pocketsnes-library'
for n in ['pocket.elf','mmu.h','mmu_plan.h','Library-System-Test.sfc']:shutil.copy2(source/n,out/n)
import pocketsnes_library as library
original=library.frontend
def frontend(dest):
 original(dest)
 f=dest/'host.c';s=f.read_text()
 if mode in ('flip','buffered','batch4'):
  start=s.index('  /* Bound the emulation rate without busy spinning.')
  end=s.index('  fc_sample(4,us()-frame_start);',start)
  s=s[:start]+'  /* Diagnostic: display flip acknowledgements provide cadence. */\n'+s[end:]
 if mode=='paced':
  s=s.replace('next_frame_us-frame_start:0;', 'next_frame_us-frame_start:0;target=target>33378u?target-33378u:0;')
 s=s.replace('PLAYER PocketSNES CPU/PPU/SPC/DSP,','PLAYER SCHEDULING '+mode+' PocketSNES CPU/PPU/SPC/DSP,');f.write_text(s)
 if mode in ('buffered','paced'):
  f=dest/'player_audio.h';s=f.read_text().replace('#define AUDIO_PREBUFFER 3u','#define AUDIO_PREBUFFER 6u').replace('3-frame prebuffer','6-frame prebuffer');f.write_text(s)
 if mode=='batch4':
  f=dest/'player_audio.h';s=f.read_text().replace('#define AUDIO_PREBUFFER 3u','#define AUDIO_PREBUFFER 8u').replace('available>=3','available>=4').replace('blocks=available>=4?3:available','blocks=available>=4?4:available').replace('12288','16384').replace('3-frame prebuffer','8-frame prebuffer; 4-frame requests');f.write_text(s)
 if mode=='paced':
  # Match this pinned core's exact NTSC clock: 3579545 / (228*262).
  f=dest/'pace.h';s=f.read_text().replace('33278','33378').replace('16639u','16689u');f.write_text(s)
library.frontend=frontend
# Build only the 68k pieces. The staged ELF has the original runtime mapping/MMU contract.
sys.argv=[sys.argv[0],'--host-only']
code=(ROOT/'scripts/build_pocketsnes_library.py').read_text()
needle='code=code.replace("original=b.prepare"'
pos=code.index(needle)
code=code[:pos]+f"code=code.replace(\"ROOT/'.context/amiga/pocketsnes-library'\",\"ROOT/'.context/amiga/pocketsnes-schedule/{mode}'\")\n"+code[pos:]
exec(compile(code,str(ROOT/'scripts/build_pocketsnes_library.py'),'exec'))
