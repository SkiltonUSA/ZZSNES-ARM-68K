#!/usr/bin/env python3
"""Build a separate SMW fullscreen preview using the accepted NES frontend.
No deployment, commercial-ROM embedding or firmware changes.
"""
import argparse,sys,shutil,subprocess,hashlib,json,importlib.util,difflib
from pathlib import Path
import build_pocketsnes as b
import build_fceux_player as front
p=argparse.ArgumentParser();p.add_argument('--host-only',action='store_true');a=p.parse_args()
ROOT=b.ROOT;OUT=ROOT/'.context/amiga/pocketsnes-player';OUT.mkdir(parents=True,exist_ok=True)
b.OUT=front.OUT=OUT
if a.host_only:
 for name in ['pocket.elf','mmu.h','mmu_plan.h']:
  if not (OUT/name).is_file():raise SystemExit('Build the ARM player first; missing '+name)
 if '#define MB_TABLE_BEGIN 0x100000u' not in (OUT/'mmu_plan.h').read_text():
  raise SystemExit('Refusing mismatched MMU layout')
front.prepare_frontend('release-profile')
def edit(name,fn):
 f=OUT/name;f.write_text(fn(f.read_text()))
# Preserve the established 24 MiB SNES layout. None of these are physical addresses.
def wire(s):
 for old,new in [('0xe0000u','0x130000u'),('0x100000u','0x190000u'),('1048592u','524288u'),('0xd0000u','0x170000u'),('0xd1000u','0x171000u')]:s=s.replace(old,new)
 return s
edit('wire.h',wire)
edit('nes_api.h',lambda s:s.replace('uint8_t buttons','uint16_t buttons').replace('// 0 =','\n#ifdef __cplusplus\nextern "C" {\n#endif\n// 0 =')+'\n#ifdef __cplusplus\n}\n#endif\n')
edit('worker.cpp',lambda s:s.replace('(uint8_t)arg','(uint16_t)arg').replace('arg&256u','arg&65536u').replace('for(unsigned i=0;i<count;i++)','for(unsigned i=0;i<count*2;i++)').replace('count*2);ad_put(reply,40,count)','count*4);ad_put(reply,40,count)'))
# Same locked-bitmap temporary mapping/revocation, new RGB565 conversion only.
edit('direct.cpp',lambda s:s.replace('#include "fast_pixels.h"','#include "pixels.h"').replace('if(!span || !src', 'if((format!=ND_BGRA32 && format!=ND_ARGB32) || !span || !src'))
shutil.copy2(b.SRC/'pixels.h',OUT/'pixels.h')
def host(s):
 s=s.replace('fc_scripted_input=1;', 'fc_scripted_input=player_audit=1;')
 s=s.replace('else return -1;}','else if(!strcmp(argv[5],"INPUTFAST"))fc_scripted_input=1;else return -1;}')
 s=s.replace('static UBYTE buttons;','static UWORD buttons;')
 s=s.replace('UBYTE input=(UBYTE)(buttons|fc_joy_buttons);','UWORD input=buttons|fc_joy_buttons;').replace('static UBYTE last;','static UWORD last;')
 s=s.replace('frames>=89&&frames<99?8:0','((frames>=360&&frames<368)||(frames>=480&&frames<488))?8:0')
 s=s.replace('?256u:0u','?65536u:0u')
 s=s.replace('case 0x4c:bit=16;', 'case 0x20:bit=256;break;case 0x21:bit=512;break;case 0x33:bit=1024;break;case 0x34:bit=2048;break;case 0x4c:bit=16;')
 s=s.replace('#include "palette.h"','#include "palette.h"\n#include "pixels.h"')
 old='{unsigned index=((unsigned)indices[2*(y*256+x)]<<8)|indices[2*(y*256+x)+1];if(index>=512){bad++;continue;}c=nes_player_palette[index];}'
 assert old in s;s=s.replace(old,'{unsigned pixel=((unsigned)indices[2*(y*256+x)]<<8)|indices[2*(y*256+x)+1];c=pocket_rgb(pixel);}')
 start=s.index(' {int id=nes_cartridge');end=s.index('\n#endif',start)
 s=s[:start]+' if(nes_cartridge(rom,size)<0){puts("This preview requires the validated 512 KiB Super Mario World (USA) cartridge");goto done;}\n printf("CARTRIDGE SNES ordinary LoROM bytes=%u\\n",size);'+s[end:]
 s=s.replace('FCEUX CPU/old PPU/APU, NTSC 32kHz quality0','PocketSNES CPU/PPU/SPC/DSP, NTSC 32040Hz stereo')
 s=s.replace('FCEUX core rejected cartridge: NTSC mapper 0/90 CHR ROM only','SNES core rejected cartridge')
 s=s.replace('ZZFceux','ZZSnes-Pocket').replace('ZZFCEUX','ZZSNES_POCKET').replace('ROM.nes','ROM.sfc')
 return s
