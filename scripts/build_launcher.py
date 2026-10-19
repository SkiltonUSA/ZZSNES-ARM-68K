#!/usr/bin/env python3
"""Rebuild the current development MUI launcher only. No ARM build or deployment."""
import argparse,json,os,shutil,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def main():
 parser=argparse.ArgumentParser(description=__doc__)
 parser.add_argument('--expanded',action='store_true')
 parser.add_argument('--options-probe',action='store_true')
 parser.add_argument('--probe',choices=['640x480','800x600','1024x768'])
 parser.add_argument('--capture-seconds',type=int,default=0,choices=range(61))
 args=parser.parse_args()
 if args.options_probe and not(args.expanded and args.probe):parser.error('--options-probe needs --expanded --probe')
 m=json.loads((ROOT/'docs/current-build.json').read_text());out=ROOT/'.context/launcher-build';lib=out/'library';snapshot=ROOT/m['library_snapshot_dir']
 if args.expanded:out=ROOT/'.context/expanded-launcher-build';lib=out/'library'
 if lib.exists():shutil.rmtree(lib)
 shutil.copytree(snapshot/'build-inputs',lib)
 for header in (snapshot/'source').glob('*.h'):shutil.copy2(header,lib/header.name)
 shutil.copytree(snapshot/'source/zip',lib/'zip',dirs_exist_ok=True)
 if m['console']=='ZZSNES':shutil.copytree(ROOT/'development/thin-scanlines-20261016/source',lib,dirs_exist_ok=True)
 shutil.copytree(ROOT/'development/optimisation-20261018/source',lib,dirs_exist_ok=True)
 shutil.copytree(ROOT/'development/scalable-launcher-20261019/source',lib,dirs_exist_ok=True)
 if args.expanded:
  overlay=ROOT/'development/expanded-20261019'
  for name in ('source','common'):shutil.copytree(overlay/name,lib,dirs_exist_ok=True)
 p=lib/'library_mui.h';p.write_text(p.read_text().replace('0.7.1 Beta','0.7.2-dev').replace('0.6.1 Beta','0.6.2-dev'))
 runtime=json.loads(os.environ.get('BETA_CONTAINER_COMMAND','["podman","--connection","nuflix-converter-root","run","--rm","--platform","linux/amd64"]'))
 flags=[];target=out/m['console']
 if args.probe:
  width,height=args.probe.split('x');target=out/(m['console']+'-Layout'+width)
  shutil.copy2(ROOT/'tests/launcher_layout_probe.h',lib/'launcher_layout_probe.h')
  flags=['-DZZ_UI_LAYOUT_PROBE',f'-DZZ_UI_TEST_WIDTH={width}',f'-DZZ_UI_TEST_HEIGHT={height}',f'-DZZ_UI_CAPTURE_TICKS={args.capture_seconds*50}']
 if args.options_probe:
  shutil.copy2(ROOT/'tests/expanded_options_ui_probe.h',lib/'expanded_options_ui_probe.h');flags+=['-DZZ_UI_OPTIONS_PROBE']
 subprocess.run(runtime+['-v',str(ROOT)+':/work','-w','/work',m['image'],'m68k-amigaos-gcc','-std=c99','-noixemul','-m68020','-O2','-g','-Wall','-Wextra','-Werror','-Wno-unused-function','-Wno-misleading-indentation',str((lib/'launcher.c').relative_to(ROOT)),str((lib/'zip/puff.c').relative_to(ROOT)),'-lamiga','-o',str(target.relative_to(ROOT))]+flags,check=True)
 print('PASS launcher',target)
if __name__=='__main__':main()
