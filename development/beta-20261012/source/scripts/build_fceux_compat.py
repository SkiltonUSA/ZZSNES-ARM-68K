#!/usr/bin/env python3
"""Expanded mapper/PAL player on the accepted border/snapshot frontend; no deployment."""
from pathlib import Path
import shutil,subprocess,json,hashlib,importlib.util
import fceux_compat as c
from amiga import PIN
from build_game_libraries import IMAGE as UI_IMAGE
from build_zz9000_debug import unpack_elf
ROOT=c.ROOT;OUT=c.OUT;BASE=ROOT/'.context/amiga/game-borders/nes';OLD=ROOT/'.context/amiga/fceux-player'
def run(a):subprocess.run(list(map(str,a)),cwd=ROOT,check=True)
def edit(n,a,z):p=OUT/n;p.write_text(c.replace(p.read_text(),a,z))
def main():
 OUT.mkdir(parents=True,exist_ok=True)
 for p in BASE.iterdir():
  if p.is_file() and p.suffix in ('.h','.c','.cpp','.elf'):shutil.copy2(p,OUT/p.name)
 sources=c.prepare()
 for n in ['worker.cpp']:shutil.copy2(OLD/n,OUT/n)
 for n in ['mmu.c','mmu_asm.S','mmu.h','mmu_plan.h']:shutil.copy2(ROOT/'amiga/mesen_bench'/n,OUT/n)
 for n in ['ui_model.h','library_mui.h','library_preview.h','library_saves.h','library_controller.h','snapshot_format.h','launcher.c']:shutil.copy2(ROOT/'amiga/fceux_player'/n,OUT/n)
 edit('library_mui.h','"Header supported: NTSC, mapper %d\\nGameplay compatibility varies by title.",mapper','"Header supported: %s, mapper %d\\nGameplay compatibility varies by title.",fc_rom_pal(p,n,model.entry[selected].path)?"PAL":"NTSC",mapper')
 edit('library_mui.h','NTSC mapper 0 or 90, without trainer or battery.','mapper 0/1/2/4/5/90; no trainer or battery.')
 edit('library_saves.h','#define LIB_STATE_CAP 32768u','#define LIB_STATE_CAP 122880u')
 edit('library_saves.h','!le32(p+28)','le32(p+28)<=1')
 edit('wire.h','#define NES_SAVE 0xd1000u','#define NES_SAVE 0x202000u')
 edit('wire.h','#define NES_SAVE_LIMIT 32768u','#define NES_SAVE_LIMIT 122880u')
 edit('worker.cpp','extern unsigned fc_state_store','extern void fc_set_region(unsigned);extern unsigned fc_get_region();\nextern unsigned fc_state_store')
 edit('worker.cpp','   error=arg>NES_ROM_LIMIT?', '   unsigned region=ad_get(cmd,32);fc_set_region(region);\n   error=(region>1||arg>NES_ROM_LIMIT)?')
 edit('worker.cpp','  ad_put(reply,16,initialized?nes_frames():0);','  ad_put(reply,56,initialized?fc_get_region():0);\n  ad_put(reply,16,initialized?nes_frames():0);')
 edit('host.c','static ULONG next_frame_us;','static ULONG next_frame_us;\nstatic ULONG fc_pal,fc_frame_us=16639u;')
 edit('host.c','ad_put(cmd,12,arg);syncio->push','ad_put(cmd,12,arg);if(op==NES_INIT)ad_put(cmd,32,fc_pal);syncio->push')
 edit('host.c','   if(op==NES_FRAME)frame_reply_ticks','   if((op==NES_INIT||op==NES_FRAME||op==10)&&ad_get(reply,56)!=fc_pal){puts("Region handshake failed");return 0;}\n   if(op==NES_FRAME)frame_reply_ticks')
 edit('host.c',' {int id=nes_cartridge(rom,size);',' fc_pal=fc_rom_pal(rom,size,rom_path);fc_frame_us=fc_period_us(fc_pal);\n printf("REGION %s period_us=%u\\n",fc_pal?"PAL":"NTSC",fc_frame_us);\n {int id=nes_cartridge(rom,size);')
 edit('host.c','fc_next_deadline(frame_start,next_frame_us)','fc_next_deadline(frame_start,next_frame_us,fc_frame_us)')
 p=OUT/'host.c';s=p.read_text().replace('require clean NTSC iNES, mapper 0/90, CHR ROM, no battery/trainer/four-screen','require iNES mapper 0/1/2/4/5/90, no battery/trainer/four-screen').replace('NTSC mapper 0/90 CHR ROM only','mapper 0/1/2/4/5/90').replace('NTSC 32kHz quality0','auto-region 32kHz quality0');p.write_text(s)
 (OUT/'pace.h').write_text('static inline ULONG fc_next_deadline(ULONG now,ULONG previous,ULONG period){if(!previous||(LONG)(now-previous)>(LONG)(period*2u))previous=now;return previous+period;}\n')
 sources += [ROOT/'amiga/mesen_bench/runtime.c',ROOT/'amiga/fceux_bench/runtime.c',OUT/'mmu.c',OUT/'mmu_asm.S',OUT/'worker.cpp',ROOT/'amiga/fceux_player/adapter.cpp',OUT/'direct.cpp']
 flags=['--target=arm-none-eabi','-mcpu=cortex-a9','-marm','-mfloat-abi=soft','-mno-unaligned-access','-ffreestanding','-fno-builtin','-fPIC','-fvisibility=hidden','-fno-stack-protector','-O3','-g','-fwrapv','-DFC_ARM','-DDINGUX','-DFC_STATES','-DNES_PLAYER','-DNES_DIRECT','-DZZ_BLOCK_SIZE=0x400000','-DZZ_CONTROL=0x80000','-DMB_MMU','-DMB_DCACHE','-DMB_ALLOCATION=0x400000u','-Wno-deprecated-register','-Wno-writable-strings','-Wno-non-c-typedef-for-linkage','-I'+str(OUT),'-I'+str(ROOT/'amiga/fceux_bench'),'-I'+str(ROOT/'amiga/mesen_nes'),'-I'+str(ROOT/'amiga/mesen_bench'),'-Iamiga/arm_debug','-Iamiga/arm_debug/zz9000']
 objects=[]
 for i,src in enumerate(sources):
  obj=OUT/f'{i}-{src.stem}.arm.o';objects.append(obj)
  run(['clang',*flags,*(['-std=c++14','-fno-exceptions','-fno-rtti','-nostdinc++'] if src.suffix=='.cpp' else []),'-c',src,'-o',obj])
 run(['/opt/homebrew/opt/lld/bin/ld.lld','-shared','-Bsymbolic','--no-undefined','-T',ROOT/'amiga/fceux_bench/payload.ld',*objects,'-o',OUT/'fceux.elf'])
 spec=importlib.util.spec_from_file_location('pack',ROOT/'amiga/fceux_bench/elf.py');pack=importlib.util.module_from_spec(spec);spec.loader.exec_module(pack)
 image,rel,entry=pack.unpack((OUT/'fceux.elf').read_bytes());mapping,mrel,point=unpack_elf((OUT/'mapping.elf').read_bytes());assert not mrel and not point and len(mapping)<=1024
 array=lambda n,d:'static const unsigned char '+n+'[]={'+','.join(map(str,d))+'};\n'
 (OUT/'zz_payload.h').write_text(f'#define ZZ_ENTRY {entry}u\n'+array('zz_image',image)+array('zz_mapping_probe',mapping)+'static const unsigned long zz_relocations[]={'+','.join(map(str,rel))+'};\n')
 cmd=['podman','--connection','nuflix-converter-root','run','--rm','--platform','linux/amd64','-v',f'{ROOT}:/work','-w','/work',PIN['image'],'m68k-amigaos-gcc','-std=c99','-noixemul','-m68020','-O2','-g','-Wall','-Wextra','-Werror','-Wno-unused-function','-Wno-misleading-indentation'];path=OUT.relative_to(ROOT)
 defines=['-DZZ_HEADLESS_BENCH','-DZZ_RELEASE','-DZZ_NES','-DZZ_BLOCK_SIZE=0x400000','-DZZ_CONTROL=0x80000','-DZZ_APP_NAME="ZZFceux-ui"','-DZZ_APP_VERSION="compat-pal"','-DZZ_MIN_STACK=65536','-DMB_ALLOCATION=0x400000u','-DMB_MMU','-DMB_DCACHE','-DZZ_HEADLESS_MMU','-DZZ_HEADLESS_ICACHE','-DNES_PLAYER','-DNES_DIRECT','-DNES_FULLSCREEN']
 run([*cmd,*defines,'-I'+str(path),'-Iamiga/mesen_nes','-Iamiga/mesen_bench','-Iamiga/arm_debug','-Iamiga/arm_debug/zz9000','amiga/arm_debug/zz9000/launcher.c',path/'host.c','-lamiga','-o',path/'ZZFceux-ui'])
 run([*[UI_IMAGE if v==PIN['image'] else v for v in cmd],'-DLIBRARY_PREVIEW',path/'launcher.c','-lamiga','-o',path/'ZZNesLibrary'])
 files={n:{'bytes':(OUT/n).stat().st_size,'md5':hashlib.md5((OUT/n).read_bytes()).hexdigest(),'sha256':hashlib.sha256((OUT/n).read_bytes()).hexdigest()} for n in ['ZZFceux-ui','ZZNesLibrary','fceux.elf']}
 record={'scope':'Mapper 0/1/2/4/5/90 with CHR RAM and automatic PAL; unchanged 4 MiB ownership/display lifecycle','image_bytes':len(image),'relocations':len(rel),'files':files,'sources':{str(p.relative_to(ROOT)):hashlib.sha256(p.read_bytes()).hexdigest() for p in sources},'hardware_tested':False};(OUT/'build.json').write_text(json.dumps(record,indent=2)+'\n');print(json.dumps(files,indent=2))
if __name__=='__main__':main()
