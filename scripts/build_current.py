#!/usr/bin/env python3
"""Standalone beta rebuild. No deployment or hardware access.
Default rebuilds 68k/MUI around the verified ARM image; --arm recompiles it.
"""
import argparse,hashlib,importlib.util,json,os,re,shutil,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def run(a):subprocess.run(list(map(str,a)),cwd=ROOT,check=True)
def main():
 p=argparse.ArgumentParser();p.add_argument('--arm',action='store_true');p.add_argument('--superfx',dest='fx3',action='store_true');p.add_argument('--experimental-chips',action='store_true');p.add_argument('--compatibility',action='store_true');a=p.parse_args()
 if (a.experimental_chips or a.compatibility or a.fx3) and not a.arm:p.error('compatibility experiments require --arm')
 compatibility=a.experimental_chips or a.compatibility or a.fx3
 m=json.loads((ROOT/'docs/current-build.json').read_text());snap=ROOT/m['snapshot_dir'];source=snap/'source';out=ROOT/('.context/fx3-build'if a.fx3 else'.context/compatibility-build'if compatibility else'.context/current-build');stage=out/'player';lib=out/'library'
 shutil.copytree(snap/'build-inputs/player',stage,dirs_exist_ok=True);library_snap=ROOT/m['library_snapshot_dir']
 shutil.copytree(library_snap/'build-inputs',lib,dirs_exist_ok=True)
 for header in (library_snap/'source').glob('*.h'):shutil.copy2(header,lib/header.name)
 shutil.copytree(library_snap/'source/zip',lib/'zip',dirs_exist_ok=True)
 shutil.copytree(ROOT/m['host_snapshot_dir'],stage,dirs_exist_ok=True)
 shutil.copytree(ROOT/m['player_overlay_dir'],stage,dirs_exist_ok=True)
 shutil.copy2(library_snap/'source/icon.c',lib/'icon.c')
 (lib/'icon.c').write_text((lib/'icon.c').read_text().replace('DOSBox icon',m['console']+' icon'))
 if m['console']=='ZZNES':
  p=lib/'library_mui.h';s=p.read_text();s=s.replace('"Header supported: NTSC, mapper %d\\nGameplay compatibility varies by title.",mapper','"Header supported: %s, mapper %d\\nGameplay compatibility varies by title.",fc_rom_pal(p,n,model.entry[selected].path)?"PAL":"NTSC",mapper').replace('NTSC mapper 0 or 90, without trainer or battery.','mapper 0/1/2/4/5/90; no trainer or battery.');p.write_text(s)
 snes=m['console']=='ZZSNES';rel=lambda p:str(p.relative_to(ROOT));inside=lambda p:os.environ.get('ARM_CONTAINER_ROOT','/work')+'/'+rel(p)
 if a.arm:
  core=source/'core';objects=[]
  if compatibility:
   core=out/'core'
   shutil.copytree(source/'core',core,dirs_exist_ok=True)
   shutil.copytree(ROOT/'development/compatibility-20261016/core',core,dirs_exist_ok=True)
  if a.fx3:
   for dest in [stage,core]:shutil.copy2(ROOT/'development/fx3-20261016/cartridge.h',dest/'cartridge.h')
  if a.experimental_chips:
   header=(stage/'cartridge.h').read_text()
   header=header.replace('(map==0x20+i||map==0x30+i||contra)&&type<=2','(((map==0x20+i||map==0x30+i||contra)&&type<=2)||(!i&&map==0x20&&(type==0x13||type==0x14||type==0x15||type==0x1a))||(!i&&map==0x23&&(type==0x34||type==0x35)))')
   (stage/'cartridge.h').write_text(header);(core/'cartridge.h').write_text(header)
  if snes:
   prefix=json.loads(os.environ.get('ARM_BUILD_COMMAND','[]'))
   flags=m['arm_flags'];flags=[f for f in flags if not f.startswith('-I')];flags=[inside(source/'amiga/pocketsnes/libc_config.h')if f=='/work/amiga/pocketsnes/libc_config.h'else f for f in flags]
   includes=[stage,core,*[source/'amiga'/d for d in ['mesen_nes','mesen_bench','arm_debug','arm_debug/zz9000']],core/'src',core/'libretro',core/'libretro/libretro-common/include']
   flags+=['-DEM_SNES']+['-I'+(inside(d)if prefix else str(d))for d in includes]
   if not prefix:flags=[str(source/'amiga/pocketsnes/libc_config.h')if f==inside(source/'amiga/pocketsnes/libc_config.h')else f for f in flags]
  else:
   prefix=[];flags=['--target=arm-none-eabi','-mcpu=cortex-a9','-marm','-mfloat-abi=soft','-mno-unaligned-access','-ffreestanding','-fno-builtin','-fPIC','-fvisibility=hidden','-fno-stack-protector','-O3','-g','-fwrapv','-DFC_ARM','-DDINGUX','-DFC_STATES','-DNES_PLAYER','-DNES_DIRECT','-DZZ_BLOCK_SIZE=0x400000','-DZZ_CONTROL=0x80000','-DMB_MMU','-DMB_DCACHE','-DMB_ALLOCATION=0x400000u','-Wno-deprecated-register','-Wno-writable-strings','-Wno-non-c-typedef-for-linkage']
   flags+=['-I'+str(d)for d in [stage,core,*[source/'amiga'/d for d in ['fceux_bench','mesen_nes','mesen_bench','arm_debug','arm_debug/zz9000']]]]
  for i,relpath in enumerate(m['arm_sources']):
   src=ROOT/relpath
   if src.is_relative_to(source/'core'):src=core/src.relative_to(source/'core')
   if src.parent==snap/'build-inputs/player':src=stage/src.name
   obj=stage/f'{i}.o';objects.append(obj);cpp=src.suffix=='.cpp';cc=('arm-none-eabi-g++'if cpp else'arm-none-eabi-gcc')if snes else os.environ.get('CLANG','clang')
   args=flags+(['-std=c++14','-fno-exceptions','-fno-rtti']+([]if snes else['-nostdinc++'])if cpp else[])
   run(prefix+[cc,*args,'-c',inside(src)if prefix else src,'-o',inside(obj)if prefix else obj])
  elf=stage/('pocket.elf'if snes else'fceux.elf');path=inside if prefix else str
  if snes:
   gcc=prefix+['arm-none-eabi-gcc','-mcpu=cortex-a9','-marm','-mfloat-abi=soft','-print-libgcc-file-name'];libgcc=subprocess.check_output(gcc,cwd=ROOT,text=True).strip()
   run(prefix+['arm-none-eabi-ld','-shared','-Bsymbolic','--gc-sections','--exclude-libs=ALL','--no-undefined','-T',path(core/'payload.ld'),*map(path,objects),'--start-group',path(core/'libc.a'),libgcc,'--end-group','-o',path(elf)])
  else:run([os.environ.get('LD_LLD','ld.lld'),'-shared','-Bsymbolic','--no-undefined','-T',core/'payload.ld',*objects,'-o',elf])
  spec=importlib.util.spec_from_file_location('pack',core/'elf.py');pack=importlib.util.module_from_spec(spec);spec.loader.exec_module(pack);image,relocs,entry=pack.unpack(elf.read_bytes())
  previous=(stage/'zz_payload.h').read_text();mapping=re.search(r'static const unsigned char zz_mapping_probe\[\]=\{[^}]+\};',previous).group()
  (stage/'zz_payload.h').write_text(f'#define ZZ_ENTRY {entry}u\nstatic const unsigned char zz_image[]={{'+','.join(map(str,image))+'};\n'+mapping+'\nstatic const unsigned long zz_relocations[]={'+','.join(map(str,relocs))+'};\n')
 runtime=json.loads(os.environ.get('BETA_CONTAINER_COMMAND','["podman","--connection","nuflix-converter-root","run","--rm","--platform","linux/amd64"]'))
 cc=runtime+['-v',str(ROOT)+':/work','-w','/work',m['image'],'m68k-amigaos-gcc','-std=c99','-noixemul','-m68020','-O2','-g','-Wall','-Wextra','-Werror','-Wno-unused-function','-Wno-misleading-indentation']
 block,control=('0x2000000','0x180000')if snes else('0x400000','0x80000')
 defs=['ZZ_HEADLESS_BENCH','ZZ_RELEASE','ZZ_NES','ZZ_BLOCK_SIZE='+block,'ZZ_CONTROL='+control,'ZZ_APP_NAME="'+m['console']+'"','ZZ_APP_VERSION="'+m['version']+'"','ZZ_MIN_STACK=65536','MB_ALLOCATION='+block+'u','MB_MMU','MB_DCACHE','ZZ_HEADLESS_MMU','ZZ_HEADLESS_ICACHE','NES_PLAYER','NES_DIRECT','NES_FULLSCREEN']
 run(cc+['-D'+d for d in defs]+['-I'+rel(stage)]+['-I'+rel(source/'amiga'/d)for d in ['mesen_nes','mesen_bench','arm_debug','arm_debug/zz9000']]+[rel(source/'amiga/arm_debug/zz9000/launcher.c'),rel(stage/'host.c'),rel(stage/'zip/puff.c'),'-lamiga','-o',rel(out/('ZZSnes-FX'if a.fx3 else m['player']))])
 for src,dst in [('launcher.c',m['console']),('icon.c','MakeIcon')]:run(cc+[rel(lib/src)]+([rel(lib/'zip/puff.c')]if src=='launcher.c'else[])+['-lamiga','-o',rel(out/dst)])
 print('PASS: rebuilt',m['console'],'ARM recompiled'if a.arm else'with verified ARM payload','at',out)
if __name__=='__main__':main()
