#!/usr/bin/env python3
"""Standalone beta rebuild. No deployment or hardware access.
Default rebuilds 68k/MUI around the verified ARM image; --arm recompiles it.
"""
import argparse,hashlib,importlib.util,json,os,re,shutil,subprocess
from pathlib import Path
from prepare_ppu_experiment import MODES as PPU_MODES
ROOT=Path(__file__).resolve().parents[1]
def run(a):subprocess.run(list(map(str,a)),cwd=ROOT,check=True)
def main():
 p=argparse.ArgumentParser();p.add_argument('--timed-superfx-experiment',action='store_true',help='Isolated Legacy/Timed Super FX candidate; requires --release-ax');p.add_argument('--ppu-test',choices=PPU_MODES,help='Isolated raster experiment; requires --release-ax');p.add_argument('--release-ax',action='store_true',help='Build the current beta with configured AHI output and matching launcher');p.add_argument('--ax-audio-test',action='store_true',help='Isolated per-app ZZ9000AX AHI backend; unit 0 fallback');p.add_argument('--host-service-test',choices=('profile','batch4'),help='Isolated 68k cost profiling or four-frame AHI batch test');p.add_argument('--presentation-test',choices=('serial','overlap'),help='Isolated every-frame presentation test; periodic readbacks and automatic capture disabled');p.add_argument('--ack-poll-us',type=int,choices=(0,250),default=0,help='Experimental bounded fullscreen ACK register polling');p.add_argument('--audio-early',action='store_true',help='Experimental batch-ready AHI handoff before presentation');p.add_argument('--jit-registers',action='store_true',help='Experimental register-only A9 translation; 32 MiB executable cache');p.add_argument('--expanded',action='store_true');p.add_argument('--arm',action='store_true');p.add_argument('--optimisation',action='store_true');p.add_argument('--core-o2',action='store_true');p.add_argument('--superfx',dest='fx3',action='store_true');p.add_argument('--thin-scanlines',action='store_true');p.add_argument('--experimental-chips',action='store_true');p.add_argument('--compatibility',action='store_true');a=p.parse_args()
 if a.timed_superfx_experiment and (not a.release_ax or a.ppu_test):p.error('--timed-superfx-experiment requires --release-ax without --ppu-test')
 if a.ppu_test and not a.release_ax:p.error('--ppu-test requires --release-ax')
 if a.release_ax:
  if a.ax_audio_test or a.presentation_test or a.host_service_test or a.audio_early or a.ack_poll_us:p.error('release build cannot include diagnostic options')
  a.arm=a.optimisation=a.jit_registers=True
  a.expanded=True
 if a.expanded:a.arm=a.optimisation=a.core_o2=a.fx3=True
 if a.optimisation:a.thin_scanlines=True
 if (a.experimental_chips or a.compatibility or a.fx3) and not a.arm:p.error('compatibility experiments require --arm')
 compatibility=a.experimental_chips or a.compatibility or a.fx3
 if a.thin_scanlines and not a.arm:p.error('thin scanlines require --arm')
 m=json.loads((ROOT/'docs/current-build.json').read_text());snap=ROOT/m['snapshot_dir'];source=snap/'source';out=ROOT/('.context/fx3-build'if a.fx3 else'.context/compatibility-build'if compatibility else'.context/current-build');stage=out/'player';lib=out/'library'
 if a.thin_scanlines:
  out=ROOT/('.context/thin-fx3-build' if a.fx3 else '.context/thin-build');stage=out/'player';lib=out/'library'
 if a.core_o2 and not a.optimisation:p.error('--core-o2 requires --optimisation')
 if a.optimisation:
  if not a.arm:p.error('optimisation candidate requires --arm')
  out=ROOT/('.context/optimisation-fx-build' if a.fx3 else '.context/optimisation-build');stage=out/'player';lib=out/'library'
 if a.core_o2:out=ROOT/('.context/optimisation-o2-fx-build' if a.fx3 else '.context/optimisation-o2-build');stage=out/'player';lib=out/'library'
 if a.optimisation:m['version']='0.6.2-dev'
 if a.expanded:
  out=ROOT/'.context/expanded-build';stage=out/'player';lib=out/'library';m['version']='0.6.5-dev'
 if a.jit_registers:
  if not a.arm or not a.optimisation or not a.expanded:p.error('JIT requires current ARM optimisation expanded build')
  out=ROOT/'.context/jit-dispatch32-build';stage=out/'player';lib=out/'library';m['version']='0.7.2-dev'
 if a.audio_early:
  if not a.jit_registers:p.error('--audio-early requires the isolated register build')
  out=out.with_name(out.name+'-audio');stage=out/'player';lib=out/'library'
 if a.ack_poll_us:
  if not a.jit_registers:p.error('--ack-poll-us requires the isolated register build')
  out=out.with_name(out.name+'-ack250');stage=out/'player';lib=out/'library'
 if a.presentation_test:
  if not a.jit_registers or a.audio_early or a.ack_poll_us:p.error('--presentation-test requires the isolated register build with original audio and ACK polling')
  out=out.with_name(out.name+'-present-'+a.presentation_test);stage=out/'player';lib=out/'library'
 if a.ax_audio_test:
  if a.presentation_test!='serial' or a.host_service_test:p.error('--ax-audio-test requires --presentation-test serial without host service profiling/batch4')
  out=out.with_name(out.name+'-ax-audio');stage=out/'player';lib=out/'library'
 if a.host_service_test:
  if a.presentation_test!='serial':p.error('--host-service-test requires --presentation-test serial')
  out=out.with_name(out.name+'-service-'+a.host_service_test);stage=out/'player';lib=out/'library'
 if a.release_ax:
  m['version']=json.loads((ROOT/'development/ax-audio-20261029/release.json').read_text())['version']
  out=ROOT/'.context/ax-release-build';stage=out/'player';lib=out/'library'
 if a.ppu_test:
  out=ROOT/('.context/ppu-'+a.ppu_test+'-build');stage=out/'player';lib=out/'library'
 if a.timed_superfx_experiment:
  from prepare_timed_superfx import VERSION
  out=ROOT/'.context/timed-superfx-build';stage=out/'player';lib=out/'library';m['version']=VERSION
 shutil.copytree(snap/'build-inputs/player',stage,dirs_exist_ok=True);library_snap=ROOT/m['library_snapshot_dir']
 shutil.copytree(library_snap/'build-inputs',lib,dirs_exist_ok=True)
 for header in (library_snap/'source').glob('*.h'):shutil.copy2(header,lib/header.name)
 shutil.copytree(library_snap/'source/zip',lib/'zip',dirs_exist_ok=True)
 shutil.copytree(ROOT/m['host_snapshot_dir'],stage,dirs_exist_ok=True)
 shutil.copytree(ROOT/m['player_overlay_dir'],stage,dirs_exist_ok=True)
 if a.thin_scanlines:
  thin=ROOT/'development/thin-scanlines-20261016'
  shutil.copytree(thin/'host',stage,dirs_exist_ok=True)
  shutil.copytree(thin/'player',stage,dirs_exist_ok=True)
  shutil.copytree(thin/'source',lib,dirs_exist_ok=True)
 if a.optimisation:
  overlay=ROOT/'development/optimisation-20261018'
  shutil.copytree(overlay/'player',stage,dirs_exist_ok=True)
  if (overlay/'source').exists():shutil.copytree(overlay/'source',lib,dirs_exist_ok=True)
  if (overlay/'host').exists():shutil.copytree(overlay/'host',stage,dirs_exist_ok=True)
  shutil.copytree(ROOT/'development/scalable-launcher-20261019/source',lib,dirs_exist_ok=True)
  header=lib/'library_mui.h';header.write_text(header.read_text().replace('0.6.1 Beta','0.6.2-dev'))
 if a.expanded:
  expanded=ROOT/'development/expanded-20261019'
  for name,dest in [('player',stage),('host',stage),('source',lib)]:shutil.copytree(expanded/name,dest,dirs_exist_ok=True)
  for dest in (stage,lib):shutil.copytree(expanded/'common',dest,dirs_exist_ok=True)
 shutil.copy2(library_snap/'source/icon.c',lib/'icon.c')
 (lib/'icon.c').write_text((lib/'icon.c').read_text().replace('DOSBox icon',m['console']+' icon'))
 if m['console']=='ZZNES':
  p=lib/'library_mui.h';s=p.read_text();s=s.replace('"Header supported: NTSC, mapper %d\\nGameplay compatibility varies by title.",mapper','"Header supported: %s, mapper %d\\nGameplay compatibility varies by title.",fc_rom_pal(p,n,model.entry[selected].path)?"PAL":"NTSC",mapper').replace('NTSC mapper 0 or 90, without trainer or battery.','mapper 0/1/2/4/5/90; no trainer or battery.');p.write_text(s)
 if a.jit_registers:
  h=lib/'library_mui.h';h.write_text(re.sub(r'#define LIB_VERSION "[^"]+"','#define LIB_VERSION "'+m['version']+'"',h.read_text()))
 if a.timed_superfx_experiment:
  from prepare_timed_superfx import prepare_launcher
  prepare_launcher(lib)
 snes=m['console']=='ZZSNES';rel=lambda p:str(p.relative_to(ROOT));inside=lambda p:os.environ.get('ARM_CONTAINER_ROOT','/work')+'/'+rel(p)
 if a.release_ax:
  from prepare_audio_window import prepare
  prepare(stage,lib,snes)
 if a.arm:
  core=out/'core';objects=[]
  shutil.copytree(source/'core',core,dirs_exist_ok=True)
  if compatibility:
   shutil.copytree(ROOT/'development/compatibility-20261016/core',core,dirs_exist_ok=True)
  # Derived sprite lists must be rebuilt for normal players too. This single
  # restore fix does not enable the experimental cartridge compatibility path.
  if snes:shutil.copy2(ROOT/'development/compatibility-20261016/core/src/snapshot.c',core/'src/snapshot.c')
  if a.fx3:
   shutil.copytree(ROOT/'development/fx3-20261016/core',core,dirs_exist_ok=True)
   for dest in [stage,core]:shutil.copy2(ROOT/'development/fx3-20261016/cartridge.h',dest/'cartridge.h')
  if a.experimental_chips:
   header=(stage/'cartridge.h').read_text()
   header=header.replace('(map==0x20+i||map==0x30+i||contra)&&type<=2','(((map==0x20+i||map==0x30+i||contra)&&type<=2)||(!i&&map==0x20&&(type==0x13||type==0x14||type==0x15||type==0x1a))||(!i&&map==0x23&&(type==0x34||type==0x35)))')
   (stage/'cartridge.h').write_text(header);(core/'cartridge.h').write_text(header)
  if a.expanded:
   shutil.copytree(expanded/'core',core,dirs_exist_ok=True)
   for dest in (core,stage):shutil.copytree(expanded/'common',dest,dirs_exist_ok=True)
  if a.optimisation and snes:
   shutil.copy2(stage/'region.h',core/'region.h')
   run(['patch','-p1','-d',core,'-i',ROOT/'development/optimisation-20261018/region-clock.patch'])
  if a.timed_superfx_experiment:
   from prepare_timed_superfx import prepare,options
   prepare(core);options(stage)
   m['arm_sources'] += [str((core/'src'/name).relative_to(ROOT)) for name in ('timed_fx.c','fxinst_timed.c')]
  if a.ppu_test:
   from prepare_ppu_experiment import prepare
   prepare(core,a.ppu_test)
  if a.jit_registers:
   from prepare_console_jit import prepare
   jit_begin=prepare(core,stage,snes,dispatch=True)
  if snes:
   prefix=json.loads(os.environ.get('ARM_BUILD_COMMAND','[]'))
   flags=[('-O2' if f=='-Os' and a.core_o2 else f) for f in m['arm_flags']];flags=[f for f in flags if not f.startswith('-I')];flags=[inside(source/'amiga/pocketsnes/libc_config.h')if f=='/work/amiga/pocketsnes/libc_config.h'else f for f in flags]
   if a.expanded:flags=[f.replace('0x2000000','0x3000000') for f in flags]
   includes=[stage,core,*[source/'amiga'/d for d in ['mesen_nes','mesen_bench','arm_debug','arm_debug/zz9000']],core/'src',core/'libretro',core/'libretro/libretro-common/include']
   flags+=['-DEM_SNES']+['-I'+(inside(d)if prefix else str(d))for d in includes]
   if not prefix:flags=[str(source/'amiga/pocketsnes/libc_config.h')if f==inside(source/'amiga/pocketsnes/libc_config.h')else f for f in flags]
  else:
   prefix=[];flags=['--target=arm-none-eabi','-mcpu=cortex-a9','-marm','-mfloat-abi=soft','-mno-unaligned-access','-ffreestanding','-fno-builtin','-fPIC','-fvisibility=hidden','-fno-stack-protector','-O3','-g','-fwrapv','-DFC_ARM','-DDINGUX','-DFC_STATES','-DNES_PLAYER','-DNES_DIRECT','-DZZ_BLOCK_SIZE=0x400000','-DZZ_CONTROL=0x80000','-DMB_MMU','-DMB_DCACHE','-DMB_ALLOCATION=0x400000u','-Wno-deprecated-register','-Wno-writable-strings','-Wno-non-c-typedef-for-linkage']
   flags+=['-I'+str(d)for d in [stage,core,*[source/'amiga'/d for d in ['fceux_bench','mesen_nes','mesen_bench','arm_debug','arm_debug/zz9000']]]]
  if a.jit_registers:
   flags=[f.replace('0x3000000','0x6000000') if snes else f.replace('0x400000','0x3000000') for f in flags]
   flags+=['-DCJ_BEGIN='+hex(jit_begin)]
  build_headers=hashlib.sha256(b''.join(p.read_bytes() for d in [stage,core] for p in sorted(d.rglob('*.h')) if p.name!='zz_payload.h')).digest() if a.expanded else b''
  for i,relpath in enumerate(m['arm_sources']):
   src=ROOT/relpath
   if src.is_relative_to(source/'core'):src=core/src.relative_to(source/'core')
   if src.parent==snap/'build-inputs/player':src=stage/src.name
   if a.expanded and src.name=='runtime.c' and 'pocketsnes' in str(src):src=core/'runtime.c'
   obj=stage/f'{i}.o';objects.append(obj);cpp=src.suffix=='.cpp';cc=('arm-none-eabi-g++'if cpp else'arm-none-eabi-gcc')if snes else os.environ.get('CLANG','clang')
   args=flags+(['-std=c++14','-fno-exceptions','-fno-rtti']+([]if snes else['-nostdinc++'])if cpp else[])
   command=prefix+[cc,*args,'-c',inside(src)if prefix else src,'-o',inside(obj)if prefix else obj]
   fingerprint=hashlib.sha256(build_headers+src.read_bytes()+repr(command).encode()+b''.join(p.read_bytes() for p in sorted((core/'src').glob('*emu.c')))+(core/'src/cpuops.c').read_bytes()).hexdigest() if a.expanded else ''
   stamp=obj.with_suffix('.build-sha256')
   if not a.expanded or not obj.exists() or not stamp.exists() or stamp.read_text()!=fingerprint:
    run(command)
    if a.expanded:stamp.write_text(fingerprint)
  elf=stage/('pocket.elf'if snes else'fceux.elf');path=inside if prefix else str
  if snes:
   gcc=prefix+['arm-none-eabi-gcc','-mcpu=cortex-a9','-marm','-mfloat-abi=soft','-print-libgcc-file-name'];libgcc=subprocess.check_output(gcc,cwd=ROOT,text=True).strip()
   run(prefix+['arm-none-eabi-ld','-shared','-Bsymbolic','--gc-sections','--exclude-libs=ALL','--no-undefined','-T',path(core/'payload.ld'),*map(path,objects),'--start-group',path(core/'libc.a'),libgcc,'--end-group','-o',path(elf)])
  else:run([os.environ.get('LD_LLD','ld.lld'),'-shared','-Bsymbolic','--no-undefined','-T',core/'payload.ld',*objects,'-o',elf])
  spec=importlib.util.spec_from_file_location('pack',core/'elf.py');pack=importlib.util.module_from_spec(spec);spec.loader.exec_module(pack);image,relocs,entry=pack.unpack(elf.read_bytes())
  previous=(stage/'zz_payload.h').read_text();mapping=re.search(r'static const unsigned char zz_mapping_probe\[\]=\{[^}]+\};',previous).group()
  (stage/'zz_payload.h').write_text(f'#define ZZ_ENTRY {entry}u\nstatic const unsigned char zz_image[]={{'+','.join(map(str,image))+'};\n'+mapping+'\nstatic const unsigned long zz_relocations[]={'+','.join(map(str,relocs))+'};\n')
 if a.ack_poll_us:
  from prepare_ack_poll import prepare
  prepare(stage)
 if a.presentation_test:
  from prepare_conversion_overlap import prepare_test
  prepare_test(stage,overlap=a.presentation_test=='overlap')
 # Normal play follows AHI unit 0; detecting AX does not identify connected speakers.
 if a.ax_audio_test:
  from prepare_ax_audio import prepare
  prepare(stage)
 if a.host_service_test:
  from prepare_host_service import prepare
  prepare(stage,a.host_service_test)
 runtime=json.loads(os.environ.get('BETA_CONTAINER_COMMAND','["podman","--connection","nuflix-converter-root","run","--rm","--platform","linux/amd64"]'))
 cc=runtime+['-v',str(ROOT)+':/work','-w','/work',m['image'],'m68k-amigaos-gcc','-std=c99','-noixemul','-m68020','-O2','-g','-Wall','-Wextra','-Werror','-Wno-unused-function','-Wno-misleading-indentation']
 block,control=('0x2000000','0x180000')if snes else('0x400000','0x80000')
 if a.expanded:block='0x3000000'
 if a.jit_registers:block='0x6000000' if snes else '0x3000000'
 defs=['ZZ_HEADLESS_BENCH','ZZ_RELEASE','ZZ_NES','ZZ_BLOCK_SIZE='+block,'ZZ_CONTROL='+control,'ZZ_APP_NAME="'+m['console']+'"','ZZ_APP_VERSION="'+m['version']+'"','ZZ_MIN_STACK=65536','MB_ALLOCATION='+block+'u','MB_MMU','MB_DCACHE','ZZ_HEADLESS_MMU','ZZ_HEADLESS_ICACHE','NES_PLAYER','NES_DIRECT','NES_FULLSCREEN']
 if a.jit_registers:defs+=['CJ_BEGIN='+hex(jit_begin)]
 if a.audio_early:defs+=['EM_AUDIO_EARLY']
 run(cc+['-D'+d for d in defs]+['-I'+rel(stage)]+['-I'+rel(source/'amiga'/d)for d in ['mesen_nes','mesen_bench','arm_debug','arm_debug/zz9000']]+[rel(source/'amiga/arm_debug/zz9000/launcher.c'),rel(stage/'host.c'),rel(stage/'zip/puff.c'),'-lamiga','-o',rel(out/('ZZSnes-FX'if a.fx3 else m['player']))])
 for src,dst in [('launcher.c',m['console']),('icon.c','MakeIcon')]:run(cc+[rel(lib/src)]+([rel(lib/'zip/puff.c')]if src=='launcher.c'else[])+['-lamiga','-o',rel(out/dst)])
 if a.expanded:shutil.copy2(out/'ZZSnes-FX',out/m['player'])
 if a.optimisation:
  from build_scanlines import build
  build(out)
 print('PASS: rebuilt',m['console'],'ARM recompiled'if a.arm else'with verified ARM payload','at',out)
if __name__=='__main__':main()