edit('host.c',host)
edit('joystick.h',lambda s:s.replace('static UBYTE fc_read_buttons','static UWORD fc_read_buttons').replace('red=A blue=B','red=B blue=Y'))
edit('player_frontend.h',lambda s:s.replace('an NTSC .nes cartridge','the SMW .sfc cartridge').replace('#?.nes','#?.sfc').replace('PROGDIR:ROMs','PROGDIR:').replace('ZZFceux','ZZSnes-Pocket'))
edit('fullscreen.h',lambda s:s.replace('ZZFceux','ZZSnes-Pocket').replace('NES=256x240','SNES=256x224 + 8px top/bottom'))
# AHI counts stereo pairs; queue capacity is 1024 pairs, three blocks per request.
def audio(s):
 s=s.replace('data[2048]','data[4096]').replace('AllocVec(6144,','AllocVec(12288,')
 s=s.replace('count*2','count*4').replace('n*2','n*4')
 s=s.replace('ahir_Frequency=32000','ahir_Frequency=32040').replace('AHIST_M16S','AHIST_S16S')
 return s.replace('32000 Hz mono','32040 Hz stereo').replace('6144 bytes','12288 bytes').replace('ZZFceux','ZZSnes-Pocket')
edit('player_audio.h',audio)
(OUT/'cartridge.h').write_text('static int nes_cartridge(const unsigned char *p,unsigned n){unsigned h=2166136261u;if(n!=524288)return -1;for(unsigned i=0;i<n;i++)h=(h^p[i])*16777619u;return h==0xae8466a1u?0:-1;}\n')
# Unused original demo symbol is required by the common host; never accepted as SNES.
(OUT/'demo.h').write_text('static const unsigned char demo_rom[1]={0};\n')
shutil.copy2(ROOT/'.context/amiga/pocketsnes/libc.a',OUT/'libc.a')
original=b.prepare
def prepare():
 sources=original();f=OUT/'src/soundux.c';old=f.read_text();new=old.replace('sample1 << 4','sample1 * 16')
 for name in ['sample1','sample2']:
  new=new.replace(f'(int32) {name} << shift',f'(int32) {name} * (1 << shift)').replace(f'{name} << shift',f'{name} * (1 << shift)')
 new=new.replace('prev0 << 1','prev0 * 2');f.write_text(new)
 (OUT/'active-audio.patch').write_text(''.join(difflib.unified_diff(old.splitlines(True),new.splitlines(True))))
 return [s if s.name!='bench.c' else b.SRC/'player.c' for s in sources]+[OUT/'direct.cpp']
b.prepare=prepare
if not a.host_only:
 code=(ROOT/'scripts/build_pocketsnes_arm.py').read_text()
 code=code.replace("text=(common/name).read_text()","text=(OUT/name).read_text() if name=='worker.cpp' else (common/name).read_text()")
 code=code.replace("'-DPOCKET_ARM'","'-DPOCKET_ARM','-DNES_PLAYER','-DNES_DIRECT','-I/work/amiga/mesen_nes'")
 exec(compile(code,'build_pocketsnes_arm.py','exec'))
