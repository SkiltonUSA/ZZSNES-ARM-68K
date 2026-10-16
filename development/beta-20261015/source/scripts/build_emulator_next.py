#!/usr/bin/env python3
"""Isolated next-beta build. No deployment; accepted core inputs remain immutable."""
from pathlib import Path
import argparse,subprocess,shutil,json,hashlib
from amiga import PIN
from prepare_emulator_logos import prepare
ROOT=Path(__file__).resolve().parents[1];OUT=ROOT/'.context/amiga/emulator-next'
def run(args):subprocess.run(list(map(str,args)),cwd=ROOT,check=True)
def build_ui(name):
 base=ROOT/'.context/amiga/emulator-beta/build/libraries'/('ZZNesLibrary' if name=='ZZNES' else 'ZZSnesLibrary')
 stage=OUT/name/'library';stage.mkdir(parents=True,exist_ok=True)
 for p in base.glob('*.h'):shutil.copy2(p,stage/p.name)
 shutil.copy2(base/'launcher.c',stage/'launcher.c')
 for n in ['ui_model.h','library_mui.h','library_scan.h','library_brand.h','library_launch.h','library_style.h','library_keys.h']:
  shutil.copy2(ROOT/'amiga/fceux_player'/n,stage/n)
 p=stage/'library_mui.h';s=p.read_text()
 if name=='ZZNES':s=s.replace('"Header supported: NTSC, mapper %d\\nGameplay compatibility varies by title.",mapper','"Header supported: %s, mapper %d\\nGameplay compatibility varies by title.",fc_rom_pal(p,n,model.entry[selected].path)?"PAL":"NTSC",mapper').replace('NTSC mapper 0 or 90, without trainer or battery.','mapper 0/1/2/4/5/90; no trainer or battery.')
 p.write_text(s);prepare(name,stage)
 rel=stage.relative_to(ROOT)
 cmd=['podman','--connection','nuflix-converter-root','run','--rm','--platform','linux/amd64','-v',str(ROOT)+':/work','-w','/work',PIN['image'],'m68k-amigaos-gcc','-std=c99','-noixemul','-m68020','-O2','-g','-Wall','-Wextra','-Werror','-Wno-unused-function','-Wno-misleading-indentation']
 run(cmd+[rel/'launcher.c','-lamiga','-o',rel/name])
 (stage/'icon.c').write_text((ROOT/'amiga/dosbox/launcher/icon.c').read_text().replace('DOSBox icon',name+' icon'))
 run(cmd+[rel/'icon.c','-lamiga','-o',rel/'MakeIcon'])
 print(name,'UI', (stage/name).stat().st_size,flush=True)
if __name__=='__main__':
 p=argparse.ArgumentParser();p.add_argument('--console',choices=['ZZNES','ZZSNES'],action='append');args=p.parse_args()
 for name in args.console or ['ZZNES','ZZSNES']:build_ui(name)