# Reuse strict ELF packing and mapping probe, but compile the fullscreen frontend.
from amiga import PIN
from build_zz9000_debug import unpack_elf
spec=importlib.util.spec_from_file_location('packer',b.SRC/'elf.py');pack=importlib.util.module_from_spec(spec);spec.loader.exec_module(pack)
image,rel,entry=pack.unpack((OUT/'pocket.elf').read_bytes())
subprocess.run(['clang','--target=arm-none-eabi','-mcpu=cortex-a9','-marm','-fPIC','-Iamiga/arm_debug/zz9000','-DZZ_CONTROL=0x180000','-c','amiga/arm_debug/zz9000/mapping_probe.S','-o',OUT/'mapping.o'],cwd=ROOT,check=True)
subprocess.run(['/opt/homebrew/opt/lld/bin/ld.lld','-shared','-Bsymbolic','--no-undefined','-T',ROOT/'amiga/arm_debug/zz9000/payload.ld',OUT/'mapping.o','-o',OUT/'mapping.elf'],check=True)
mapping,mrel,point=unpack_elf((OUT/'mapping.elf').read_bytes());assert not mrel and not point and len(mapping)<=1024
array=lambda n,d:'static const unsigned char '+n+'[]={'+','.join(map(str,d))+'};\n'
(OUT/'zz_payload.h').write_text(f'#define ZZ_ENTRY {entry}u\n'+array('zz_image',image)+array('zz_mapping_probe',mapping)+'static const unsigned long zz_relocations[]={'+','.join(map(str,rel))+'};\n')
relative=OUT.relative_to(ROOT)
subprocess.run(['podman','--connection','nuflix-converter-root','run','--rm','--platform','linux/amd64','-v',f'{ROOT}:/work','-w','/work',PIN['image'],'m68k-amigaos-gcc','-std=c99','-noixemul','-m68020','-O2','-g','-Wall','-Wextra','-Werror','-Wno-unused-function','-Wno-misleading-indentation','-DZZ_HEADLESS_BENCH','-DZZ_RELEASE','-DZZ_NES','-DZZ_BLOCK_SIZE=0x1800000','-DZZ_CONTROL=0x180000','-DZZ_APP_NAME="ZZSnes-Pocket"','-DZZ_APP_VERSION="0.1-preview"','-DZZ_MIN_STACK=65536','-DMB_ALLOCATION=0x1800000u','-DMB_MMU','-DMB_DCACHE','-DZZ_HEADLESS_MMU','-DZZ_HEADLESS_ICACHE','-DNES_PLAYER','-DNES_DIRECT','-DNES_FULLSCREEN','-I'+str(relative),'-Iamiga/mesen_nes','-Iamiga/mesen_bench','-Iamiga/arm_debug','-Iamiga/arm_debug/zz9000','amiga/arm_debug/zz9000/launcher.c',str(relative/'host.c'),'-lamiga','-o',str(relative/'ZZSnes-Pocket')],cwd=ROOT,check=True)
binary=OUT/'ZZSnes-Pocket';record={'upstream':b.PIN,'scope':'SMW fullscreen preview; A9 core/RGB565 presentation; 320x240 XACP triple buffer; 32040Hz stereo AHI unit0; SRAM persistence disabled','image_bytes':len(image),'relocations':len(rel),'executable_bytes':binary.stat().st_size,'executable_md5':hashlib.md5(binary.read_bytes()).hexdigest(),'sha256':{p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in [binary,OUT/'pocket.elf',OUT/'host.c',OUT/'worker.cpp',OUT/'player_audio.h',b.SRC/'player.c',b.SRC/'pixels.h',Path(__file__).resolve()]}}
(OUT/'build.json').write_text(json.dumps(record,indent=2)+'\n');print(json.dumps(record,indent=2))
